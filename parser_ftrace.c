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
struct info {
    char name[40];
    float nr;
    int cpu;
    struct info *next;
    struct info *nested;
    int maybe_nested;
};

static int chart_cnt = 0;
static FILE *fp = NULL;

static struct info *parser(struct info *head)
{
    char buffer[80] = { 0 };

    while (memset(buffer, '\0', 80), fgets(buffer, 80, fp) != NULL) {
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
                        parser(tmp);
                        goto next;
                    }
                }

                parser(info);
                info->nested = info->next;
                info->next = NULL;
                name_rollback = 0;
                goto success;
            }
            offset = strlen(info->name);
            info->name[offset - 2] = '\0';
            head = info;
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
#undef in_sym_list
            goto success;

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

static void __print_ftrace_info(struct info *head)
{
    float time = 0;

    for (struct info *tmp = head->next; tmp; tmp = tmp->next)
        time += tmp->nr;
    printf("'%s - %f us',\n", head->name, time);
    printf("          [['%s', 'duration (us)'],\n", head->name);
    for (struct info *tmp = head->nested ? head->nested : head->next; tmp;) {
        struct info *next = tmp->next;

        if (!next)
            printf("           ['%s', %f]],\n", tmp->name, tmp->nr);
        else
            printf("           ['%s', %f],\n", tmp->name, tmp->nr);
        free(tmp);
        tmp = next;
    }
    printf("          \"piechart%d\");\n", chart_cnt++);
}

static void __print_info(struct info *head)
{
    float time = 0;

    printf("drawChart(");
    if (strncmp(head->name, "ftrace", sizeof("ftrace")) == 0) {
        __print_ftrace_info(head);
        return;
    }
    printf("'%s - %f us',\n", head->name, head->nr);
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
static void print_info(struct info *head)
{
    for (struct info *tmp = head->nested ? head->nested : head->next; tmp;
         tmp = tmp->next) {
        if (tmp->nested)
            print_info(tmp);
    }
    __print_info(head);
}

static void add_top_info(FILE *fp)
{
    const char end_info[] = "  3)   0 us        |  }";

    fseek(fp, 0, SEEK_END);
    fprintf(fp, end_info);
}

// https://jsfiddle.net/linD026/a1fsxpkj/
int main(void)
{
    struct info main_info = {
        .name = "ftrace",
        .cpu = 0,
        .nr = 0,
        .maybe_nested = 0,
        .next = NULL,
        .nested = NULL,
    };

    printf("JavaScript pie chart data table (see draw_pie_chart.js):\n\n");
    fp = fopen("ftrace_output", "r+");
    parser(&main_info);
    fclose(fp);

    print_info(&main_info);

    printf("\n\nHTML source code:\n");
    printf(
        "<script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\n");
    for (int i = 0; i < chart_cnt; i++)
        printf(
            "       <div id=\"piechart%d\" style=\"width: 900px; height: 500px;\"></div>\n",
            i);

    return 0;
}
