#include "../syscall.h"

int main(void) {
    syscall(SYS_greet, 0);
    syscall(SYS_msleep, 0);
    syscall(SYS_greet, 0);
    syscall(SYS_msleep, 0);
    syscall(SYS_greet, 0);
    syscall(SYS_msleep, 0);
    int i = 0;
    while (1)
    {
        i++;
        if (i > 100000000) break;
    }
    syscall(SYS_greet, 0);
    syscall(SYS_msleep, 0);
    syscall(SYS_greet, 0);
    syscall(SYS_msleep, 0);
    syscall(SYS_greet, 0);
    syscall(SYS_msleep, 0);
    return 0;
}
