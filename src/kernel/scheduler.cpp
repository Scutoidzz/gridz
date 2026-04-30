#include "scheduler.hpp"

extern "C" void serial_print(const char* s);

static TCB tasks[MAX_TASKS];
static uint8_t task_stacks[MAX_TASKS][STACK_SIZE] __attribute__((aligned(16)));
static int current_task = -1;

extern volatile uint32_t timer_ticks;

void scheduler_init() {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_EMPTY;
    }
    tasks[0].id = 0;
    tasks[0].state = TASK_RUNNING;
    current_task = 0;
}

int create_task(void (*entry)()) {
    int task_id = -1;
    for (int i = 1; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_EMPTY || tasks[i].state == TASK_DEAD) {
            task_id = i;
            break;
        }
    }

    if (task_id == -1) return -1;

    tasks[task_id].id = task_id;
    uint64_t* stack_top = (uint64_t*)&task_stacks[task_id][STACK_SIZE];

    *(--stack_top) = 0x10;                     // SS (Kernel Data)
    *(--stack_top) = (uint64_t)&task_stacks[task_id][STACK_SIZE]; // RSP
    *(--stack_top) = 0x202;                    // RFLAGS
    *(--stack_top) = 0x08;                     // CS (Kernel Code)
    *(--stack_top) = (uint64_t)entry;          // RIP
    
    // Push general purpose registers (0 for init)
    for (int i = 0; i < 15; i++) {
        *(--stack_top) = 0;
    }

    tasks[task_id].rsp = (uint64_t)stack_top;
    tasks[task_id].state = TASK_READY;

    return task_id;
}

extern "C" uint64_t schedule(uint64_t current_rsp) {
    timer_ticks++;

    if (current_task == -1) return current_rsp;

    if (tasks[current_task].state == TASK_RUNNING) {
        tasks[current_task].rsp = current_rsp;
        tasks[current_task].state = TASK_READY;
    }

    int next_task = current_task;
    for (int i = 0; i < MAX_TASKS; i++) {
        next_task = (next_task + 1) % MAX_TASKS;
        if (tasks[next_task].state == TASK_READY) {
            break;
        }
    }

    if (tasks[next_task].state == TASK_READY) {
        tasks[next_task].state = TASK_RUNNING;
        current_task = next_task;
    }

    return tasks[current_task].rsp;
}
