#include <stdio.h>
#include <string.h>

//  5)   1.908 us    |    copy_signal();
const char *pattern = "%d%c %f %s %c %s";


// https://jsfiddle.net/linD026/a1fsxpkj/
int main (void)
{
    char buffer[80] = { 0 };
    FILE *fp = fopen("parse_ftrace_input", "r");
    float total_time = 0;

    fgets(buffer, 80, fp);
    sscanf(buffer, "%f", &total_time);

    while (fgets(buffer, 80, fp) != NULL) {
        float nr = 0;
        char name[40] = {0};
        int cpu;
        char us[5];
        char l;
        char t;

        sscanf(buffer, "%d%c %f %s %c %s", &cpu, &l, &nr, us, &t, name);
        int offset = strlen(name);
        name[offset - 3] = '\0';
        printf("['%s', %f],\n", name, nr);
        total_time -= nr;
    }
    printf("['%s', %f]\n", "rest", total_time);

    fclose(fp);

    return 0;
}
