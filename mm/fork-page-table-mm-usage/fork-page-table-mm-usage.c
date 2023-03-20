#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>

#define GIG (1ul << 30)
#define NGIG 16
#define SIZE (NGIG * GIG)

#define BUFFER_SIZE 512

unsigned int parser(char *buffer, const char *symbol)
{
    if (strstr(buffer, symbol)) {
        char dump1[BUFFER_SIZE];
        char dump2[BUFFER_SIZE];
        unsigned int tmp;

        sscanf(buffer, "%s %u %s", dump1, &tmp, dump2);
        return tmp;
    }
    return 0;
}

void read_file(const char *prefix)
{
    int pid = (int)getpid();
    char file[BUFFER_SIZE] = { 0 };
    char buffer[BUFFER_SIZE] = { 0 };
    FILE *fp = NULL;
    unsigned int vmPTE = 0;

    sprintf(file, "/proc/%d/status", pid);
    fp = fopen(file, "r");
    if (!fp) {
        perror("fopen");
        exit(1);
    }

    while (fgets(buffer, BUFFER_SIZE, fp)) {
        buffer[BUFFER_SIZE - 1] = '\0';
        vmPTE += parser(buffer, "VmPTE");
        memset(buffer, '0', BUFFER_SIZE);
    }
    fclose(fp);

    printf("[%s] [PID %d] VmPTE:%u kB\n", prefix, pid, vmPTE);
}

void touch(char *p, int page_size)
{
    /* Touch every page */
    for (int i = 0; i < SIZE; i += page_size)
        p[i] = 0;
}

int main(void)
{
    int pid, page_size = getpagesize();
    char *p = NULL;

    p = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
             -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    madvise(p, SIZE, MADV_NOHUGEPAGE);
    touch(p, page_size);

    pid = fork();
    if (pid == 0) {
        sleep(4);
        read_file("Child");
        if (execl("/bin/bash", "bash", "script.sh", NULL) < 0)
            perror("execl");
        exit(0);
    }
    read_file("Parent");
    waitpid(pid, NULL, 0);

    return 0;
}
