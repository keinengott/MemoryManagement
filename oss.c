#include <getopt.h>
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "bankers.h"
#include "global_constants.h"
#include "helpers.h"
#include "message_queue.h"
#include "queue.h"
#include "resources.h"
#include "shared_memory.h"
#include "main_memory.h"

typedef struct timeval timeval;
typedef struct msqid_ds msqid_ds;
struct message
{
    int pid;
    char txt[10];
    int resource;
};
typedef struct message message;

const unsigned MAX_TIME_BETWEEN_EXECS = 500000000; // nanoseconds
const unsigned TOTAL_RUNTIME = 5;       // Max time to run oss
const unsigned MAX_REAL_RUNTIME = 5;      // Max real time to run oss

bool VERBOSE_COUT = true;
void printUsage();
void bindSignalHandlers();
void sigintHandler(int sig);
void sigalrmHandler(int sig);
void cleanAndEscape();
void unblockBlockedProcs(int resource, msgbuf rsc_msg_box, Clock* time_blocked);
void doAFork(char** execv_arr, unsigned pid);
void waitOnChildren();
void printBlockedProcessQueue();
void printResourceStatus(unsigned numRequests);
unsigned get_nanoseconds();
unsigned getNextPidIndex();
unsigned ossGetWork();
unsigned resourcesGranted = 0, bankersCount = 0;
float RatioOfResourcesUsed(unsigned numRequests);
Clock getRandomForkTime(Clock sysclock);
message processUserMessage(char* aMsg);

int simClockId, rscTableId, rscMsgBoxId;
bool exiting = false;
Clock* sysclock;
Clock* blockClock; // clock used just to keep track of total time in a blocked state
ResourcesTable* rsc_tbl;
pid_t* childpids;
FILE* fp; // logfile oss.log
Queue* blocked;

