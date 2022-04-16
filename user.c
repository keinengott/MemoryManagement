#include <locale.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>

#include "clock.h"
#include "global_constants.h"
#include "helpers.h"
#include "message_queue.h"
#include "resources.h"
#include "shared_memory.h"
#include "main_memory.h"

void bindSignalHandlers();
void sigtermHandler(int sig);
void requestRsc(int rscMsgBoxId, int pid, ResourcesTable* rsc_tbl);
void releaseRsc(int rscMsgBoxId, int pid, ResourcesTable* rsc_tbl);
void notifyParentOfTerminiation(int rscMsgBoxId, int pid);
void createRecourceMessage(char* mtext, int pid, ResourcesTable* rsc_tbl);

bool askForTermination();
bool askForRelease();

unsigned getChance();
unsigned releaseRandomResourceAtIndex(int pid, ResourcesTable* rsc_tbl);

Clock getRandomReleaseTime(Clock sysclock);

const unsigned TERMINATE_CHANCE = 25;
const unsigned CHANCE_RELEASE = 50;

int main (int argc, char *argv[])
{
    bindSignalHandlers();
    srand(time(NULL) ^ getpid());
    setlocale(LC_NUMERIC, "");

    // Get shared memory ids from argv
    int simClockID = atoi(argv[SYSCLOCK_ID_IDX]);
    int tblID = atoi(argv[RSC_TBL_ID_IDX]);
    int rscMsgBoxId = atoi(argv[RSC_MSGBX_ID_IDX]);
    int pid = atoi(argv[PID_IDX]);

    // Attach to shared memory
    ResourcesTable* rsc_tbl = attachToMemory(tblID, 0);
    Clock* sysclock = attachToMemory(simClockID, 1);

    Clock ttr = getRandomReleaseTime(*sysclock); // generate random release request time

    while(true)
    {
        if (clocksAreEqual(*sysclock, ttr) < 0) continue;

        // Time to request/release a resource 
        if (!has_resource(pid, rsc_tbl))
        {
            requestRsc(rscMsgBoxId, pid, rsc_tbl);

            if (askForTermination())
            {
                notifyParentOfTerminiation(rscMsgBoxId, pid);
                break;
            }
        }
        else
        {
            // roll the die on release
            if (askForRelease()) releaseRsc(rscMsgBoxId, pid, rsc_tbl);
            else
            {
                requestRsc(rscMsgBoxId, pid, rsc_tbl);

                if (askForTermination())
                {
                    notifyParentOfTerminiation(rscMsgBoxId, pid);
                    break;
                }
            }
        }
        // Generate new release time
        ttr = getRandomReleaseTime(*sysclock);
    } 

    return 0;  
}

void notifyParentOfTerminiation(int rscMsgBoxId, int pid)
{
    msgbuf rsc_msg_box;
    sprintf(rsc_msg_box.mtext, "%d,TERM,0", pid);
    sendMsg(rscMsgBoxId, &rsc_msg_box, pid);
}

void releaseRsc(int rscMsgBoxId, int pid, ResourcesTable* rsc_tbl)
{
    msgbuf rsc_msg_box;
    unsigned releasedResource = releaseRandomResourceAtIndex(pid, rsc_tbl);
    sprintf(rsc_msg_box.mtext, "%d,RLS,%d", pid, releasedResource);
    sendMsg(rscMsgBoxId, &rsc_msg_box, pid);
    receiveMsg(rscMsgBoxId, &rsc_msg_box, pid+MAX_RUNTIME_PROCS);
}

void requestRsc(int rscMsgBoxId, int pid, ResourcesTable* rsc_tbl)
{
    msgbuf rsc_msg_box;
    createRecourceMessage(rsc_msg_box.mtext, pid, rsc_tbl);
    sendMsg(rscMsgBoxId, &rsc_msg_box, pid);
    receiveMsg(rscMsgBoxId, &rsc_msg_box, pid+MAX_RUNTIME_PROCS);
}

// randomly select a resource to release
unsigned releaseRandomResourceAtIndex(int pid, ResourcesTable* rsc_tbl)
{
    unsigned* allocated_resources = getCurrentlyAllocatedResources(pid, rsc_tbl);
    unsigned size = getResourcesAllocatedToPIDs(pid, rsc_tbl);
    unsigned random_idx = rand() % size;
    unsigned releasedResource = allocated_resources[random_idx];
    free(allocated_resources);
    return releasedResource;
}

void createRecourceMessage(char* mtext, int pid, ResourcesTable* rsc_tbl)
{
    unsigned resource_to_request, num_currently_allocated, max_claims;
    do {
        resource_to_request = rand() % RSC_TYPES;
        num_currently_allocated = rsc_tbl->rsc_descs[resource_to_request].allocated[pid];
        max_claims = rsc_tbl->max_claims[pid];
    } while (num_currently_allocated == max_claims);
    // We currently do not have more of this resource allocated then our max claims limits
    sprintf(mtext, "%d,REQ,%d", pid, resource_to_request);
}

bool askForRelease()
{
    return rollTheDice(CHANCE_RELEASE);
}

bool askForTermination()
{
    return rollTheDice(TERMINATE_CHANCE);
}

Clock getRandomReleaseTime(Clock sysclock)
{
    unsigned nanoseconds = (rand() % (700000)) + 100000;
    Clock releaseRequestTime = sysclock;
    incrementClock_ns(&releaseRequestTime , nanoseconds);
    return releaseRequestTime ;
}

unsigned getChance()
{
    unsigned retVal = (rand() % 99) + 1;
    return retVal;
}

void bindSignalHandlers()
{
    struct sigaction act;
    act.sa_handler = sigtermHandler; // handler function
    sigemptyset(&act.sa_mask);      // Only block this signal
    act.sa_flags = 0;
    if (sigaction(SIGTERM, &act, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
}

void sigtermHandler(int sig)
{
    _exit(0);
}

