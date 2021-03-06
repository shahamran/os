CFLAGS=-std=c++11 -Wall -Wextra -g -pthread $(INCS)

# cpp to object files rule
%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

# Library Stuff
INCS=-I.
LOADLIBES=-L.

LIBSRC=MapReduceFramework.cpp
LIBH=MapReduceFramework.h
LIBOBJ=MapReduceFramework.o
ARFLAGS=rvs
RANLIB=ranlib

LIBTARGET=MapReduceFramework.a

# test rules
TEST_SRC=Search.cpp
TEST_FILE=Search
VALGRIND_FLAGS = --leak-check=full --show-possibly-lost=yes \
		 --show-reachable=yes --undef-value-errors=yes

$(TEST_FILE): $(TEST_SRC) $(LIBTARGET) 
	$(CXX) $(CFLAGS) $(LOADLIBES) $^ -o $@

COUNT_DIRS:
	number=0 ; while [[ $$number -le 49 ]] ; do \
		echo $$number ; \
		((number = number + 1)) ; \
	done

# Directory that contains all search dirs
TEST_DIR=tests/test1/theirs/os_50folders
# Gets all dirs inside $TEST_DIR
TEST_DIRS=$(wildcard $(TEST_DIR)/*)
# String to search
TEST_SUBSTR=os
# Prints: <SUBSTRING_TO_SEARCH> <ALL_TEST_DIRS>
TEST_ARGS=$(TEST_SUBSTR) $(foreach dir,$(TEST_DIRS),$(dir) ) 

ValgrindTest: $(TEST_FILE)
	valgrind $(VALGRIND_FLAGS) ./$< $(TEST_ARGS)


# library rules
$(LIBTARGET): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

$(LIBOBJ): $(LIBSRC)
	$(CXX) $(CFLAGS) -c $< -o $(LIBOBJ)

# cleaning and such
RM=rm -fv
LOG_FILE=.MapReduceFramework.log
clean:
	$(RM) $(TEST_FILE) $(LIBTARGET) *.o $(LOG_FILE)

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

# tar rule
TAR=tar
TARFLAGS=-cvf
TARNAME=ex3.tar
EXTRA_HEADERS=my_pthread.h error_handle.h
GANTT_PICS=RR.jpg FCFS.jpg SRTF.jpg Priority.jpg
TARSRCS=$(LIBSRC) $(TEST_SRC) Makefile README $(EXTRA_HEADERS) $(GANTT_PICS)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

all: $(TEST_FILE)

.PHONY: all clean tar ValgrindTest
