#include <heap.h>
using namespace rexos;
using namespace rexos::common;


// We want to call malloc from static functions,
// so like with the interrupt handler, we'll have
// activeMemoryManager which will be set to this 
// in the constructor
MemoryManager* MemoryManager::activeMemoryManager = 0;

MemoryManager::MemoryManager(common::size_t start, common::size_t size) {
    activeMemoryManager = this;
    // start is the offset of the data MemoryManager is going to handle. This
    // is where we put the first memory chunk. If the size is not sufficient,
    // we should protect ourselves in this situation since we'll be writing
    // outiside the allowed location
    if(size < sizeof(MemoryChunk)) {
        first = 0;
    } else {
        first = (MemoryChunk*)start;
        first->allocated = false;
        first->prev = 0;
        first->next = 0;
        first->size = size - sizeof(MemoryChunk);
    }
        
}
MemoryManager::~MemoryManager() {
    if(activeMemoryManager == this) {
        activeMemoryManager = 0;
    }
}

// When we try to allocate something, we will iterate through the list of
// chunks and choose one that's large enough
void* MemoryManager::malloc(common::size_t size) {
    MemoryChunk* result = 0;
    // Iterating the linked list
    for(MemoryChunk* chunk = first; chunk != 0 && result == 0; chunk = chunk->next)
       if(chunk->size >= size && !chunk->allocated)
           result = chunk;
    // If we haven't found anything the result is still 0
    if(result == 0)
        return 0;
    
    // Reaching here means the result is set to something
    
    // If the result chunk is large (ie. 30 MB) and requested chunk is smaller
    // (ie. 10 bytes), we set another chunk struct that's after the requested 
    // ptr whose size = result size - requested size. This chunk is unallocated.
    // If requested size + chunk struct's size > result size, we cannot split
    // the result memory
    // If you don't put +1, you'd only split the result chunk into requested
    // data + the header (containing the node struct) for the next chunk and
    // it would have size 0 which wouldn't make sense
    if(result->size >= size+sizeof(MemoryChunk) + 1) {
        // Splitting: The unallocated chunk starts after the requested space
        // which is ptr to result + (size of chunk struct + requested size)
        MemoryChunk* temp = (MemoryChunk*)((size_t)result + sizeof(MemoryChunk) + size);
        temp->allocated = false;
        temp->size = result->size - size - sizeof(MemoryChunk);
        // Linking temp to result node
        temp->prev = result;
        temp->next = result->next; // result->next is 0 here (usually)
        // Linking temp to the next node if it exists
        if(temp->next != 0)
            temp->next->prev = temp;
        
        result->size = size;
        // Linking result to temp
        result->next =  temp;
    }
    result->allocated = true;
    // Returning ptr to the usable space
    return (void*) ((size_t) result + sizeof(MemoryChunk));
}
void MemoryManager::free(void* ptr) {
    // Getting back to the header of the given memory space's ptr
    MemoryChunk* chunk = (MemoryChunk*)((size_t)ptr - sizeof(MemoryChunk));
    chunk->allocated = false;
    // If neighboring chunks are deallocated:
    // 1. Merge with the previous chunk first
    if(chunk->prev != 0 && !chunk->prev->allocated) {
        // Linking previous chunk to the next chunk removing current node
        chunk->prev->next = chunk->next;
        // Merging space
        chunk->prev->size += chunk->size + sizeof(MemoryChunk);
        // Linking previous chunk to next chunk if it exists
        if(chunk->next != 0)
            chunk->next->prev = chunk->prev;
        // Finally setting this chunk's ptr to the previous chunk's ptr
        chunk = chunk->prev;
    }
    // 2. Merge with the next chunk afterwards
    if(chunk->next != 0 && !chunk->next->allocated) {
        chunk->size += chunk->next->size + sizeof(MemoryChunk);
        chunk->next = chunk->next->next; // basically 0
        if(chunk->next != 0)
            chunk->next->prev = chunk;
    }
}

void* operator new(unsigned size) {
    if(rexos::MemoryManager::activeMemoryManager == 0)
        return 0;
    return rexos::MemoryManager::activeMemoryManager->malloc(size);
}
void* operator new[](unsigned size) {
    if(rexos::MemoryManager::activeMemoryManager == 0)
        return 0;
    return rexos::MemoryManager::activeMemoryManager->malloc(size);
}

void* operator new(unsigned size, void* ptr) {
    return ptr;
}
void* operator new[](unsigned size, void* ptr) {
    return ptr;
}



void operator delete(void* ptr) {
    if(rexos::MemoryManager::activeMemoryManager != 0)
        rexos::MemoryManager::activeMemoryManager->free(ptr);
}
void operator delete[](void* ptr) {
    if(rexos::MemoryManager::activeMemoryManager != 0)
        rexos::MemoryManager::activeMemoryManager->free(ptr);
}