int main (int argc, char* argv[])
{
    bool verbose = false;
    set_timer(MAX_REAL_RUNTIME); // SIGALRM time for max real time to run
    bindSignalHandlers();
    setlocale(LC_NUMERIC, "");
    srand(time(NULL) ^ getpid());
    int option;

    while ((option = getopt (argc, argv, "chv")) != -1)
    switch (option) {
        case 'c':
            VERBOSE_COUT = false;
            break;
        case 'h':
            printUsage();
            break;
        case 'v':
            verbose = true;
            break;
        default:
            printUsage();
    }

    unsigned pid = 0, proc_cnt = 0, num_messages = 0;
    char buffer[255];                    // classic buffer to stream to logfile or cout
    timeval tv_start, tv_stop;           // for time
    unsigned realElapsedTime = 0;        // real elapsed time in seconds
    gettimeofday(&tv_start, NULL);       // set oss start time now
    blocked = malloc(sizeof(Queue) * RSC_TYPES); // Blocked Queue array

    for (unsigned i = 0; i < RSC_TYPES; i++)
    {
        Queue bq;
        queueInit(&bq);
        blocked[i] = bq;
    }


    Clock forkTime = getClock();    // Holds time to schedule new process

    char* execv_arr[EXECV_SIZE];                
    execv_arr[0] = "./user";
    execv_arr[EXECV_SIZE - 1] = NULL;
    
    // Shared logical clock
    simClockId = getSharedMemoryId();
    sysclock = (Clock*) attachToMemory(simClockId, 0);
    reset_clock(sysclock);
    // Shared Resource Table 
    rscTableId = getSharedMemoryId();
    rsc_tbl = (ResourcesTable*) attachToMemory(rscTableId, 0);
    populateResourceTable(rsc_tbl);
    // Shared resource message box for user processes to request/release resources 
    rscMsgBoxId = getMsgQueue();
    msgbuf rsc_msg_box;

    // childpid array
    childpids = malloc(sizeof(pid_t) * (MAX_RUNTIME_PROCS + 1));

    for (unsigned i = 1; i <= MAX_RUNTIME_PROCS; i++) childpids[i] = 0;

    // Open log file with w, such that it is rewritten each run
    if ((fp = fopen("./oss.log", "w")) == NULL)
    {
        perror("fopen");
        exit(1);
    }

    // Get time to fork first user process
    forkTime = getRandomForkTime(*sysclock);
    
    // Set time to fork time
    *sysclock = forkTime;

    message msg;
    msqid_ds msgq_ds;
    msgq_ds.msg_qnum = 0;
    bool granted, rscAvailable;
    int resource;
    // for stat print
    unsigned numRequests = 0;

    Clock time_blocked[MAX_RUNTIME_PROCS+1];

    for (unsigned i = 1; i <= MAX_RUNTIME_PROCS; i++)
    {
        time_blocked[i].seconds = 0;
        time_blocked[i].nanoseconds = 0;
    }

    blockClock = malloc(sizeof(Clock));
    reset_clock(blockClock);

    while ( realElapsedTime < TOTAL_RUNTIME )
    {
        // fork when it is time
        if (clocksAreEqual(*sysclock, forkTime) >= 0 && proc_cnt < MAX_RUNTIME_PROCS)
        {
            pid = getNextPidIndex();
            rsc_tbl->max_claims[pid] = generateMaxResourceClaims();
            doAFork(execv_arr, pid);
            printf("num procs: %d \n", proc_cnt);
            proc_cnt++;
            
            if (verbose)
            {
                sprintf(buffer, "oss: Creating new process: P%d at time %ld:%'ld\n",
                    pid, sysclock->seconds, sysclock->nanoseconds);
                if (VERBOSE_COUT) writeToStdOut(buffer);
                writeToLog(buffer, fp);
            }

            forkTime = getRandomForkTime(*sysclock);
        }

        msgctl(rscMsgBoxId, IPC_STAT, &msgq_ds);
        num_messages = msgq_ds.msg_qnum;
        if(num_messages > 0)
        printf("num messages%d\n", num_messages);
        if (num_messages > 0)
        {
            receiveMsg(rscMsgBoxId, &rsc_msg_box, 0);

            if (strlen(rsc_msg_box.mtext) < 5) continue;

            msg = processUserMessage(rsc_msg_box.mtext);
            resource = msg.resource;
            pid = msg.pid;

            if (strcmp(msg.txt, "REQ") == 0)
            {
                if (verbose)
                {
                    sprintf(buffer, "oss: P%d is requesting R%d at time %ld:%'ld\n",
                        pid, resource, sysclock->seconds, sysclock->nanoseconds);
                    if (VERBOSE_COUT) writeToStdOut(buffer);
                    writeToLog(buffer, fp);
                }

                numRequests++;

                granted = 0;
                rscAvailable = resourceAvailable(rsc_tbl, resource);
                // sprintf(reason, "resource is unavailable");
                
                if (rscAvailable)
                {
                    // run baker's algorithm
                    granted = bankersAlgorithm(rsc_tbl, pid, resource);
                    incrementClock_ns(sysclock, ossGetWork());
                    bankersCount++;
                }
                
                if (granted)
                {
                    if (verbose)
                    {
                        sprintf(buffer, "oss: Granting P%d R%d at time %ld:%'ld\n",
                            pid, resource+1, sysclock->seconds, sysclock->nanoseconds);
                        writeToLog(buffer, fp);
                        if (VERBOSE_COUT) writeToStdOut(buffer);
                    }

                    rsc_tbl->rsc_descs[resource].allocated[pid]++;
                    resourcesGranted++;
                    
                    if (resourcesGranted % 20 == 0)
                    {
                        writeAllocatedResourceTable(rsc_tbl, fp);
                    }

                    // send msg back to user to allow request
                    sendMsg(rscMsgBoxId, &rsc_msg_box, pid+MAX_RUNTIME_PROCS);
                }
                else {
                    sprintf(buffer, "oss: Blocking P%d while R%d is busy at time %ld:%'ld\n",
                        pid, resource+1, sysclock->seconds, sysclock->nanoseconds);
                    writeToLog(buffer, fp);
                    if (VERBOSE_COUT) writeToStdOut(buffer);
                    enqueue(&blocked[resource], pid);     
                    // save time of blocked process
                    time_blocked[pid] = *sysclock;
                }

            }
            else if (strcmp(msg.txt, "RLS") == 0)
            {
                if (verbose)
                {
                    sprintf(buffer, "OSS: P%d released R%d at time %ld:%'ld\n",
                        pid, resource+1, sysclock->seconds, sysclock->nanoseconds);
                    writeToLog(buffer, fp);
                    if (VERBOSE_COUT) writeToStdOut(buffer);
                }
                rsc_tbl->rsc_descs[resource].allocated[pid]--;
                sendMsg(rscMsgBoxId, &rsc_msg_box, pid+MAX_RUNTIME_PROCS);
                unblockBlockedProcs(resource, rsc_msg_box, time_blocked);

            }
            else {
                if (verbose)
                {
                    sprintf(buffer, "OSS: Acknowledging that P%d terminated at time %ld:%'ld\n",
                        pid, sysclock->seconds, sysclock->nanoseconds);
                    if (VERBOSE_COUT) writeToStdOut(buffer);
                    writeToLog(buffer, fp);
                }

                // Update program state
                childpids[pid] = 0;
                proc_cnt--;
                releaseResources(rsc_tbl, pid); // Updates resource table

                for (unsigned i = 0; i < RSC_TYPES; i++)
                {
                     for (unsigned j = 0; j < MAX_CLAIMS; j++)
                    {
                        // Check if any processes can be unblocked
                        unblockBlockedProcs(i, rsc_msg_box, time_blocked);
                    }
                }

            }
            if (verbose)
            {
                sprintf(buffer, "\n");
                writeToLog(buffer, fp);
                if (VERBOSE_COUT) writeToStdOut(buffer);
            }

            incrementClock_ns(sysclock, ossGetWork());
        }
        incrementClock_ns(sysclock, get_nanoseconds());

        // Calculate total elapsed real-time seconds
        gettimeofday(&tv_stop, NULL);
        realElapsedTime = tv_stop.tv_sec - tv_start.tv_sec;
    }

    sprintf(buffer, "oss: Exiting at time %'ld:%'ld because %d simulated clock seconds have elapsed\n",
        sysclock->seconds, sysclock->nanoseconds, TOTAL_RUNTIME);
    writeToLog(buffer, fp);

    if (VERBOSE_COUT) writeToStdOut(buffer);

    writeAllocatedResourceTable(rsc_tbl, fp);
    summarizeRun(rsc_tbl, fp);

    sprintf(buffer, "\n");
    writeToLog(buffer, fp);

    if (VERBOSE_COUT) writeToStdOut(buffer);

    printBlockedProcessQueue();
    printResourceStatus(numRequests);
    cleanAndEscape();

    return 0;
}

