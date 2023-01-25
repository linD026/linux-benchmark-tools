#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/file.h>
#include <time.h>

#include "debug.h"

#define MEM_SIZE 4096 * 512
struct mm {
    size_t nr_mem;
    void *private;
    void *shared;
};

struct test_info {
    pid_t child_pid;
    struct mm mm;
};

static struct test_info test_info;

static int set_mem(void)
{
    test_info.mm.private =
        mmap(0, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
    if (!test_info.mm.private)
        return -ENOMEM;
    test_info.mm.nr_mem += MEM_SIZE;

    test_info.mm.shared = mmap(0, MEM_SIZE, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_SHARED, 0, 0);
    if (!test_info.mm.shared)
        return -ENOMEM;
    test_info.mm.nr_mem += MEM_SIZE;

    return 0;
}

static void unset_mem(void)
{
    munmap(test_info.mm.private, MEM_SIZE);
    munmap(test_info.mm.shared, MEM_SIZE);
    test_info.mm.nr_mem = 0;
}

static void set_all(void)
{
    BUG_ON(set_mem(), "memory");
}

static void unset_all(void)
{
    unset_mem();
}

static inline void print_cputime(struct timespec ts1, struct timespec ts2)
{
    double duration = 1000.0 * ts2.tv_sec + 1e-6 * ts2.tv_nsec -
                      (1000.0 * ts1.tv_sec + 1e-6 * ts1.tv_nsec);
    flock(STDOUT_FILENO, LOCK_EX);
    printf("%f\n", duration);
    fflush(stdout);
    flock(STDOUT_FILENO, LOCK_UN);
}

#define time_diff(start, end)                                                  \
    (end.tv_nsec - start.tv_nsec < 0 ?                                         \
             (1000000000 + end.tv_nsec - start.tv_nsec) :                      \
             (end.tv_nsec - start.tv_nsec))
#define time_check(_FUNC_)                                                     \
    do {                                                                       \
        struct timespec time_start;                                            \
        struct timespec time_end;                                              \
        double during;                                                         \
        clock_gettime(CLOCK_MONOTONIC, &time_start);                           \
        _FUNC_;                                                                \
        clock_gettime(CLOCK_MONOTONIC, &time_end);                             \
        during = time_diff(time_start, time_end);                              \
        printf("%f\n", during);                                                \
    } while (0)

static void fork_benchmark(void)
{
    struct timespec ts1, ts2;
    
    set_all();

    clock_gettime(CLOCK_MONOTONIC, &ts1);
    test_info.child_pid = fork();
    switch (test_info.child_pid) {
    case -1:
        BUG_ON(1, "fork");
    case 0:
        /* child */
        unset_all();
        return;
    default:
        clock_gettime(CLOCK_MONOTONIC, &ts2);
        print_cputime(ts1, ts2);
        /* parent */
        unset_all();
    }
}

static void system_benchmark(void)
{
#define SYSTEM_CALL system("echo 1")
    pr_info("non allocate memory\n");
    time_check(SYSTEM_CALL);


    set_all();
    pr_info("allocate memory: %zu\n", test_info.mm.nr_mem);
    time_check(SYSTEM_CALL);
    unset_all();
#undef SYSTEM_CALL
}

int main(void)
{
    system_benchmark();
    pr_info("benchmark succeeded\n");

    return 0;
}
