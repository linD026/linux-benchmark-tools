#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "debug.h"

struct info {
    char name[40];
    float nr;
    int cpu;
    struct info *next;
};

static int chart_cnt = 0;
static FILE *fp = NULL;

static float parser(struct info *head)
{
    char buffer[80] = { 0 };
    float time = 0;

    while (memset(buffer, '\0', 80), fgets(buffer, 80, fp) != NULL) {
        char us[5];
        char l, t, a, r;
        int ret = 0;
        int offset = 0;
        int name_rollback = 3;
        struct info *info = malloc(sizeof(struct info));

        BUG_ON(!info, "malloc");
        info->nr = 0;
        info->next = NULL;
        memset(info->name,'\0', 40);

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
        ret = sscanf(buffer, "%d%c %c %s %c",
                     &info->cpu, &l, &t, info->name, &r);
        if (ret == 5 && l == ')' && t == '|' && r == '{') {
            if (head) {
                struct info *tmp = malloc(sizeof(struct info));

                BUG_ON(!tmp, "malloc");
                offset = strlen(info->name);
                info->name[offset - 2] = '\0';
                strncpy(tmp->name, info->name, 40);
                tmp->cpu = info->cpu;
                tmp->nr = 0;
                tmp->next = NULL;
                info->nr = parser(tmp);
                name_rollback = 0;
                goto success;
            }
            offset = strlen(info->name);
            info->name[offset - 2] = '\0';
            head = info;
            continue;
        }
        //   5)   99.954 us   |  }
        ret = sscanf(buffer, "%d%c %f %s %c %c",
                &info->cpu, &l, &info->nr, us, &t, &r);
        if (ret == 6  && l == ')' && t == '|' && r == '}')
            goto end;
        //   5) + 99.954 us   |  }
        ret = sscanf(buffer, "%d%c %c %f %s %c %c",
                &info->cpu, &l, &a, &info->nr, us, &t, &r);
        if (ret == 7 && l == ')' && in_sym_list(a) && t == '|' && r == '}' )  {
end:
            head->nr = info->nr;
            free(info);
            goto out;
        }

        //   5)   1.908 us    |    copy_signal();
        ret = sscanf(buffer, "%d%c %f %s %c %s",
                     &info->cpu, &l, &info->nr, us, &t, info->name);
        if (ret == 6 && l == ')' && t == '|' &&
            info->name[0] != '}' && info->name[0] != '{')
            goto success;
        //   1) + 21.733 us   |    dup_mm();
        ret = sscanf(buffer, "%d%c %c %f %s %c %s",
                     &info->cpu, &l, &a, &info->nr, us, &t, info->name);
         if (ret == 7 && l == ')' && in_sym_list(a) && t == '|' &&
                  info->name[0] != '}' && info->name[0] != '{')
#undef in_sym_list
            goto success;

        BUG_ON(1, "parse failed:%s", buffer);

success:
        BUG_ON(!head, "head == NULL with info:%s", buffer);
        offset = strlen(info->name);
        info->name[offset - name_rollback] = '\0';
        for (struct info *tmp = head->next; tmp; tmp = tmp->next) {
            if (strncmp(tmp->name, info->name, 40) == 0) {
                tmp->nr += info->nr;
                goto skip;
            }
        }
        info->next = head->next;
        head->next = info;
        continue;
skip:
        free(info);
    }

out:
    printf("drawChart(");
    printf("'%s - %f us',\n", head->name, head->nr);
    printf("[['%s', 'duration (us)'],\n", head->name);
    for (struct info *tmp = head->next; tmp;) {
        struct info *next = tmp->next;

        printf("['%s', %f],\n", tmp->name, tmp->nr);
        time += tmp->nr;
        free(tmp);
        tmp = next;
    }
    printf("['%s', %f]],\n", "rest", head->nr - time);
    printf("\"piechart%d\");\n", chart_cnt++);
    time = head->nr;
    free(head);

    return time;
}

// https://jsfiddle.net/linD026/a1fsxpkj/
int main (void)
{
    char buffer[80] = { 0 };

    fp = fopen("ftrace_output", "r");
    parser(NULL);
    fclose(fp);
    
    printf("\n<script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\n");
    for (int i = 0; i < chart_cnt;i++)
        printf("       <div id=\"piechart%d\" style=\"width: 900px; height: 500px;\"></div>\n", i);

    return 0;
}
