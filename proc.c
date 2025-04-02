#include "elf.h"
#include "proc.h"
#include "fs/fs.h"
#include "lib/string.h"
#include "console.h"
#include "drivers/sheduler.h"

struct vm vm;
void trapret();
void swtch(void** oldstack, void* newstack);

void run_elf(const char* name) {
    struct stat statbuf;
    if (stat(name, &statbuf)) {
        printk("file not found\n");
        return;
    }

    // Создаем новую задачу для каждого процесса
    struct task* new_task = kalloc();
    if (!new_task) {
        printk("failed to allocate task\n");
        return;
    }

    new_task->pgdir = setupkvm();
    allocuvm(new_task->pgdir, USER_BASE, USER_BASE + statbuf.size);
    allocuvm(new_task->pgdir, USER_STACK_BASE - 8 * PGSIZE, USER_STACK_BASE);
    switchuvm(&new_task->tss, new_task->stack.bottom, new_task->pgdir);

    if (read_file(&statbuf, (void*)USER_BASE, 100 << 20) <= 0) {
        kfree(new_task);
        printk("failed to read file\n");
        return;
    }

    Elf32_Ehdr* hdr = (void*)USER_BASE;
    struct kstack* u = &new_task->stack;
    memset(u, 0, sizeof(*u));
    u->context.eip = (uint32_t)trapret;

    registers_t* tf = &u->trapframe;
    tf->eip = hdr->e_entry;
    tf->cs = (SEG_UCODE << 3) | DPL_USER;
    tf->ds = (SEG_UDATA << 3) | DPL_USER;
    tf->es = tf->ss = tf->fs = tf->gs = tf->ds;
    tf->eflags = FL_IF;
    tf->useresp = USER_STACK_BASE;
    add_to_sheduler(new_task); // Передаем новую задачу в планировщик
}

void run_ellf(const char* name) {
    struct stat statbuf;
    if (stat(name, &statbuf) != 0) {
        printk(name);
        printk(": file not found\n");
        return;
    }
    if (!vm.user_task) {
        vm.user_task = kalloc();
    }
    vm.user_task->pgdir = setupkvm();
    allocuvm(vm.user_task->pgdir, USER_BASE, USER_BASE + statbuf.size);
    allocuvm(vm.user_task->pgdir, USER_STACK_BASE - 2 * PGSIZE, USER_STACK_BASE);
    switchuvm(&vm.user_task->tss, vm.user_task->stack.bottom, vm.user_task->pgdir);

    if (read_file(&statbuf, (void*)USER_BASE, 100 << 20) <= 0) {
        printk(name);
        printk(": file not found\n");
        return;
    }
    Elf32_Ehdr* hdr = (void*)USER_BASE;

    struct kstack* u = &vm.user_task->stack;
    memset(u, 0, sizeof(*u));
    u->context.eip = (uint32_t)trapret;

    registers_t* tf = &u->trapframe;
    tf->eip = hdr->e_entry;
    tf->cs = (SEG_UCODE << 3) | DPL_USER;
    tf->ds = (SEG_UDATA << 3) | DPL_USER;
    tf->es = tf->ds;
    tf->fs = tf->ds;
    tf->gs = tf->ds;
    tf->ss = tf->ds;
    tf->eflags = FL_IF;
    tf->useresp = USER_STACK_BASE;

    // initialization done, now switch to the process
    swtch(&vm.kernel_thread, &u->context);

    // process has finished
}

_Noreturn void killproc() {
    struct task* t = tasks[current_task_idx].task; // Получить текущую задачу из планировщика
    void* task_stack;
    if (!t) {
        switchkvm();
        freevm(vm.user_task->pgdir);
    }
    else {
        // Удалить задачу из планировщика
        remove_from_scheduler(t);

        // Освободить ресурсы
        switchkvm();
        freevm(t->pgdir);
    }

    // Переключиться на ядро
    swtch(&task_stack, vm.kernel_thread);

    __builtin_unreachable();
}