MODULES = ./database
PROGRAM1 = travelMonitorClient
PROGRAM2 = monitorServer

all: $(PROGRAM1) $(PROGRAM2)

OBJS1 = travelMonitorClient.o
OBJS1 += protocol.o
OBJS2 = monitorServer.o
OBJS2 += protocol.o

CFLAGS += -I./database
include $(MODULES)/make.inc

$(PROGRAM1): clean $(OBJS1)
	gcc $(OBJS1) -o $(PROGRAM1)


$(PROGRAM2): $(OBJS2)
	gcc $(CFLAGS) $(OBJS2) -o $(PROGRAM2) -lpthread
	rm -f $(OBJS1) $(OBJS2)
	

clean:
	rm -f $(PROGRAM1) $(PROGRAM2) log_* tmp_*
	

run: $(PROGRAM1)
	./$(PROGRAM1)
