#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include "debug.h"

#define MEM_SIZE 128
struct mm {
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
   test_info.mm.private = mmap(0, MEM_SIZE, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANON, 0, 0);
    if (!test_info.mm.private)
        return -ENOMEM;
   test_info.mm.shared = mmap(0, MEM_SIZE, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_SHARED, 0, 0);
    if (!test_info.mm.shared)
        return -ENOMEM;

    return 0;
}

static void unset_mem(void)
{
    munmap(test_info.mm.private, MEM_SIZE);
    munmap(test_info.mm.shared, MEM_SIZE);
}

int main(void)
{

    set_mem();

    test_info.child_pid = fork();
    switch (test_info.child_pid) {
        case -1:
            BUG_ON(1, "fork");
        case 0:
            /* child */
            unset_mem();
            return 0;
        default:
            /* parent */
            unset_mem();
    }

    pr_info("succeeded\n");

    return 0;
}
