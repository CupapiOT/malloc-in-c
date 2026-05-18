CC = clang
# The flags `-shared` and `-fPIC` lets the code be shared.
CCFLAGS = -O0 -g -W -Wall -Wextra -shared -fPIC
SRC = custom-malloc.c
# `.so` stands for "Shared Object".
BIN = custom-malloc.so

CCTEST = gcc
CCTESTFLAGS = -std=c99 -Wall -Wextra -Wshadow -Wfloat-equal -Wformat=2 -g -O2 -pedantic
DGB = gdb
TEST_PROGRAM = use_halloc.out
TEST_FILE = use_halloc.c
DEBUG_FILE = halloc.c

.PHONY: debug build test buildtest clean

build: ${BIN}

${BIN}: ${SRC}
	${CC} ${CCFLAGS} ${SRC} -o ${BIN}
	@echo Copy the following: export LD_PRELOAD="$(abspath $@)"

test: ${TEST_PROGRAM}
	./$<

debug: ${TEST_FILE} ${DEBUG_FILE}
	${CCTEST} -g $^ -o ${TEST_PROGRAM}

buildtest: ${TEST_PROGRAM}

${TEST_PROGRAM}: ${TEST_FILE} ${DEBUG_FILE}
	${CCTEST} ${CCTESTFLAGS} $^ -o ${TEST_PROGRAM}

clean:
	rm -f *.o *.out *.so
