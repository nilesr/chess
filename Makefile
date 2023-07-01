#CFLAGS=-ggdb3 -Og -Wall -fopenmp -std=gnu11 -march=native
CFLAGS=-Ofast -DNDEBUG -fopenmp -std=gnu11 -march=native -fomit-frame-pointer
HEADERS=$(wildcard *.h)
chess: chess.o chess_list.o uci.o
	$(CC) $^ -o $@ $(CFLAGS)

%.o: %.c $(HEADERS)
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm chess *.o

all: chess

.PHONY: clean all
