CC = gcc
CFLAGS = -c -g -Wall
LDFLAGS = 
OBJECTS = rpjtag_stateMachine.o rpjtag.o rpjtag_bit_reader.o rpjtag_bdsl_reader.o

all: RpiJtag

rpjtag.o: rpjtag.c rpjtag.h rpjtag_io.h
	${CC} ${CFLAGS} rpjtag.c -o rpjtag.o

rpjtag_stateMachine.o: rpjtag_stateMachine.c
	${CC} ${CFLAGS} rpjtag_stateMachine.c -o rpjtag_stateMachine.o

rpjtag_bit_reader.o: rpjtag_bit_reader.c
	${CC} ${CFLAGS} rpjtag_bit_reader.c -o rpjtag_bit_reader.o

rpjtag_bdsl_reader.o: rpjtag_bdsl_reader.c
	${CC} ${CFLAGS} rpjtag_bdsl_reader.c -o rpjtag_bdsl_reader.o

RpiJtag: rpjtag.o rpjtag_stateMachine.o rpjtag_bit_reader.o rpjtag_bdsl_reader.o
	${CC} ${LDFLAGS} ${OBJECTS} -o RpiJtag

clean:
	rm -f ${OBJECTS} RpiJtag

.PHONY: clean