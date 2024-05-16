#ifndef DUMALLOC_H
#define DUMALLOC_H
#define FIRST_FIT 0
#define BEST_FIT 1
#define Managed(p) (*p)
#define Managed_t(t) t*
// The interface for DU malloc and free
void duInitMalloc(int strategy);
void* duMalloc(int size);
void duFree(void* ptr);
void duMemoryDump();
void** duManagedMalloc(int size);
void duManagedInitMalloc(int searchType);
void duManagedFree(void** mptr);
void minorCollection();
#endif






