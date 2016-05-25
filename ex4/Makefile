CFLAGS=-std=c++11 -Wall -Wextra -g -pthread $(INCS)

# cpp to object files rule
%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

# Library Stuff
INCS=-I.
LOADLIBES=-L.

LIBSRC= # TODO
LIBH= 	# TODO
LIBOBJ=	# TODO
ARFLAGS=rvs
RANLIB=ranlib

LIBTARGET= # TODO

# test rules
TEST_SRC=# TODO
TEST_FILE=# TODO
VALGRIND_FLAGS = --leak-check=full --show-possibly-lost=yes \
		 --show-reachable=yes --undef-value-errors=yes

$(TEST_FILE): $(TEST_SRC) $(LIBTARGET) 
	$(CXX) $(CFLAGS) $(LOADLIBES) $^ -o $@

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
LOG_FILE=# TODO
clean:
	$(RM) $(TEST_FILE) $(LIBTARGET) *.o $(LOG_FILE)

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

# tar rule
TAR=tar
TARFLAGS=-cvf
TARNAME=ex4.tar
EXTRA_HEADERS=#TODO
TARSRCS=$(LIBSRC) $(TEST_SRC) Makefile README $(EXTRA_HEADERS) 

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

all: $(TEST_FILE)

.PHONY: all clean tar ValgrindTest
