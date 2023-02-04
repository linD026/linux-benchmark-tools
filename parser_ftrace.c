#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>

#include "debug.h"

// Allow to use pr_info() without breaking the following:
// ./parser_ftrace > parser_output
#undef debug_stream
#define debug_stream stderr

/*
 * task
 *  |
 *  V                      [next]
 * main function -> function -> ... 
 *                  | [nested_function]
 *                  nested_function -> ....
 *
 */

#define NR_NAME 40

static char ftrace_output[NR_NAME] = "ftrace_output";

#define STACK_INFO_SIZE 80
struct info {
    char name[NR_NAME];
    struct {
        const char *stack_info[STACK_INFO_SIZE];
        unsigned int top;
    };
    float nr;
    int cpu;
    struct info *next;
    struct info *nested;
    int maybe_nested;
};

#define MAX_TASK_SIZE 512
struct task {
    /* curr_info == current head */
    struct info *curr_info;
    /* task main head */
    struct info *head;
    unsigned int pid;
    char name[NR_NAME];
};

static struct task tasks[MAX_TASK_SIZE];
static unsigned int task_top = 0;
static struct task *current;

static unsigned int *prev_cpu;

#define BUF_MAX_SIZE 256
static char buffer[BUF_MAX_SIZE];

static int chart_cnt = 0;
static FILE *fp = NULL;

#define OPT_FTRACE_OUTPUT 0
#define OPT_CHECK_CPU 1
#define NR_OPTION 2
#define OPT_STRING "01"

struct opt_data {
    unsigned int flags[NR_OPTION];
    struct option options[NR_OPTION];
};

static struct opt_data opt_data = {
    .flags = { 0 },
    .options = { 
        { "ftrace_output", optional_argument, 0, OPT_FTRACE_OUTPUT },
        { "check_cpu", no_argument, 0, OPT_CHECK_CPU },
    },
};

static void set_option(int argc, char *argv[])
{
    int opt;
    int opt_index;

    while ((opt = getopt_long(argc, argv, OPT_STRING, opt_data.options,
                              &opt_index)) != -1) {
        switch (opt) {
        case OPT_FTRACE_OUTPUT:
            strncpy(ftrace_output, optarg, NR_NAME);
            ftrace_output[NR_NAME - 1] = '\0';
            break;
        case OPT_CHECK_CPU:
            opt_data.flags[OPT_CHECK_CPU] = 1;
            break;
        default:
            BUG_ON(1, "unkown option: %d", opt);
        }
    }
}

static void check_and_update_prev_cpu(struct info *info)
{
    /*
     * In some of the case (i.e., the single thread mode) we don't check the
     * cpu number since it has the false positive.
     * But for the mutiple thread (e.g., ftrace with function-fork option),
     * we should check the cpu number since it might be the context switch
     * without notification.
     */
    if (!opt_data.flags[OPT_CHECK_CPU])
        return;

    if (prev_cpu)
        BUG_ON(info->cpu != *prev_cpu,
               "Potentially miss something plz check manually\nbuffer:%s",
               buffer);
    prev_cpu = &info->cpu;
}

static struct info *dup_main_info(void)
{
    struct info *tmp = malloc(sizeof(struct info));
    BUG_ON(!tmp, "malloc");

    strncpy(tmp->name, "ftrace", sizeof("ftrace"));
    tmp->top = 0;
    tmp->cpu = 0;
    tmp->nr = 0;
    tmp->maybe_nested = 0;
    tmp->next = NULL;
    tmp->nested = NULL;

    return tmp;
}

