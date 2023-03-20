#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>

// https://unix.stackexchange.com/questions/33381/getting-information-about-a-process-memory-usage-from-proc-pid-smaps
// https://stackoverflow.com/questions/9922928/what-does-pss-mean-in-proc-pid-smaps
// https://lwn.net/Articles/230975/

#define USEC 1000000
#define GIG (1ul << 30)
#define NGIG 32
#define SIZE (NGIG * GIG)

#define BUFFER_SIZE 256

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

void read_smaps(const char *prefix)
{
    int pid = (int)getpid();
    char smap_file[BUFFER_SIZE] = { 0 };
    char buffer[BUFFER_SIZE] = { 0 };
    FILE *fp = NULL;
    unsigned int rss = 0;
    unsigned int pss = 0;

    sprintf(smap_file, "/proc/%d/smaps", pid);
    fp = fopen(smap_file, "r");

    while (fread(buffer, BUFFER_SIZE, 1, fp)) {
        buffer[BUFFER_SIZE - 1] = '\0';
        rss += parser(buffer, "Rss:");
        pss += parser(buffer, "Pss:");
    }
    fclose(fp);

    printf("[%s] [PID %d] RSS:%u, PSS:%u (kB)\n", prefix, pid, rss, pss);
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
        read_smaps("Child before touching pages");
        touch(p, page_size);
        read_smaps("Child after touching pages");
        exit(0);
    }
    read_smaps("Parent's mem before child touching pages");
    waitpid(pid, NULL, 0);
    read_smaps("Parent's mem after child touching pages");

    return 0;
}
