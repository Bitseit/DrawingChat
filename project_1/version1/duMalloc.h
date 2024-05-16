#ifndef DUMALLOC_H
#define DUMALLOC_H
#define BEST_FIT 1
#define FIRST_FIT 0

// The interface for DU malloc and free
void duInitMalloc();
void* duMalloc(int size);
void duFree(void* ptr);

void duMemoryDump();

#endif