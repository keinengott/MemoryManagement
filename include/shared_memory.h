#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include "clock.h"

int getSharedMemoryId();
void* attachToMemory(int shmemid, unsigned readonly);
void cleanMemory(int shmemid, void* p);

#endif