static int switch_task(struct info **head, const char *next_name,
                       unsigned int next_pid)
{
    current->curr_info = *head;

    for (int i = 0; i < task_top; i++) {
        if (next_pid == tasks[i].pid) {
            *head = tasks[i].curr_info;
            current = &tasks[i];
            return 0;
        }
    }
    if (task_top >= MAX_TASK_SIZE) {
        // we will output warning at the end.
        return 1;
    }

    strncpy(tasks[task_top].name, next_name, strlen(next_name));
    tasks[task_top].pid = next_pid;
    tasks[task_top].head = dup_main_info();
    tasks[task_top].curr_info = tasks[task_top].head;
    *head = tasks[task_top].curr_info;
    current = &tasks[task_top];
    task_top++;

    return 0;
}

static struct info *parser(struct info *head)
{
    while (memset(buffer, '\0', BUF_MAX_SIZE),
           fgets(buffer, BUF_MAX_SIZE, fp) != NULL) {
        char us[5];
        char l, t, a, r;
        int ret = 0;
        int offset = 0;
        int name_rollback = 3;
        struct info *info = malloc(sizeof(struct info));

        BUG_ON(!info, "malloc");
        info->nr = 0;
        info->maybe_nested = 0;
        info->next = NULL;
        info->nested = NULL;
        memset(info->name, '\0', NR_NAME);
        if (head) {
            info->top = head->top;
            for (int i = 0; i < info->top; i++)
                info->stack_info[i] = head->stack_info[i];
        }

        //printf("> %s", buffer);

        /*
         *   + means that the function exceeded 10 usecs.
         *   ! means that the function exceeded 100 usecs.
         *   # means that the function exceeded 1000 usecs.
         *   * means that the function exceeded 10 msecs.
         *   @ means that the function exceeded 100 msecs.
         *   $ means that the function exceeded 1 sec.
         */
#define in_sym_list(ch) \
    (ch == '+' || ch == '!' || ch == '#' || ch == '*' || ch == '@' || ch == '$')
        //   5)               |  copy_process() {
        ret =
            sscanf(buffer, "%d%c %c %s %c", &info->cpu, &l, &t, info->name, &r);
        if (ret == 5 && l == ')' && t == '|' && r == '{') {
            check_and_update_prev_cpu(info);
            if (head) {
                offset = strlen(info->name);
                info->name[offset - 2] = '\0';
                for (struct info *tmp = head->nested ? head->nested :
                                                       head->next;
                     tmp; tmp = tmp->next) {
                    if (strncmp(tmp->name, info->name, NR_NAME) == 0) {
                        free(info);
                        tmp->maybe_nested = 1;
                        current->curr_info = tmp;
                        parser(tmp);
                        goto next;
                    }
                }

                info->stack_info[info->top++] = head->name;
                current->curr_info = info;
                parser(info);
                info->nested = info->next;
                info->next = NULL;
                name_rollback = 0;
                goto success;
            }
            offset = strlen(info->name);
            info->name[offset - 2] = '\0';
            head = info;
            current->head = info;
            current->curr_info = info;
            continue;
        }
        //   5)   99.954 us   |  }
        ret = sscanf(buffer, "%d%c %f %s %c %c", &info->cpu, &l, &info->nr, us,
                     &t, &r);
        if (ret == 6 && l == ')' && t == '|' && r == '}')
            goto end;
        //   5) + 99.954 us   |  }
        ret = sscanf(buffer, "%d%c %c %f %s %c %c", &info->cpu, &l, &a,
                     &info->nr, us, &t, &r);
        if (ret == 7 && l == ')' && in_sym_list(a) && t == '|' && r == '}') {
        end:
            check_and_update_prev_cpu(info);
            head->nr += info->nr;
            free(info);
            break;
        }
        //   5)   1.908 us    |    copy_signal();
        ret = sscanf(buffer, "%d%c %f %s %c %s", &info->cpu, &l, &info->nr, us,
                     &t, info->name);
        if (ret == 6 && l == ')' && t == '|' && info->name[0] != '}' &&
            info->name[0] != '{')
            goto success;
        //   1) + 21.733 us   |    dup_mm();
        ret = sscanf(buffer, "%d%c %c %f %s %c %s", &info->cpu, &l, &a,
                     &info->nr, us, &t, info->name);
        if (ret == 7 && l == ')' && in_sym_list(a) && t == '|' &&
            info->name[0] != '}' && info->name[0] != '{')
            goto success;
#undef in_sym_list
        //  ------------------------------------------
        //  7)   <...>-659    =>   <...>-681
        // ------------------------------------------
        //
        if (strstr(buffer, "------------------------------------------")) {
            int next_cpu;
            unsigned int curr_pid, next_pid;
            char arrow[3];
            char curr_name[NR_NAME];
            char next_name[NR_NAME];

            // cleanup the cpu checking info
            prev_cpu = NULL;

            memset(buffer, '\0', BUF_MAX_SIZE);
            memset(curr_name, '\0', NR_NAME);
            memset(next_name, '\0', NR_NAME);
            BUG_ON(!fgets(buffer, BUF_MAX_SIZE, fp), "fget() == NULL");
            ret = sscanf(buffer, "%d%c %s %s %s", &next_cpu, &l, curr_name,
                         arrow, next_name);
            //printf("> %s", buffer);
            BUG_ON(ret != 5, "switch task:|%s|%s|%s|", buffer, curr_name,
                   next_name);
            for (int i = 0; i < NR_NAME - 3; i++) {
                if (curr_name[i] == '>' && curr_name[i + 1] == '-') {
                    curr_pid = atoi(&curr_name[i + 2]);
                    curr_name[i + 1] = '\0';
                    break;
                }
            }
            for (int i = 0; i < NR_NAME - 3; i++) {
                if (next_name[i] == '>' && next_name[i + 1] == '-') {
                    next_pid = atoi(&next_name[i + 2]);
                    next_name[i + 1] = '\0';
                    break;
                }
            }

            free(info);
            BUG_ON(!current, "current == NULL");
            if (current->pid == 0) {
                strncpy(current->name, curr_name, strlen(curr_name));
                current->pid = curr_pid;
            }
            if (switch_task(&head, next_name, next_pid))
                break;

            //skip next split line
            BUG_ON(!fgets(buffer, BUF_MAX_SIZE, fp), "fget() == NULL");
            BUG_ON(!fgets(buffer, BUF_MAX_SIZE, fp), "fget() == NULL");
            goto next;
        }

        BUG_ON(1, "parse failed:%s", buffer);

    success:
        BUG_ON(!head, "head == NULL with info:%s", buffer);
        check_and_update_prev_cpu(info);

        offset = strlen(info->name);
        info->name[offset - name_rollback] = '\0';
        for (struct info *tmp = head->nested ? head->nested : head->next; tmp;
             tmp = tmp->next) {
            if (strncmp(tmp->name, info->name, NR_NAME) == 0) {
                tmp->nr += info->nr;
                free(info);
                goto next;
            }
        }
        if (head->nested || head->maybe_nested) {
            info->next = head->nested;
            head->nested = info;
        } else {
            info->next = head->next;
            head->next = info;
        }
    next:;
    }
    return head;
}

