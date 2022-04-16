#ifndef CONSTANTS_H
#define CONSTANTS_H

const unsigned MAX_RUNTIME_PROCS = 18;
const unsigned RSC_TYPES = 20;         // the number of types of resources available for distribution
const unsigned MAX_CLAIMS = 2;         // Upper bound of max number of a resource class a process can claim
const unsigned EXECV_SIZE = 6;
// for ease of setting and parsing cmd line args between processes
const unsigned SYSCLOCK_ID_IDX = 1;
const unsigned RSC_TBL_ID_IDX = 2;
const unsigned RSC_MSGBX_ID_IDX = 3;
const unsigned PID_IDX = 4;

#endif
