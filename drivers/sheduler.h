#pragma once
#include "proc.h"

// Полное определение структуры task_info
struct task_info {
    struct task* task;
    uint32_t runned;
    void* task_stack;
    struct context* context;
};

// Объявления внешних переменных
extern int current_task_idx;
extern struct task_info tasks[MAX_TASKS];
extern int num_tasks;

// Прототипы функций
void shedule();
void add_to_sheduler(struct task* t);
void init_sheduler();
void remove_from_scheduler(struct task* t);