#include <multitasking.h>
#include <common/types.h>

using namespace rexos;
using namespace rexos::common;

Task::Task(GlobalDescriptorTable* gdt, void entrypoint()){
    // cpustate is the ptr to the task and for a new task the ptr is after the
    // previous task. so entrypoint = start of stack + size of stack - size of
    // entry
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate->eax = 0;   // Accumulator register
    cpustate->ebx = 0;   // Base register
    cpustate->ecx = 0;   // Counting register
    cpustate->edx = 0;   // Data register

    cpustate->esi = 0;   // Stack index
    cpustate->edi = 0;   // Data index
    cpustate->ebp = 0;   // Stack base ptr

    // Values pushed by default (same as interruptstubs)
    /*
    cpustate->gs = 0;  
    cpustate->fs = 0;    
    cpustate->es = 0;
    cpustate->ds = 0;
    */    
    // cpustate->error = 0; // Error code
        
        /* Values pushed by the processor (Kernel spcace) */
    cpustate->eip = (uint32_t) entrypoint;   // Set to the entry point
    cpustate->cs = gdt->CodeSegmentOffset();    // Code segment
    cpustate->eflags = 0x202;
    // cpustate->esp = 0;   // Required if we're dealing w/ userspace and
    // different security levels/rings
    // cpustate->ss = 0; // stack segment is also not using while not dealing with


}
Task::~Task(){}

TaskManager::TaskManager() {
    numTasks = 0;   // no task in the beginning
    currentTask = -1;   // nothing legal inside the array
}
TaskManager::~TaskManager() {}
bool TaskManager::AddTask(Task* task) {
    if(numTasks >= 256)
        return false;
    tasks[numTasks++] = task; // dereference to assign value to the ptr
    // otherwise it'd be assigning ptr to ptr which isn't expected
    return true;
}
CPUState* TaskManager::Schedule(CPUState* cpustate) {
    if(numTasks <=0) {
        return cpustate;
    }
    if(currentTask >= 0) {
        tasks[currentTask]->cpustate = cpustate;
    }
    // Round robin: if currentTask exceeds numTasks, we start at the beginning
    if(++currentTask >= numTasks)
        currentTask %= numTasks;
    return tasks[currentTask]->cpustate;
}