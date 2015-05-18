CXX ?= g++
RM ?= rm -rf
OBJS = src/pool.o src/fy_alloc.o

TEST_OBJS = test/main_test.o
TEST_BIN = bin/test

IFLAGS = -I. -I./src/ -I./include/
LFLAGS =
CFLAGS = #-DNDEBUG

$(shell ./build_conf config.h)

.PHONY: all
.PHONY: test
.PHONY: clean

all: ${OBJS}
${OBJS}: %.o: %.cc
	${CXX} ${IFLAGS} ${LFLAGS} ${CFLAGS} -c $^ -o $@

test: ${TEST_BIN}
${TEST_BIN}: ${TEST_OBJS} ${OBJS}
	${CXX} ${IFLAGS} ${LFLAGS} ${CFLAGS} -o $@ $^
${TEST_OBJS}: %.o: %.cc
	${CXX} ${IFLAGS} ${LFLAGS} ${CFLAGS} -c $^ -o $@

clean:
	$(RM) *.o src/*.o test/*.o bin/* config.h
