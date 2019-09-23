CC=gcc
LIB_DIR=lib/
INC_DIR=include/
BIN_DIR=bin/
SRC_DIR=src/

all: cthread
	ar crs libcthread.a $(BIN_DIR)lib.o $(BIN_DIR)support.o
	mv libcthread.a $(LIB_DIR)

cthread: #gera arquivo objeto do cthread.c
	$(CC) -c $(SRC_DIR)lib.c -Wall
	mv lib.o $(BIN_DIR)

clean:
	rm -rf $(LIB_DIR)*.a $(BIN_DIR)*.o $(SRC_DIR)*~ $(INC_DIR)*~ *~