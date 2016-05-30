CFLAGS=-std=c++11 -Wall -Wextra -g $$(pkg-config fuse --cflags --libs)

# cpp to object files rule
%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

# test rules
TEST_SRC=CachingFileSystem.cpp Cache.h
TEST_FILE=caching
VALGRIND_FLAGS = --leak-check=full --show-possibly-lost=yes \
		 --show-reachable=yes --undef-value-errors=yes

$(TEST_FILE): $(TEST_SRC) 
	$(CXX) $< $(CFLAGS) -o $@

ValgrindTest: $(TEST_OBJ)
	valgrind $(VALGRIND_FLAGS) ./$<


# library rules
#$(LIBTARGET): $(LIBOBJ)
#	$(AR) $(ARFLAGS) $@ $^
#	$(RANLIB) $@
#
#$(LIBOBJ): $(LIBSRC)
#	$(CXX) $(CFLAGS) -c $< -o $(LIBOBJ)

# cleaning and such
RM=rm -fv
LOG_FILE=.filesystem.log
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
