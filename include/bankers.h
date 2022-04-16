#ifndef BANKERS_H
#define BANKERS_H

#include <stdbool.h>
#include "resources.h"

bool* getFinishedArray();
bool bankersAlgorithm(ResourcesTable* rsc_tbl, int pid, int requested_resource);
bool checkIfSequenceSafe(bool* can_finish);
unsigned** getResourcesNeeded(ResourcesTable* rsc_tbl);
unsigned* getWork(unsigned* available_resources);

#endif