void doAFork(char** execv_arr, unsigned pid)
{
    // fork and if child, exec user process
    if ((childpids[pid] = fork()) == 0)
    {
        char clock_id[10];
        char rmsgbox_id[10];
        char p_id[5];
        char rtbl_id[10];
        
        sprintf(clock_id, "%d", simClockId);
        sprintf(rtbl_id, "%d", rscTableId);
        sprintf(rmsgbox_id, "%d", rscMsgBoxId);
        sprintf(p_id, "%d", pid);
        
        execv_arr[SYSCLOCK_ID_IDX] = clock_id;
        execv_arr[RSC_TBL_ID_IDX] = rtbl_id;
        execv_arr[RSC_MSGBX_ID_IDX] = rmsgbox_id;
        execv_arr[PID_IDX] = p_id;

        execvp(execv_arr[0], execv_arr);

        perror("Child failed to execvp");
        exit(1);
    }

    if (childpids[pid] == -1)
    {
        perror("Child failed to fork\n");
        exit(1);
    }
}

void waitOnChildren()
{
    printf("oss: waiting on children\n");
    fprintf(fp, "oss: waiting on children\n");
    pid_t pid;

    while ((pid = wait(NULL)))
    {
        if (pid < 0)
        {
            if (errno == ECHILD)
            {
                perror("oss");
                break;
            }
        }
    }
}

void killChildProcs()
{
    printf("oss: killing children with SIGTERM\n");
    fprintf(fp, "oss: killing children with SIGTERM\n");

    for (unsigned i = 1; i <= MAX_RUNTIME_PROCS; i++)
    {
        if (childpids[i] == 0) continue;

        if (kill(childpids[i], SIGTERM) < 0)
        {
            if (errno != ESRCH) perror("Failed to terminate running child process");

        }
    }
    free(childpids);
}

