EXE = quickerSort 
SHELL := /bin/bash
CC = gcc
OBJDIR = obj
SRC = $(wildcard *.c)
HDR = $(wildcard *.h)
OBJ = $(patsubst %.c, $(OBJDIR)/%.o, $(SRC))
LIBS = -lm -Wall -lpthread

$(EXE): $(OBJ)
	$(CC) $(OBJ) -o $(EXE) $(LIBS)

$(OBJDIR)/%.o: %.c $(HDR) | $(OBJDIR)
	$(CC) -c $< -o $@ $(LIBS)

$(OBJDIR):
	mkdir -p $@


all: $(SRC) $(HDR)
	$(CC) $(SRC) -o $(EXE) $(LIBS)
 
clean:
	rm -rf $(OBJDIR) $(EXE)

debug: $(SRC) $(HDR)
	$(CC) $(SRC) -g -o $(EXE) $(LIBS)

run:
	make
	./quickerSort
	

