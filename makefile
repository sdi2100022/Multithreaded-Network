OBJS    = build/jobCommander.o build/jobExecutorServer.o build/jobCommands.o build/jobUtil.o build/jobThreads.o
SOURCE  = src/jobCommander.c src/jobExecutorServer.c src/jobCommands.c src/jobUtil.c src/jobThreads.c src/progDelay
HEADER  = lib/jobThreads.h lib/jobCommands.h lib/jobUtil.h lib/jobProject.h
OUT     = bin/jobCommander bin/jobExecutorServer bin/progDelay
SCRIPTS = $(wildcard ./tests/*.sh)
CC      = gcc
CFLAGS  = -Wall -g -Iinclude
LDFLAGS = -pthread

all: $(OUT)

bin/jobCommander: build/jobCommander.o
	$(CC) $(LDFLAGS) build/jobCommander.o -o bin/jobCommander

bin/jobExecutorServer: build/jobExecutorServer.o build/jobCommands.o build/jobUtil.o build/jobThreads.o
	$(CC) $(LDFLAGS) build/jobExecutorServer.o build/jobCommands.o build/jobUtil.o build/jobThreads.o -o bin/jobExecutorServer

build/jobCommander.o: src/jobCommander.c
	$(CC) $(CFLAGS) -c src/jobCommander.c -o build/jobCommander.o

build/jobExecutorServer.o: src/jobExecutorServer.c
	$(CC) $(CFLAGS) -c src/jobExecutorServer.c -o build/jobExecutorServer.o

build/jobThreads.o: src/jobThreads.c
	$(CC) $(CFLAGS) -c src/jobThreads.c -o build/jobThreads.o

build/jobCommands.o: src/jobCommands.c
	$(CC) $(CFLAGS) -c src/jobCommands.c -o build/jobCommands.o

build/jobUtil.o: src/jobUtil.c
	$(CC) $(CFLAGS) -c src/jobUtil.c -o build/jobUtil.o

bin/progDelay: src/progDelay.c
	$(CC) src/progDelay.c -o bin/progDelay

scripts:
	@chmod +x $(SCRIPTS)

clean:
	rm -f $(OBJS) $(OUT)

count: 
	wc $(SOURCE) $(HEADER) $(SCRIPTS)

