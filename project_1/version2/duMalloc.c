#include "duMalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
// Defining heap size
#define HEAP_SIZE (128 * 8)
#define USED 0
#define FREE 1
void *managedList[HEAP_SIZE / 8] = {NULL}; // Managed List
int managedListSize = 0;                   // Size of the Managed List ?????????????
// Structure for memory block header
typedef struct memoryBlockHeader
{
    int free; // 0 - used, 1 = free
    int size; // size of the reserved block
    int managedIndex;
    struct memoryBlockHeader *next; // the next block in the integrated free list

} memoryBlockHeader;

// global variables
unsigned char heap[HEAP_SIZE];
memoryBlockHeader *freeListHead;
int allocationStrategy;

void duInitMalloc(int strategy)
{
    allocationStrategy = strategy;
    // initializing the memory of the heap to 0
    for (int i = 0; i < HEAP_SIZE; i++)
    {
        heap[i] = 0;
    }
    // initializing the entire heap as one large block
    memoryBlockHeader *currentBlock = (memoryBlockHeader *)heap;
    currentBlock->size = HEAP_SIZE - sizeof(memoryBlockHeader);

    currentBlock->free = 1; // Initially, the whole heap is free
    currentBlock->next = NULL;
    freeListHead = currentBlock;
}

void printMemoryBlock(memoryBlockHeader *block)
{
    // Print the block
    printf("%s at %p, size %d\n", (block->free == USED) ? "Used" : "Free", block, block->size);
}

void printFreeList()
{
    printf("\n");
    printf("Free List\n");
    memoryBlockHeader *currentBlock = freeListHead;
    while (currentBlock != NULL)
    {
        printf("Block at %p, size %d\n", currentBlock, currentBlock->size);
        currentBlock = currentBlock->next;
    }
}

void duManagedInitMalloc(int searchType)
{
    // Call the original initialization function
    duInitMalloc(searchType);
    // Initialize Managed List slots and size
    for (int i = 0; i < HEAP_SIZE / 8; i++)
    {
        managedList[i] = NULL;
    }
    managedListSize = 0;
}

void printManagedList()
{
    printf("\nManagedList\n");
    for (int i = 0; i < managedListSize; i++)
    {
        printf("ManagedList[%d] = %p\n", i, managedList[i]);
    }
}

void duMemoryDump()
{
    printf("MEMORY DUMP\n");
    // Print memory block information for all blocks
    memoryBlockHeader *current = (memoryBlockHeader *)heap;
    char free = 'a';
    char used = 'A';
    char string[HEAP_SIZE / 8 + 1];

    while (current < (memoryBlockHeader *)(heap + HEAP_SIZE))
    {
        printMemoryBlock(current);
        int blockSize = (current->size + sizeof(memoryBlockHeader)) / 8; // Number of characters to represent block
        int string_i = ((unsigned char *)current - heap) / 8;

        if (current->free == FREE)
        {
            for (int i = string_i; i < string_i + blockSize; i++)
            {
                string[i] = free;
            }
            free++;
        }
        else
        {
            for (int i = string_i; i < string_i + blockSize; i++)
            {
                string[i] = used;
            }
            used++;
        }
        current = (memoryBlockHeader *)((unsigned char *)current + sizeof(memoryBlockHeader) + (current->size));
    }
    string[HEAP_SIZE / 8] = '\0';
    // Print graphical representation of memory blocks
    printf("Memory Block\n");
    printf("%s\n", string);
    // Print free list
    printFreeList(freeListHead);

    printManagedList();
}

void *duMalloc(int size)
{
    // Calculate the size of the block to allocate
    // Round up to nearest multiple of 8
    int blockSize = (size + 7) & ~7;
    int totalSize = blockSize + sizeof(memoryBlockHeader);
    memoryBlockHeader *currentBlock;
    memoryBlockHeader *prevBlock;
    if (allocationStrategy == FIRST_FIT)
    {
        // Traverse free list to find first block that fits
        currentBlock = freeListHead;
        prevBlock = NULL;
        // Find the first block that fits
        while (currentBlock != NULL && currentBlock->size < totalSize)
        {
            prevBlock = currentBlock;
            currentBlock = currentBlock->next;
        }
    }
    else if (allocationStrategy == BEST_FIT)
    {
        currentBlock = freeListHead;
        prevBlock = NULL;
        memoryBlockHeader *bestBlock = NULL;
        memoryBlockHeader *prevBestBlock = NULL;
        while (currentBlock != NULL)
        {
            if (currentBlock->size >= totalSize)
            {
                if (bestBlock == NULL || currentBlock->size < bestBlock->size)
                {
                    bestBlock = currentBlock;
                    prevBestBlock = prevBlock;
                }
            }
            prevBlock = currentBlock;
            currentBlock = currentBlock->next;
        }
        currentBlock = bestBlock;
        prevBlock = prevBestBlock;
    }
    else
    {
        printf("Invalid allocation strategy\n");
        exit(1);
    }
    // If no block found, returnw block
    memoryBlockHeader *newBlock = (memoryBlockHeader *)((unsigned char *)currentBlock + totalSize);
    newBlock->size = currentBlock->size - totalSize;
    newBlock->next = currentBlock->next;
    newBlock->free = FREE;
    // insert the new block into the free list
    if (prevBlock == NULL)
    {
        freeListHead = newBlock;
    }
    else
    {
        prevBlock->next;
    }
    // Set the size of the block to return
    currentBlock->size = blockSize;
    currentBlock->next = NULL;
    currentBlock->free = USED;
    // Return the address of the block
    return (unsigned char *)currentBlock + sizeof(memoryBlockHeader);
}

void duFree(void *ptr)
{
    // Calculate block header pointer
    memoryBlockHeader *blockHeader = (memoryBlockHeader *)((unsigned char *)ptr - sizeof(memoryBlockHeader));
    // Traverse free list to find correct location to splice in the block
    memoryBlockHeader *currentBlock = freeListHead;
    memoryBlockHeader *prevBlock = NULL;
    // Find the first block that is greater than the block to free
    while (currentBlock != NULL && currentBlock < blockHeader)
    {
        prevBlock = currentBlock;
        currentBlock = currentBlock->next;
    }

    blockHeader->next = currentBlock;
    blockHeader->free = FREE;
    // Splice in the block
    if (prevBlock == NULL)
    {
        // Block becomes new head of free list
        freeListHead;
    }
    else
    {
        prevBlock->next = blockHeader;
    }
}

void **duManagedMalloc(int size)
{
    // Call the original malloc function
    void *ptr = duMalloc(size);
    if (ptr == NULL)
    {
        return NULL; // Allocation failed
    }
    // Add an entry into the Managed List
    if (managedListSize < HEAP_SIZE / 8)
    {
        managedList[managedListSize] = ptr;
        // Set the managed index in the heap block
        memoryBlockHeader *blockHeader = (memoryBlockHeader *)((unsigned char *)ptr - sizeof(memoryBlockHeader));
        blockHeader->managedIndex = managedListSize;
        managedListSize++;
    }
    else
    {
        // Managed List is full, handle error or resize array
        // For now, just return NULL
        return NULL;
    }
    // Return the pointer to the Managed List slot
    return &managedList[managedListSize - 1];
}

void duManagedFree(void **mptr)
{
    // Check if the Managed List slot is NULL
    if (*mptr == NULL)
    {
        return; // Pointer has already been freed
    }
    // Call the original free function to remove the block from the heap
    duFree(*mptr);
    // Null out the address at the slot in the Managed List
    *mptr = NULL;
}