static void print_ftrace_info(void)
{
    float time = 0;

    for (int i = 0; i < task_top; i++) {
        for (struct info *tmp = tasks[i].head->next; tmp; tmp = tmp->next)
            time += tmp->nr;
    }

    printf("drawChart(");
    printf("'ftrace - %f us',\n", time);
    printf("          [['ftrace', 'duration (us)'],\n");

    for (int i = 0; i < task_top; i++) {
        for (struct info *tmp = tasks[i].head->next; tmp;) {
            struct info *next = tmp->next;

            if (!next && i + 1 == task_top)
                printf("           ['%s-PID<%d>:%s', %f]],\n", tasks[i].name,
                       tasks[i].pid, tmp->name, tmp->nr);
            else
                printf("           ['%s-PID<%d>%s', %f],\n", tasks[i].name,
                       tasks[i].pid, tmp->name, tmp->nr);
            free(tmp);
            tmp = next;
        }
    }
    printf("          \"piechart%d\");\n", chart_cnt++);
}

static void print_per_task_ftrace_info(struct info *head, const char *name,
                                       unsigned int pid)
{
    float time = 0;

    for (struct info *tmp = head->next; tmp; tmp = tmp->next)
        time += tmp->nr;

    printf("drawChart(");
    printf("'%s-PID<%d>:", name, pid);
    printf("%s - %f us',\n", head->name, time);
    printf("          [['%s', 'duration (us)'],\n", head->name);

    for (struct info *tmp = head->nested ? head->nested : head->next; tmp;) {
        struct info *next = tmp->next;
        if (!next)
            printf("           ['%s-PID<%d>:%s', %f]],\n", name, pid, tmp->name,
                   tmp->nr);
        else
            printf("           ['%s-PID<%d>%s', %f],\n", name, pid, tmp->name,
                   tmp->nr);
        tmp = next;
    }
    printf("          \"piechart%d\");\n", chart_cnt++);
}

