CFLAGS=-Wall -Wextra -g -std=c++11 $(INCS)

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

LIBTARGET=libuthreads.a

all: $(TEST_OBJ)

# library rules
$(LIBOBJ): $(LIBSRC)
	$(CXX) $(CFLAGS) -pthread -c $<

$(LIBTARGET): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@


# test rules
TEST_FILE=Search.cpp
TEST_OBJ=Search
VALGRIND_FLAGS = --leak-check=full --show-possibly-lost=yes \
		 --show-reachable=yes --undef-value-errors=yes

$(TEST_OBJ): $(LIBTARGET)
	$(CC) $(CFLAGS) $(TEST_FILE) $(LIBTARGET) -o $@

ValgrindTest: $(TEST_OBJ)
	valgrind $(VALGRIND_FLAGS) ./$<

# cleaning and such
RM=rm -fv
clean:
	$(RM) $(TEST_OBJ) $(LIBTARGET) 

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

# tar rule
TAR=tar
TARFLAGS=-cvf
TARNAME=ex3.tar
TARSRCS=$(LIBSRC) $(TEST_FILE) Makefile README

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

.PHONY: all clean tar ValgrindTest
