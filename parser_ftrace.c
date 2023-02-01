#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "debug.h"

/*
 *                          [next]
 * main function -> function -> ... 
 *                  | [nested_function]
 *                  nested_function -> ....
 *
 *
 *
 */

#define FTRACE_OUPUT "ftrace_output"
//#define FTRACE_OUPUT "triforce-fuzzer.log"

#define STACK_INFO_SIZE 80
struct info {
    char name[40];
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

#define MAX_TASK_SIZE 256
struct task {
    /* curr_info == current head */
    struct info *curr_info;
    /* task main head */
    struct info *head;
    unsigned int pid;
};
static struct task tasks[MAX_TASK_SIZE];
static unsigned int task_top = 0;
static struct task *current;

#define BUF_MAX_SIZE 256

static int chart_cnt = 0;
static FILE *fp = NULL;

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

static void switch_task(struct info **head, unsigned int curr_pid,
                        unsigned int next_pid)
{
    current->curr_info = *head;

    for (int i = 0; i < task_top; i++) {
        if (next_pid == tasks[i].pid) {
            *head = tasks[i].curr_info;
            current = &tasks[i];
            return;
        }
    }
    BUG_ON(task_top >= MAX_TASK_SIZE, "max task:%d/%d", task_top,
           MAX_TASK_SIZE);
    tasks[task_top].pid = next_pid;
    tasks[task_top].head = dup_main_info();
    tasks[task_top].curr_info = tasks[task_top].head;
    *head = tasks[task_top].curr_info;
    current = &tasks[task_top];
    task_top++;
}

static struct info *parser(struct info *head)
{
    char buffer[BUF_MAX_SIZE] = { 0 };

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
        memset(info->name, '\0', 40);
        if (head) {
            info->top = head->top;
            for (int i = 0; i < info->top; i++)
                info->stack_info[i] = head->stack_info[i];
        }

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
            if (head) {
                offset = strlen(info->name);
                info->name[offset - 2] = '\0';
                for (struct info *tmp = head->nested ? head->nested :
                                                       head->next;
                     tmp; tmp = tmp->next) {
                    if (strncmp(tmp->name, info->name, 40) == 0) {
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
            char curr_name[40];
            char next_name[40];

            memset(buffer, '\0', BUF_MAX_SIZE);
            memset(curr_name, '\0', 40);
            memset(next_name, '\0', 40);
            BUG_ON(!fgets(buffer, BUF_MAX_SIZE, fp), "fget() == NULL");
            ret = sscanf(buffer, "%d%c %s %s %s", &next_cpu, &l, curr_name,
                         arrow, next_name);
            BUG_ON(ret != 5, "switch task:|%s|%s|%s|", buffer, curr_name,
                   next_name);
            for (int i = 0; i < 37; i++) {
                if (curr_name[i] == '>' && curr_name[i + 1] == '-') {
                    curr_pid = atoi(&curr_name[i + 2]);
                    break;
                }
            }
            for (int i = 0; i < 37; i++) {
                if (next_name[i] == '>' && next_name[i + 1] == '-') {
                    next_pid = atoi(&next_name[i + 2]);
                    break;
                }
            }

            free(info);
            BUG_ON(!current, "current == NULL");
            if (current->pid == 0)
                current->pid = curr_pid;
            switch_task(&head, curr_pid, next_pid);

            //skip next split line
            BUG_ON(!fgets(buffer, BUF_MAX_SIZE, fp), "fget() == NULL");
            BUG_ON(!fgets(buffer, BUF_MAX_SIZE, fp), "fget() == NULL");
            goto next;
        }

        BUG_ON(1, "parse failed:%s", buffer);

    success:
        BUG_ON(!head, "head == NULL with info:%s", buffer);
        offset = strlen(info->name);
        info->name[offset - name_rollback] = '\0';
        for (struct info *tmp = head->nested ? head->nested : head->next; tmp;
             tmp = tmp->next) {
            if (strncmp(tmp->name, info->name, 40) == 0) {
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

    printf("'ftrace - %f us',\n", time);
    printf("          [['ftrace', 'duration (us)'],\n");

    for (int i = 0; i < task_top; i++) {
        for (struct info *tmp = tasks[i].head->nested ? tasks[i].head->nested :
                                                        tasks[i].head->next;
             tmp;) {
            struct info *next = tmp->next;

            if (!next)
                printf("           ['PID<%d>:%s', %f]],\n", tasks[i].pid,
                       tmp->name, tmp->nr);
            else
                printf("           ['PID<%d>%s', %f],\n", tasks[i].pid,
                       tmp->name, tmp->nr);
            free(tmp);
            tmp = next;
        }
    }
    printf("          \"piechart%d\");\n", chart_cnt++);
}

static void __print_info(struct info *head, unsigned int pid)
{
    float time = 0;

    printf("drawChart(");
    if (strncmp(head->name, "ftrace", sizeof("ftrace")) == 0)
        return;

    printf("'PID<%d>:", pid);
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
static void print_info(struct info *head, unsigned int pid)
{
    for (struct info *tmp = head->nested ? head->nested : head->next; tmp;
         tmp = tmp->next) {
        if (tmp->nested)
            print_info(tmp, pid);
    }
    __print_info(head, pid);
}

// https://jsfiddle.net/linD026/a1fsxpkj/
int main(void)
{
    struct info *main_info = dup_main_info();
    current = &tasks[task_top++];
    current->head = main_info;
    current->curr_info = main_info;

    printf("JavaScript pie chart data table (see draw_pie_chart.js):\n\n");
    fp = fopen(FTRACE_OUPUT, "r+");
    parser(main_info);
    fclose(fp);

    for (int i = 0; i < task_top; i++)
        print_info(tasks[i].head, tasks[i].pid);
    print_ftrace_info();

    printf("\n\nHTML source code:\n");
    printf(
        "<script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\n");
    for (int i = 0; i < chart_cnt; i++)
        printf(
            "       <div id=\"piechart%d\" style=\"width: 900px; height: 500px;\"></div>\n",
            i);

    return 0;
}
