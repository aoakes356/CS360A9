EXE1 = server
EXE2 = client
SHELL := /bin/bash
CC = gcc
OBJDIR = obj
SRC1 = mftpserve.c
SRC2 = mftp.c
HDR = $(wildcard *.h)
OBJ = $(patsubst %.c, $(OBJDIR)/%.o, $(SRC))
LIBS = -lm -Wall -lpthread -g

$(EXE1): $(OBJ)
	$(CC) $(SRC1) $(HDR) -o $(EXE1) $(LIBS)
	
$(EXE2): $(OBJ)
	$(CC) $(SRC2) $(HDR) -o $(EXE2) $(LIBS)


all: $(SRC1) $(SRC2) $(HDR)
	$(CC) $(SRC1) -o $(EXE1) $(LIBS)
	$(CC) $(SRC2) -o $(EXE2) $(LIBS)
 
clean:
	rm -rf $(EXE1) $(EXE2)

run:
	make
	./server &
	./client localhost

