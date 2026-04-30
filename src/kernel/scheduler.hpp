#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <stdint.h>

#define MAX_TASKS 16
#define STACK_SIZE 32768

enum TaskState {
    TASK_EMPTY,
    TASK_READY,
    TASK_RUNNING,
    TASK_DEAD
};

struct TCB {
    int id;
    uint64_t rsp;
    TaskState state;
};

void scheduler_init();
int create_task(void (*entry)());

extern "C" uint64_t schedule(uint64_t current_rsp);

#endif
