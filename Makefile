CXX ?= g++
RM ?= rm -rf
OBJS = src/pool.o src/fy_alloc.o src/conf.o src/logger.o

TEST_OBJS = test/main_test.o
TEST_BIN = bin/test

IFLAGS = -I. -I./src/ -I./include/
LFLAGS =
CFLAGS = #-DNDEBUG

LIBRARY = libcctools.a

$(shell ./build_conf cctools_config.h)

.PHONY: all
.PHONY: test
.PHONY: clean

all: ${LIBRARY}
${LIBRARY}: ${OBJS}
	${AR} -rs $@ ${OBJS}
${OBJS}: %.o: %.cc
	${CXX} ${IFLAGS} ${LFLAGS} ${CFLAGS} -c $^ -o $@

test: ${TEST_BIN}
${TEST_BIN}: ${TEST_OBJS} ${OBJS}
	${CXX} ${IFLAGS} ${LFLAGS} ${CFLAGS} -o $@ $^
${TEST_OBJS}: %.o: %.cc
	${CXX} ${IFLAGS} ${LFLAGS} ${CFLAGS} -c $^ -o $@

clean:
	$(RM) *.o src/*.o test/*.o bin/* cctools_config.h ${LIBRARY}
