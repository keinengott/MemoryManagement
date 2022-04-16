#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "message_queue.h"

int getMsgQueue()
{
    int msgqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);

    if (msgqid == -1)
    {
        perror("msgget");
        exit(1);
    }

    return msgqid;
}

void receiveMsgNow(int msgqid, msgbuf* mbuf, int mtype)
{
    sprintf(mbuf->mtext, "0");

    if (msgrcv(msgqid, mbuf, sizeof(mbuf->mtext), mtype, IPC_NOWAIT) == -1)
    {
        if (errno == ENOMSG) return;
        perror("msgrcv");
        exit(1);
    }
}

void receiveMsg(int msgqid, msgbuf* mbuf, int mtype)
{
    if (msgrcv(msgqid, mbuf, sizeof(mbuf->mtext), mtype, 0) == -1)
    {
        perror("msgrcv");
        exit(1);
    }
}

void sendMsg(int msgqid, msgbuf* mbuf, int mtype)
{
    mbuf->mtype = mtype;
    if (msgsnd(msgqid, mbuf, sizeof(mbuf->mtext), IPC_NOWAIT) < 0)
    {
        perror("msgsnd");
        exit(1);
    }
}

void rmMsgQueue(int msgqid)
{
    if (msgctl(msgqid, IPC_RMID, NULL) == -1)
    {
        perror("msgctl");
        exit(1);
    }
}
