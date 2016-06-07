CC = g++
default: all

EXECSEARCH = Search
EXECWORDCOUNT = WordCount
LIB = MapReduceFramework.a
CFLAGS = -Wall -Wvla -g -std=c++11 -lpthread -L. -lcrypto $(INCS)

all: WordCount

WordCount: WordCount.o MapReduceClient.h MapReduceFramework.h
	$(CC) $(CFLAGS) WordCount.o -o $(EXECWORDCOUNT) -l:MapReduceFramework.a
