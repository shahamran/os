CFLAGS=-std=c++11 -Wall -Wextra -g -pthread $(INCS)

# cpp to object files rule
%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

# Library Stuff
INCS=-I.
LOADLIBES=-L./

LIBSRC=MapReduceFramework.cpp
LIBH=MapReduceFramework.h
LIBOBJ=MapReduceFramework.o
ARFLAGS=rvs
RANLIB=ranlib

LIBTARGET=MapReduceFramework.a

# test rules
TEST_SRC=Search.cpp
TEST_OBJ=Search.o
TEST_FILE=Search
VALGRIND_FLAGS = --leak-check=full --show-possibly-lost=yes \
		 --show-reachable=yes --undef-value-errors=yes

$(TEST_FILE): $(TEST_OBJ) $(LIBTARGET) 
	$(CXX) $(CFLAGS) $^ -o $@

ValgrindTest: $(TEST_OBJ)
	valgrind $(VALGRIND_FLAGS) ./$<


# library rules
$(LIBTARGET): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

$(LIBOBJ): $(LIBSRC)
	$(CXX) $(CFLAGS) -c $< -o $(LIBOBJ)

# cleaning and such
RM=rm -fv
clean:
	$(RM) $(TEST_FILE) $(LIBTARGET) *.o

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

# tar rule
TAR=tar
TARFLAGS=-cvf
TARNAME=ex3.tar
TARSRCS=$(LIBSRC) $(TEST_SRC) Makefile README

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

.PHONY: all clean tar ValgrindTest
