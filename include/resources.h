#ifndef RESOURCES_H
#define RESOURCES_H

#include <stdbool.h>
#include <stdio.h>

struct Resource {
    unsigned total;
    unsigned allocated[19];
};
typedef struct Resource Resource; // because c++ is better

struct ResourcesTable {
    Resource rsc_descs[20];
    unsigned max_claims[19];
};
typedef struct ResourcesTable ResourcesTable;

bool has_resource(int pid, ResourcesTable* rsc_tbl);
bool resourceAvailable(ResourcesTable* rsc_tbl, int requested_resource);
void populateResourceTable(ResourcesTable* rsc_tbl);
void writeCurrentRscState(unsigned* resources, char* title, FILE* fp);
void summarizeRun(ResourcesTable* rsc_tbl, FILE* fp);
void initAllocatedRscArray(unsigned* allocated);
void releaseResources(ResourcesTable* rsc_tbl, int pid);
void writeAllocatedResourceTable(ResourcesTable* rsc_tbl, FILE* fp);
unsigned generateResourceCount();
unsigned generateMaxResourceClaims();
unsigned* getCurrentlyAllocatedResources(int pid, ResourcesTable* rsc_tbl);
unsigned getResourcesAllocatedToPIDs(int pid, ResourcesTable* rsc_tbl);
unsigned* getAllocatedResourceArray(ResourcesTable* rsc_tbl);
unsigned* getResourcesAvailable(ResourcesTable* rsc_tbl);
unsigned* getTotalResourceArray(ResourcesTable* rsc_tbl);
Resource getResource();

#endif
