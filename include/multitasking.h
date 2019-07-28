#ifndef __REXOS__MULTITASKING_H
#define __REXOS__MULTITASKING_H
#include <common/types.h>
#include <gdt.h>

namespace rexos {
    struct CPUState {
        /* Values pushed by the user (Userspace) */
        common::uint32_t eax;   // Accumulator register
        common::uint32_t ebx;   // Base register
        common::uint32_t ecx;   // Counting register
        common::uint32_t edx;   // Data register

        common::uint32_t esi;   // Stack index
        common::uint32_t edi;   // Data index
        common::uint32_t ebp;   // Stack base ptr

        // Values pushed by default (same as interruptstubs)
        /*
        common::uint32_t gs;  
        common::uint32_t fs;    
        common::uint32_t es;
        common::uint32_t ds;
        */
        common::uint32_t error; // Error code
        
        /* Values pushed by the processor (Kernel space) */
        common::uint32_t eip;   // Instruction ptr
        common::uint32_t cs;    // Code segment
        common::uint32_t eflags;
        common::uint32_t esp;   // Stack ptr
        common::uint32_t ss;    // Stack segment
    } __attribute__((packed));  
    class Task {
        // (M.1.) Task manager may need access to Task's private values
        friend class TaskManager;
        private:
        // (M.2.) Allocating stacks
        common::uint8_t stack[4096]; // 4 KiB
        // (M.3.) A struct (DS) will be laid on the stack for CPU and user pushed data
        CPUState* cpustate; // Ptr to the stack is the ptr to this struct
        public:
        // (M.4.) In the ctor, Task will have to talk to the GDT
        // and a func ptr to func that's supposed to be executed
        Task(GlobalDescriptorTable* gdt, void entrypoint());
        ~Task();
    };
    class TaskManager {
        private:
        // (M.4.) An array of above Tasks
        Task* tasks[256];
        int numTasks;   // # of tasks
        // (M.5.) Index of currently active task
        int currentTask;    // taskTo store the stack ptr of previous task
        public:
        TaskManager();
        ~TaskManager();
        bool AddTask(Task* task);
        // A method for Round Robin scheduling: a linear array of task ptr
        // Every time the timer interrupt occurs when we are executing a task,
        // we go to the next task and so on.
        CPUState* Schedule(CPUState* cpustate);
    };

}

#endif