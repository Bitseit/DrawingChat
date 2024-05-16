#include "duMalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// Defining heap size
#define HEAP_SIZE (128 * 8)
#define USED 0
#define FREE 1
#define ROWS 2             // number of rows (current and new heaps as the rows)
#define COLS HEAP_SIZE / 8 // number of columns (size and memory location as the columns)

void *managedList[HEAP_SIZE / 8] = {NULL}; // Managed List
int managedListSize = 0;                   // Size of the Managed List

// Structure for memory block header
typedef struct memoryBlockHeader
{
    int free;                       // 0 - used, 1 = free
    int size;                       // size of the reserved block
    int managedIndex;               // index of the block in the managed list
    struct memoryBlockHeader *next; // the next block in the integrated free list

} memoryBlockHeader;

// global variables
unsigned char heap[ROWS][COLS]; // 2d array heap

memoryBlockHeader *freeListHeaders[ROWS]; // 1d array of free block headers, 0 for current 1 for new
memoryBlockHeader *currrentHeader = NULL;
int currentHeapIndex = 0;

int allocationStrategy;

void duInitMalloc(int strategy)
{
    allocationStrategy = strategy;
    // initializing the memory of the heap to 0
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            heap[i][j] = 0;
        }
    }
    // initializing the entire heap as one large block
    memoryBlockHeader *currentBlock = (memoryBlockHeader *)heap[currentHeapIndex];
    currentBlock->size = HEAP_SIZE - sizeof(memoryBlockHeader);

    currentBlock->free = 1; // Initially, the whole heap is free
    currentBlock->next = NULL;
    freeListHeaders[currentHeapIndex] = currentBlock;
}

void printMemoryBlock(memoryBlockHeader *block)
{
    // Print the block
    printf("%s at %p, size %d\n", (block->free == USED) ? "Used" : "Free", block, block->size);
}

void printFreeList(int heapIndex)
{
    printf("\n");
    printf("Free List\n");
    memoryBlockHeader *currentBlock = (memoryBlockHeader *)heap[heapIndex]; // start from heap free list
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
    // should i include a if statement to check if the managedList is empty, do i print nil?
    printf("\nManagedList\n");
    for (int i = 0; i < managedListSize; i++)
    {
        printf("ManagedList[%d] = %p\n", i, managedList[i]);
    }
}

void duMemoryDump()
{
    printf("MEMORY DUMP\n");
    printf("Current heap (0/1 young):%d\n", currentHeapIndex);
    printf("Young Heap (only current one)\n");
    // Print memory block information for all blocks
    memoryBlockHeader *current = (memoryBlockHeader *)heap[currentHeapIndex];
    char free = 'a';
    char used = 'A';
    char string[HEAP_SIZE / 8 + 1];

    while (current < (memoryBlockHeader *)(heap[currentHeapIndex] + HEAP_SIZE))
    {
        printMemoryBlock(current);
        int blockSize = (current->size + sizeof(memoryBlockHeader)) / 8; // Number of characters to represent block
        int string_i = ((unsigned char *)current - (unsigned char *)heap[currentHeapIndex]) / 8;

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
    printFreeList(currentHeapIndex);

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
        currentBlock = freeListHeaders[currentHeapIndex];
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
        currentBlock = freeListHeaders[currentHeapIndex];
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
    // If no block found, return NULL
    if (currentBlock == NULL)
    {
        return NULL;
    }
    // Calculate the address of the new block
    memoryBlockHeader *newBlock = (memoryBlockHeader *)((unsigned char *)currentBlock + totalSize);
    newBlock->size = currentBlock->size - totalSize;
    newBlock->next = currentBlock->next;
    newBlock->free = FREE;
    // insert the new block into the free list
    if (prevBlock == NULL)
    {
        freeListHeaders[currentHeapIndex] = newBlock;
    }
    else
    {
        prevBlock->next = newBlock;
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
    memoryBlockHeader *currentBlock = freeListHeaders[currentHeapIndex];
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
        freeListHeaders[currentHeapIndex] = blockHeader;
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

void minorCollection()
{
    for (int i = 0; i < managedListSize; i++)
    {
        if (managedList[i] != NULL)
        {
            memoryBlockHeader *currentBlock = (memoryBlockHeader *)((unsigned char *) managedList[i] - sizeof(memoryBlockHeader));
            void *pointerToMoveTo = currentBlock;
            void *pointerToFromFrom = managedList[i];
            size_t numberOfBytesToMoveTo = currentBlock->size;
            // if slot is not free/null, the live pointer points to a new address
            memcpy(pointerToMoveTo, pointerToFromFrom, numberOfBytesToMoveTo);
        }
    }

    // reset the managed list
    for (int i = 0; i < managedListSize; i++)
    {
        managedList[i] = NULL;
    }
    managedListSize = 0;
    // reset the free list
    memoryBlockHeader *currentBlock = freeListHeaders[currentHeapIndex];
    memoryBlockHeader *prevBlock = NULL;
    while (currentBlock != NULL)
    {
        prevBlock = currentBlock;
        currentBlock = currentBlock->next;
    }
    freeListHeaders[currentHeapIndex] = prevBlock;
    // swap the heaps
    currentHeapIndex = (currentHeapIndex + 1) % 2;
    // reset the free list
    if (heap[currentHeapIndex] != NULL)
    {
        freeListHeaders[currentHeapIndex] = (memoryBlockHeader *)heap[currentHeapIndex];
        freeListHeaders[currentHeapIndex]->size = HEAP_SIZE - sizeof(memoryBlockHeader);
        freeListHeaders[currentHeapIndex]->free = 1;
        freeListHeaders[currentHeapIndex]->next = NULL;
    }
}

// void minorCollection() {
//     // Create a new heap
//     currentHeapIndex = 1 - currentHeapIndex;
//     memoryBlockHeader *currentBlock = (memoryBlockHeader *)heap[currentHeapIndex];
//     currentBlock->size = HEAP_SIZE - sizeof(memoryBlockHeader);
//     currentBlock->free = 1; // Initially, the whole heap is free
//     currentBlock->next = NULL;
//     freeListHeaders[currentHeapIndex] = currentBlock;
//     // Traverse the old heap and copy live blocks to the new heap
//     memoryBlockHeader *oldBlock = (memoryBlockHeader *)heap[1 - currentHeapIndex];
//     memoryBlockHeader *prevBlock = NULL;
//     while (oldBlock != NULL)
//     {
//         if (oldBlock->free == USED)
//         {
//             // Copy the block to the new heap
//             memccpy((unsigned char *)currentBlock + sizeof(memoryBlockHeader), (unsigned char *)oldBlock + sizeof(memoryBlockHeader), 1, oldBlock->size);
//         }
//         oldBlock = (memoryBlockHeader *)((unsigned char *)oldBlock + sizeof(memoryBlockHeader) + oldBlock->size);
//     }
//     // Free the old heap
//     freeListHeaders[1 - currentHeapIndex] = NULL;
//     // Reset the managed list
//     for (int i = 0; i < HEAP_SIZE / 8; i++)
//     {
//         managedList[i] = NULL;
//     }
//     managedListSize = 0;
// }