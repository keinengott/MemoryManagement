/********************************************
 *
 * Bankers algorithm adopted from rosettacide.org/wiki/Banker%27s_algorithm
 *
 * *******************************************/

#include "bankers.h"
#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>

#define RSC_TYPES 20
#define MAX_RUNTIME_PROCS 18

bool bankersAlgorithm(ResourcesTable* rsc_tbl, int pid, int requested_resource)
{

    rsc_tbl->rsc_descs[requested_resource].allocated[pid]++;

    unsigned* available_resources = getResourcesAvailable(rsc_tbl);
    unsigned** needs = getResourcesNeeded(rsc_tbl);
    unsigned* work = getWork(available_resources);
    bool* can_finish = getFinishedArray();

    unsigned numFinishableProcs = 0;

    do {
        for (unsigned i = 1; i <= MAX_RUNTIME_PROCS; i++)
        {
            if (can_finish[i]) {
                continue;
            }
            for(unsigned j = 0; j < RSC_TYPES; j++)
            {
                if (needs[i][j] > available_resources[j]) {
                    can_finish[i] = 0;
                    break;
                }
            }
            if (can_finish[i])
            {
                for (unsigned j = 0; j < RSC_TYPES; j++) {
                    work[j] += rsc_tbl->rsc_descs[j].allocated[i];
                }
                numFinishableProcs++;
            }
        }

    } while (numFinishableProcs > 0);

    bool isSafe = checkIfSequenceSafe(can_finish);
    rsc_tbl->rsc_descs[requested_resource].allocated[pid]--;
    free(needs); free(available_resources); free(work); free(can_finish);
    return isSafe;
}

unsigned** getResourcesNeeded(ResourcesTable* rsc_tbl)
{
    unsigned** needs = initializeArray(MAX_RUNTIME_PROCS+1, RSC_TYPES);
    unsigned max_processes, allocated_processes;
    for (unsigned i = 1; i <= MAX_RUNTIME_PROCS; i++)
    {
        for (unsigned j = 0; j < RSC_TYPES; j++)
        {
            max_processes = rsc_tbl->max_claims[i];                   // Max number of resources for process i
            allocated_processes = rsc_tbl->rsc_descs[j].allocated[i]; // Number of allocated resources for process i
            needs[i][j] = max_processes - allocated_processes;
        }
    }
    return needs;
}

bool checkIfSequenceSafe(bool* can_finish)
{
    for (unsigned i = 1; i <= MAX_RUNTIME_PROCS; i++)
    {
        if (!can_finish[i]) return false;
    }
    return true;
}

unsigned* getWork(unsigned* available_resources)
{
    unsigned* work = malloc(sizeof(unsigned) * RSC_TYPES);
    for (unsigned i = 0; i < RSC_TYPES; i++) {
        work[i] = available_resources[i];
    }
    return work;
}

bool* getFinishedArray()
{
    bool* finishedArray = malloc(MAX_RUNTIME_PROCS+1);

    for (unsigned i = 1; i <= MAX_RUNTIME_PROCS; i++)
    {
        finishedArray[i] = 1;
    }
    return finishedArray;
}

