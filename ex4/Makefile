CFLAGS=-std=c++11 -Wall -Wextra -g

# cpp to object files rule
%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

# test rules
TEST_SRC=CachingFileSystem.cpp Cache.h
TEST_FILE=CachingFileSystem

$(TEST_FILE): $(TEST_SRC) 
	$(CXX) $< $(CFLAGS) $$(pkg-config fuse --cflags --libs) -o $@


# valgrind rule
VALGRIND_FLAGS = --leak-check=full --show-possibly-lost=yes \
		 --show-reachable=yes --undef-value-errors=yes
TEST_PARAMS=/tmp/root/ /tmp/mount/ 100 0.33 0.2

ValgrindTest: $(TEST_FILE)
	valgrind $(VALGRIND_FLAGS) ./$< $(TEST_PARAMS) &

# tar rule
TAR=tar
TARFLAGS=-cvf
TARNAME=ex4.tar
EXTRA_HEADERS=
TARSRCS=$(TEST_SRC) Makefile README $(EXTRA_HEADERS) 

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

RM=rm -fv
LOG_FILE=.filesystem.log
clean:
	$(RM) $(TEST_FILE) *.o $(LOG_FILE)

all: $(TEST_FILE)

.PHONY: all clean tar ValgrindTest
