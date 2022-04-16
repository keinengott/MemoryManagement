#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <unistd.h>

#include "shared_memory.h"

int getSharedMemoryId()
{
    int shmemid;
    shmemid = shmget(IPC_PRIVATE, getpagesize(), IPC_CREAT | S_IRUSR | S_IWUSR);

    if (shmemid == -1)
    {
        perror("shmget");
        exit(1);
    }
    
    return shmemid;
}

void* attachToMemory(int shmemid, unsigned readonly)
{
    void* p;
    int shmflag;

    if (readonly) shmflag = SHM_RDONLY;
    else {
        shmflag = 0;
    }

    p = (void*) shmat(shmemid, 0, shmflag);

    if (!p)
    {
        perror("shmat");
        exit(1);
    }

    return p;
}

void cleanMemory(int shmemid, void* p)
{
    if (shmdt(p) == -1)
    {
        perror("shmdt");
        exit(1);
    }

    if (shmctl(shmemid, IPC_RMID, 0) == 1)
    {
        perror("shmctl");
        exit(1);
    }
}
