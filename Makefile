UTILS = stringUtils.c
EXE1 = server
OBJ1 = $(patsubst %.c, $(OBJDIR)/%.o, $(SRC1))
EXE2 = client
OBJ2 = $(patsubst %.c, $(OBJDIR)/%.o, $(SRC2))
OBJ3 = $(patsubst %.c, $(OBJDIR)/%.o, $(UTILS))
SHELL := /bin/bash
CC = gcc
OBJDIR = obj
SRC1 = mftpserve.c
SRC2 = mftp.c
HDR = $(wildcard *.h)
LIBS = -lm -Wall -lpthread -g

$(EXE1): $(OBJ1) $(OBJ3)
	$(CC) $(OBJ1) $(OBJ3) -o $(EXE1) $(LIBS)
	
$(EXE2): $(OBJ2) $(OBJ3)
	$(CC) $(OBJ2) $(OBJ3) -o $(EXE2) $(LIBS)

$(OBJDIR)/%.o: %.c $(HDR) | $(OBJDIR)
	$(CC) -c $< -o $@ $(LIBS)

$(OBJDIR):
	mkdir -p $@

all: $(SRC1) $(SRC2) $(HDR)
	make clean
	make $(EXE1)
	make $(EXE2)
 
clean:
	rm -rf $(EXE1) $(EXE2) $(OBJDIR)

run:
	make all
	./server &
	./client localhost

