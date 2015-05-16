$(shell ./build_conf)

CXX = g++
RM ?= rm -rf
OBJS = src/pool.o src/fy_alloc.o
TEST_OBJS = test/main_test.o
TEST_BIN = bin/test
IFLAGS = -I. -I./src/ -I./include/
LFLAGS =
FLAGS = #-DNDEBUG

.PHONY: all
.PHONY: test
.PHONY: clean

all:

test: ${TEST_BIN}

${TEST_BIN}: ${OBJS} ${TEST_OBJS}
	${CXX} ${IFLAGS} ${LFLAGS} ${FLAGS} -o $@ $^

%.o: %.cc
	${CXX} ${IFLAGS} ${LFLAGS} ${FLAGS} -c $^ -o $@

clean:
	$(RM) *.o src/*.o test/*.o bin/* config.h
