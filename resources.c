#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resources.h"
#include "helpers.h"

#define BUF_SIZE 2048

const unsigned maxClaims = 2;
const unsigned maxProcesses= 18;
const unsigned NumRscCls = 20;

void summarizeRun(ResourcesTable* rsc_tbl, FILE* fp)
{
    unsigned* total_resources = getTotalResourceArray(rsc_tbl);
    unsigned* allocated_resources = getAllocatedResourceArray(rsc_tbl);
    unsigned* available = getResourcesAvailable(rsc_tbl);

    char buffer[100];

    sprintf(buffer, "%48s", "Resource Summary\n");
    writeToLog(buffer, fp);
    writeToStdOut(buffer);

    writeCurrentRscState(total_resources, "Total Resources\n", fp);
    writeCurrentRscState(allocated_resources, "Allocated Resources\n", fp);
    writeCurrentRscState(available, "Available Resources\n", fp);

    free(total_resources);
    free(allocated_resources);
    free(available);
}

void writeCurrentRscState(unsigned* resources, char* title, FILE* fp)
{
    char buffer[BUF_SIZE];
    sprintf(buffer + strlen(buffer), "\n%s  ", title);
    for (int i = 0; i < NumRscCls; i++) sprintf(buffer + strlen(buffer),"R%-3d", i);

    sprintf(buffer + strlen(buffer),"\n  ");

    for (int i = 0; i < NumRscCls; i++) sprintf(buffer + strlen(buffer),"%-4d", resources[i]);

    sprintf(buffer + strlen(buffer),"\n");
    writeToLog(buffer, fp);
    writeToStdOut(buffer);
}

void writeAllocatedResourceTable(ResourcesTable* rsc_tbl, FILE* fp)
{
    char buffer[BUF_SIZE];
    sprintf(buffer,"\n");
    sprintf(buffer + strlen(buffer),"%61s", "Currently Allocated Resources\n");
    sprintf(buffer + strlen(buffer),"     ");
    // print titles
    for (int i = 0; i < NumRscCls; i++) sprintf(buffer + strlen(buffer),"R%-3d", i);

    sprintf(buffer + strlen(buffer),"\n");

    for (int i = 1; i <= maxProcesses; i++)
    {
        sprintf(buffer + strlen(buffer),"P%-4d", i-1);

        for (int j = 0; j < NumRscCls; j++) sprintf(buffer + strlen(buffer),"%-4d", rsc_tbl->rsc_descs[j].allocated[i]);

        sprintf(buffer + strlen(buffer),"\n");
    }

    sprintf(buffer + strlen(buffer),"\n");
    writeToLog(buffer, fp);
    writeToStdOut(buffer);
}

void populateResourceTable(ResourcesTable* rsc_tbl)
{
    for (int i = 0; i < NumRscCls; i++) rsc_tbl->rsc_descs[i] = getResource();
}

Resource getResource()
{
    Resource rsc_desc ={ .total = generateResourceCount(), };

    initAllocatedRscArray(rsc_desc.allocated);
    return rsc_desc;
}

unsigned generateResourceCount()
{
    unsigned retVal = (rand() % 10) + 1;
    return retVal;
}

void initAllocatedRscArray(unsigned* allocated)
{
    for (int i = 1; i <= maxProcesses; i++) allocated[i] = 0;

}

unsigned generateMaxResourceClaims()
{
    unsigned retVal = (rand() % maxClaims) + 1;
    return retVal;
}

void releaseResources(ResourcesTable* rsc_tbl, int pid) {
    for (unsigned i = 0; i < NumRscCls; i++) rsc_tbl->rsc_descs[i].allocated[pid] = 0;
}

unsigned* getCurrentlyAllocatedResources(int pid, ResourcesTable* rsc_tbl)
{
    // Returns an array of all resource classes that are currently allocated
    unsigned num_resources, num_resource_classes = 0, j = 0;
    num_resource_classes = getResourcesAllocatedToPIDs(pid, rsc_tbl);
    unsigned* allocated_resources = malloc(sizeof(unsigned) * num_resource_classes);

    for (unsigned i = 0; i < NumRscCls; i++)
    {
        num_resources = rsc_tbl->rsc_descs[i].allocated[pid];

        if (num_resources > 0) allocated_resources[j++] = i;
    }

    return allocated_resources;
}

unsigned getResourcesAllocatedToPIDs(int pid, ResourcesTable* rsc_tbl)
{
    unsigned num_resources, num_resource_classes = 0;

    for (unsigned i = 0; i < NumRscCls; i++)
    {
        num_resources = rsc_tbl->rsc_descs[i].allocated[pid];
        if (num_resources > 0) num_resource_classes++;
    }

    return num_resource_classes;
}

bool resourceAvailable(ResourcesTable* rsc_tbl, int requested_resource)
{
    unsigned* allocated_resources = getAllocatedResourceArray(rsc_tbl);
    unsigned currently_allocated = allocated_resources[requested_resource];
    unsigned total = rsc_tbl->rsc_descs[requested_resource].total;

    // resources unavailable
    if (currently_allocated == total) return false;

    free(allocated_resources);
    return true;
}

bool has_resource(int pid, ResourcesTable* rsc_tbl)
{
    unsigned num_resources = 0;

    for (unsigned i = 0; i < NumRscCls; i++)
    {
        num_resources = rsc_tbl->rsc_descs[i].allocated[pid];

        if (num_resources > 0) return true;
    }
    return false;
}


unsigned* getResourcesAvailable(ResourcesTable* rsc_tbl) {
    // Subtract to get the total resources available array
    unsigned* allocated_resources = getAllocatedResourceArray(rsc_tbl);
    unsigned* total_resources = getTotalResourceArray(rsc_tbl);
    unsigned* available_resources = malloc(sizeof(unsigned) * NumRscCls);

    for (unsigned i = 0; i < NumRscCls; i++) available_resources[i] = total_resources[i] - allocated_resources[i];
    
    free(allocated_resources);
    free(total_resources);
    
    return available_resources;
}

unsigned* getAllocatedResourceArray(ResourcesTable* rsc_tbl)
{
    unsigned* allocated_resources = malloc(sizeof(unsigned) * NumRscCls);

    for (unsigned i = 0; i < NumRscCls; i++) allocated_resources[i] = 0;

    for (unsigned i = 0; i < NumRscCls; i++)
    {
        for (unsigned j = 1; j <= maxProcesses; j++) allocated_resources[i] += rsc_tbl->rsc_descs[i].allocated[j];
    }

    return allocated_resources;
}

unsigned* getTotalResourceArray(ResourcesTable* rsc_tbl)
{
    unsigned* total_resources = malloc(sizeof(unsigned) * NumRscCls);

    for (unsigned i = 0; i < NumRscCls; i++) total_resources[i] = rsc_tbl->rsc_descs[i].total;

    return total_resources;
}

