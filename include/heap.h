#ifndef __REXOS__HEAP_H
#define __REXOS__HEAP_H
#include <common/types.h>

namespace rexos {
    struct MemoryChunk
    {
        MemoryChunk *next;
        MemoryChunk *prev;
        bool allocated;
        common::size_t size;
    };
    class MemoryManager {
        protected:
            MemoryChunk* first;
        public:
            // We want to call malloc from static functions,
            // so like with the interrupt handler, we'll have
            // activeMemoryManager which will be set to this 
            // in the constructor
            static MemoryManager* activeMemoryManager;
            MemoryManager(common::size_t start, common::size_t size);
            ~MemoryManager();

            void* malloc(common::size_t size);
            void free(void* ptr);
    };
}

void* operator new(unsigned size);
void* operator new[](unsigned size);

// placement new
void* operator new(unsigned size, void* ptr);
void* operator new[](unsigned size, void* ptr);

void operator delete(void* ptr);
void operator delete[](void* ptr);



#endif