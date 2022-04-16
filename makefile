CC = c99
IDIR =./include
CFLAGS = -I$(IDIR) -g -Wall -lm -D_SVID_SOURCE -D_POSIX_C_SOURCE

EXEC1 = oss
OBJS1 = oss.o

EXEC2 = user
OBJS2 = user.o

SHARE = helpers.o message_queue.o shared_memory.o queue.o clock.o bankers.o resources.o main_memory.o

DEPS = global_constants.h helpers.h message_queue.h shared_memory.h clock.h queue.h bankers.h resources.h main_memory.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: oss user

oss: $(OBJS1) $(SHARE)
	$(CC) -o $(EXEC1) $^ $(CFLAGS)
	
user: $(OBJS2) $(SHARE)
	$(CC) -o $(EXEC2) $^ $(CFLAGS)

clean:
	rm $(EXEC1) $(OBJS1) $(EXEC2) $(OBJS2) $(SHARE)
