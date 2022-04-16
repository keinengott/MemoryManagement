#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#define MSGSZ 50

struct msgbuf {
    long mtype;
    char mtext[MSGSZ];
};
typedef struct msgbuf msgbuf;

int getMsgQueue();
void rmMsgQueue(int msgqid);
void receiveMsg(int msgqid, msgbuf* mbuf, int mtype);
void receiveMsgNow(int msgqid, msgbuf* mbuf, int mtype);
void sendMsg(int msgqid, msgbuf* mbuf, int mtype);

#endif
