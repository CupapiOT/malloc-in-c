CC = gcc
CCFLAGS = -std=c99 -Wall -Wextra -Wshadow -Wfloat-equal -Wformat=2 -g -O2 -pedantic
DGB = gdb
PROGRAM = use_halloc.out
SRC = use_halloc.c
HALLOC = halloc.c

.PHONY: run debug build clean

run: ${PROGRAM}
	./$< ${a}

debug: ${SRC} ${HALLOC}
	${CC} -g $^ -o ${PROGRAM}

build: ${PROGRAM}

${PROGRAM}: ${SRC} ${HALLOC}
	${CC} ${CCFLAGS} $^ -o ${PROGRAM}

clean:
	rm -f *.o *.out
