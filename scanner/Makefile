CC=gcc

CUR=$(shell pwd)

SRCS_DP = uploader.c
OBJS_DP = $(SRCS_DP:.c=.o)
SRCS_DW = download.c
OBJS_DW = $(SRCS_DW:.c=.o)

CFLAGS= -O2 -g -Wall
LIBS =

PROGRAMS = uploader download

all: $(PROGRAMS) 

uploader:.depend_p $(OBJS_DP)
	$(CC) $(OBJS_DP) $(LIBS) $(LOCAL_LD_FLAGS) -o $@
download:.depend_d $(OBJS_DW)
	$(CC) $(OBJS_DW) $(LIBS) $(LOCAL_LD_FLAGS) -o $@

.depend_p:
	$(CC) -M $(CFLAGS) $(SRCS_DP) >$(@)
.depend_d:
	$(CC) -M $(CFLAGS) $(SRCS_DP) >$(@)

clean:
	rm -f *.o $(PROGRAMS) .depend*