static void __print_info(struct info *head, const char *name, unsigned int pid)
{
    float time = 0;

    if (strncmp(head->name, "ftrace", sizeof("ftrace")) == 0) {
        print_per_task_ftrace_info(head, name, pid);
        return;
    }

    printf("drawChart(");
    printf("'%s-PID<%d>:", name, pid);
    for (int i = 0; i < head->top; i++)
        printf("%s:", head->stack_info[i]);
    printf("%s - %f us',\n", head->name, head->nr);
    printf("          [['%s', 'duration (us)'],\n", head->name);

    for (struct info *tmp = head->nested ? head->nested : head->next; tmp;) {
        struct info *next = tmp->next;

        printf("           ['%s', %f],\n", tmp->name, tmp->nr);
        time += tmp->nr;
        free(tmp);
        tmp = next;
    }
    printf("           ['%s', %f]],\n", "rest", head->nr - time);
    printf("          \"piechart%d\");\n", chart_cnt++);
}

/* if [main function]->next->nr == 0, then it has nested_function */
static void print_info(struct info *head, const char *name, unsigned int pid)
{
    for (struct info *tmp = head->nested ? head->nested : head->next; tmp;
         tmp = tmp->next) {
        if (tmp->nested)
            print_info(tmp, name, pid);
    }
    __print_info(head, name, pid);
}

// https://jsfiddle.net/linD026/a1fsxpkj/
int main(int argc, char *argv[])
{
    struct info *main_info = dup_main_info();
    current = &tasks[task_top++];
    current->head = main_info;
    current->curr_info = main_info;

    set_option(argc, argv);

    printf("JavaScript pie chart data table (see draw_pie_chart.js):\n\n");
    fp = fopen(ftrace_output, "r+");
    BUG_ON(!fp, "fopen:%s", strerror(errno));
    parser(main_info);
    fclose(fp);

    for (int i = 0; i < task_top; i++)
        print_info(tasks[i].head, tasks[i].name, tasks[i].pid);
    print_ftrace_info();

    printf("\n\nHTML source code:\n");
    printf(
        "<script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\n");
    for (int i = 0; i < chart_cnt; i++)
        printf(
            "       <div id=\"piechart%d\" style=\"width: 900px; height: 500px;\"></div>\n",
            i);

    pr_info("[OPTION] %s %s\n", opt_data.options[OPT_CHECK_CPU].name,
            opt_data.flags[OPT_CHECK_CPU] ? "set" : "unset");
    pr_info("[OPTION] Scan the file: %s\n", ftrace_output);
    pr_info("nr_task:%d\n", task_top);
    WARN_ON(task_top >= MAX_TASK_SIZE, "max task:%d/%d", task_top,
            MAX_TASK_SIZE);

    return 0;
}
