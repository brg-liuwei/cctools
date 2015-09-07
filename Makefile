CXX ?= g++
RM ?= rm -rf
OBJS = src/pool.o src/fy_alloc.o src/conf.o src/logger.o src/util.o src/net.o

TEST_MAIN_OBJS = test/main_test.o
TEST_MAIN_BIN = bin/main_test

TEST_NET_OBJS = test/net_test.o
TEST_NET_BIN = bin/net_test

IFLAGS = -I. -I./src/ -I./include/
LFLAGS = -L. -lcctools -lpthread
CFLAGS = -fPIC #-DNDEBUG

STATIC_LIB = libcctools.a
DYNAMIC_LIB = libcctools.${DY_SUFFIX}

$(shell ./build_conf cctools_config.h)
VAR = vars
$(shell ./detect_platform ${VAR})
include ${VAR}

.PHONY: all test clean

all: ${STATIC_LIB} ${DYNAMIC_LIB}
${STATIC_LIB}: ${OBJS}
	${AR} -rs $@ ${OBJS}
${DYNAMIC_LIB}: ${OBJS}
	${CXX} -shared ${OBJS} -o $@
${OBJS}: %.o: %.cc
	${CXX} ${IFLAGS} ${CFLAGS} -c $^ -o $@

test: ${TEST_MAIN_BIN} ${TEST_NET_BIN}
${TEST_MAIN_BIN}: ${TEST_MAIN_OBJS}
	${CXX} $^ ${LFLAGS} ${CFLAGS} -o $@
${TEST_NET_BIN}: ${TEST_NET_OBJS}
	${CXX} $^ ${LFLAGS} ${CFLAGS} -o $@
${TEST_MAIN_OBJS}: %.o: %.cc
	${CXX} ${IFLAGS} ${CFLAGS} -c $^ -o $@
${TEST_NET_OBJS}: %.o: %.cc
	${CXX} ${IFLAGS} ${CFLAGS} -c $^ -o $@

clean:
	$(RM) *.o src/*.o test/*.o bin/* cctools_config.h ${VAR} ${STATIC_LIB} ${DYNAMIC_LIB}