void bindSignalHandlers()
{
    struct sigaction act;
    act.sa_handler = sigintHandler; // Signal handler
    sigemptyset(&act.sa_mask);      // only block this signal
    act.sa_flags = 0;               // do not modify behavior
    if (sigaction(SIGINT, &act, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    act.sa_handler = sigalrmHandler; // Signal handler
    sigemptyset(&act.sa_mask);       // only block this signal
    if (sigaction(SIGALRM, &act, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
}

void sigintHandler(int sig)
{
    printf("\noss: Caught SIGINT%d\n", sig);
    fprintf(fp, "\noss: Caught SIGINT%d\n", sig);
    if (!exiting)
    {
        exiting = true;
        cleanAndEscape();
    }
}

void sigalrmHandler(int sig)
{
    printf("\noss: Caught SIGALRM%d\n", sig);
    fprintf(fp, "\noss: Caught SIGALRM%d\n", sig);
    if (!exiting)
    {
        exiting = true;
        cleanAndEscape();
    }

}

void cleanAndEscape()
{
    killChildProcs();
    printf("oss: Removing message queues and shared memory\n");
    fprintf(fp, "oss: Removing message queues and shared memory\n");
    rmMsgQueue(rscMsgBoxId);
    waitOnChildren();
    cleanMemory(simClockId, sysclock);
    cleanMemory(rscTableId, rsc_tbl);
    free(blocked);
    free(blockClock);
    // close log file
    fclose(fp);
    exit(0);
}

Clock getRandomForkTime(Clock sysclock)
{
    unsigned ns_before_next_proc = rand() % MAX_TIME_BETWEEN_EXECS;
    incrementClock_ns(&sysclock, ns_before_next_proc);
    return sysclock;
}

unsigned get_nanoseconds()
{
    unsigned retVal = (rand() % 800000) + 10000;
    return retVal;
}

unsigned ossGetWork()
{
    unsigned retVal = (rand() % 800000) + 10000;
    return retVal;
}

unsigned getNextPidIndex()
{
    for (unsigned i = 1; i <= MAX_RUNTIME_PROCS; i++)
    {
        if (childpids[i] > 0)
        {
            continue;
        }
        return i;
    }
    return -1;
}

// read message from user process and free memory
message processUserMessage(char* aMsg)
{
    message msg;
    char ** msgDetails = split_string(aMsg, ",");
    msg.pid = atoi(msgDetails[0]);
    strcpy(msg.txt, msgDetails[1]);
    msg.resource = atoi(msgDetails[2]);
    free(msgDetails);

    return msg;
}

void unblockBlockedProcs(int resource, msgbuf rsc_msg_box, Clock* time_blocked)
{
    // if there are no blocked processes for this resource or resource is not available, return
    if (empty(&blocked[resource]) || !resourceAvailable(rsc_tbl, resource)) return;

    char buffer [256];

    int pid = peek(&blocked[resource]);

    // run banker's algorithm to check if request can be granted
    bool granted = bankersAlgorithm(rsc_tbl, pid, resource);
    bankersCount++;

    incrementClock_ns(sysclock, ossGetWork());

    if (granted)
    {
        sprintf(buffer, "oss: Unblocking P%d and allocating resource to R%d at time %ld:%'ld\n",
            pid, resource+1, sysclock->seconds, sysclock->nanoseconds);

        writeToLog(buffer, fp);

        if (VERBOSE_COUT) writeToStdOut(buffer);

        rsc_tbl->rsc_descs[resource].allocated[pid]++;
        dequeue(&blocked[resource]);
        Clock wait_time = clockDiff(*sysclock, time_blocked[pid]);
        *blockClock = addClock(*blockClock, wait_time);
        
        resourcesGranted++;

        if (resourcesGranted % 20 == 0) writeAllocatedResourceTable(rsc_tbl, fp);

        // Send response back to user
        sendMsg(rscMsgBoxId, &rsc_msg_box, pid+MAX_RUNTIME_PROCS);
    }
}

void printBlockedProcessQueue()
{
    char buffer[1024];
    char* queue;
    bool queue_is_empty = true;
    
    sprintf(buffer, "Blocked Processes\n");
    for (unsigned i = 0; i < RSC_TYPES; i++)
    {
        // if the queue is empty, skip
        if (empty(&blocked[i])) continue;
        
        sprintf(buffer + strlen(buffer), "  R%2d:", i);
        queue = getQueue(&blocked[i]);
        sprintf(buffer + strlen(buffer), "%s", queue);
        free(queue);
        queue_is_empty = false;
    }
    
    if (queue_is_empty) sprintf(buffer + strlen(buffer), "  < no blocked processes >\n");

    writeToLog(buffer, fp);

    if (VERBOSE_COUT) writeToStdOut(buffer);
}

void printResourceStatus(unsigned numRequests)
{
    char buffer[1024];

    sprintf(buffer, "Run Statistics\n");
    sprintf(buffer + strlen(buffer), "  %-25s: %'d\n", "Total Requests Made", numRequests);
    sprintf(buffer + strlen(buffer), "  %-25s: %'d\n", "Requests Granted", resourcesGranted);
    sprintf(buffer + strlen(buffer), "  %-25s: %.2f%\n", "Granted/Total requests", RatioOfResourcesUsed(numRequests));
    sprintf(buffer + strlen(buffer), "  %-25s: %'ld:%'ld\n", "Time Blocked", blockClock->seconds, blockClock->nanoseconds);
    sprintf(buffer + strlen(buffer), "  %-25s: %'d\n\n", "Deadlock Avoidance Runs", bankersCount);
    
    writeToLog(buffer, fp);
    writeToStdOut(buffer);
}

float RatioOfResourcesUsed(unsigned numRequests)
{
    float retval = (resourcesGranted / (float) numRequests) * 100;
    return  retval;
}

void printUsage()
{
    fprintf(stderr, "Usage: oss [-v for verbose mode -c to make console less cluttered in verbose mode]\n");
    exit(0);
}
