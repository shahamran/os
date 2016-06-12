#################################################################
#
#				Warning!!!!
#				============
#	
# this make file compiles all the sources with implicit rules
# therefore in order to make sure all the changes were made 
# run it with "make clean all"
#
##################################################################
## ps auxw -T | fgrep v3
CC = g++
default: all

EXECSEARCH = Search
EXECWORDCOUNT = WordCount
TAR=tar
TARFLAGS=-cvf
TARNAME=ex3.tar
LIB = MapReduceFramework.a

RANLIB=ranlib

FILES = Search.o WordCount.o MapReduceFramework.o $(LIB)

INCS = -I.
CFLAGS = -Wall -Wvla -g -std=c++11 -pthread -L. -lcrypto $(INCS) # -lpthread --> -pthread
LOADLIBES = -pthread -L./ -lcrypto # -lpthread --> -pthread

all: Search

Search : Search.o MapReduceFramework.h MapReduceClient.h
	$(CC) $(CFLAGS) Search.o -o $(EXECSEARCH) -l:MapReduceFramework.a

#Search : Search.o MapReduceFramework.h MapReduceClient.h
#	$(CC) $(CFLAGS) Search.o -o $(EXECSEARCH) -l:MapReduceFramework.a

#Search.o: Search.cpp MapReduceClient.h MapReduceFramework.h
#	$(CC) $(CFLAGS) -c Search.cpp MapReduceClient.h MapReduceFramework.h
