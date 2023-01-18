#include <stdio.h>
#include <string.h>

// https://jsfiddle.net/linD026/a1fsxpkj/
int main (void)
{
    char buffer[80] = { 0 };
    /* Format:
     *      function duration
     *        1)   0.207 us    |    copy_semundo();
     *        ...
     */
    FILE *fp = fopen("ftrace_output", "r");
    float total_time = 0;

    fgets(buffer, 80, fp);
    sscanf(buffer, "%f", &total_time);

    while (fgets(buffer, 80, fp) != NULL) {
        float nr = 0;
        char name[40] = {0};
        int cpu;
        char us[5];
        char l, t, a;
        int ret = 0;
        int offset = 0;

        //  5)   1.908 us    |    copy_signal();
        ret = sscanf(buffer, "%d%c %f %s %c %s",
                     &cpu, &l, &nr, us, &t, name);
        if (ret == 6)
            goto success;
        //   1) + 21.733 us   |    dup_mm();
        ret = sscanf(buffer, "%d%c %c %f %s %c %s",
                     &cpu, &l, &a, &nr, us, &t, name);
         if (ret == 7)
            goto success;
        printf("Fail to scan buffer:%s", buffer);
        goto out;

success:
        offset = strlen(name);
        name[offset - 3] = '\0';
        printf("['%s', %f],\n", name, nr);
        total_time -= nr;
    }
    printf("['%s', %f]\n", "rest", total_time);

out:
    fclose(fp);

    return 0;
}
