#include "pit.h"
#include "proc.h"
#include "sheduler.h"
#include "../console.h"

struct task_info tasks[MAX_TASKS];
int num_tasks = 0;
int current_task_idx = -1;
volatile uint32_t sheduler_ticks = 0;
struct task reserved;

void shedule() {
    sheduler_ticks++;
    if (sheduler_ticks < 200) return;
    sheduler_ticks = 0;
    cli();
    if (num_tasks == 0) return;

    // Сохраняем контекст текущей задачи (если она активна)
    if (current_task_idx != -1 && tasks[current_task_idx].runned == 1) {
        if (tasks[current_task_idx].task) {
            switchuvm(&tasks[current_task_idx].task->tss, tasks[current_task_idx].task->stack.bottom, tasks[current_task_idx].task->pgdir);
            tasks[current_task_idx].task->stack.context.eip = reserved.stack.context.eip;
            tasks[current_task_idx].runned = 0;
            swtch((void**)&tasks[current_task_idx].task->stack.context, vm.kernel_thread);
        }
    }

    // Выбираем следующую задачу
    int next_idx = (current_task_idx + 1) % num_tasks;

    // Проверка на валидность следующей задачи
    if (!tasks[next_idx].task) {
        current_task_idx = -1;
        return;
    }
    // Восстанавливаем контекст новой задачи
    switchuvm(&tasks[next_idx].task->tss, tasks[next_idx].task->stack.bottom, tasks[next_idx].task->pgdir);
    current_task_idx = next_idx;
    tasks[next_idx].runned = 1;
    reserved.stack.context.eip = tasks[next_idx].task->stack.context.eip;
    swtch(&vm.kernel_thread, &tasks[next_idx].task->stack.context);
    sti();
}

void add_to_sheduler(struct task* t) {
    if (num_tasks >= MAX_TASKS) return;
    tasks[num_tasks].task = t;
    num_tasks++;
}

void init_sheduler() {
    add_timer_callback(shedule);
    run_elf("greet");
    run_elf("div0");
    run_elf("shout");
    run_elf("div0");
    run_elf("shout");
}

void remove_from_scheduler(struct task* t) {
    for (int i = 0; i < num_tasks; i++) {
        if (tasks[i].task == t) {
            // Сдвиг оставшихся задач
            for (int j = i; j < num_tasks - 1; j++) {
                tasks[j] = tasks[j + 1];
            }
            num_tasks--;
            // Корректировка индекса текущей задачи
            if (current_task_idx >= i) current_task_idx--;
            break;
        }
    }
}