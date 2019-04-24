EXE1 = server
EXE2 = client
SHELL := /bin/bash
CC = gcc
OBJDIR = obj
SRC1 = mftpserve.c
SRC2 = mftp.c
HDR = $(wildcard *.h)
OBJ = $(patsubst %.c, $(OBJDIR)/%.o, $(SRC))
LIBS = -lm -Wall -lpthread

$(EXE1): $(OBJ)
	$(CC) $(OBJ) -o $(EXE1) $(LIBS)
	
$(EXE2): $(OBJ)
	$(CC) $(OBJ) -o $(EXE2) $(LIBS)

$(OBJDIR)/%.o: %.c $(HDR) | $(OBJDIR)
	$(CC) -c $< -o $@ $(LIBS)

$(OBJDIR):
	mkdir -p $@


all: $(SRC) $(HDR)
	$(CC) $(SRC) -o $(EXE) $(LIBS)
 
clean:
	rm -rf $(OBJDIR) $(EXE1) $(EXE2)

run:
	make
	./server &
	./client

