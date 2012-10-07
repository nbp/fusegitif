include config.mk
CFLAGS=
LDFLAGS=

all: lsR fusegitif

lsR: CFLAGS=${GIT2_CFLAGS}
lsR: LDFLAGS=${GIT2_LDFLAGS}

fusegitif: CFLAGS=${GIT2_CFLAGS} ${FUSE_CFLAGS}
fusegitif: LDFLAGS=${GIT2_LDFLAGS} ${FUSE_LDFLAGS}

gitstat.o: CFLAGS=${GIT2_CFLAGS}
gitstat.o: LDFLAGS=${GIT2_LDFLAGS}

lsR: lsR.o gitstat.o
	${CC} -O0 -ggdb3 ${CFLAGS} ${LDFLAGS} -std=gnu99 -o $@ $^

fusegitif: fusegitif.o gitstat.o
	${CC} -O0 -ggdb3 ${CFLAGS} ${LDFLAGS} -std=gnu99 -o $@ $^

%.o: %.c %.h
	${CC} -O0 -ggdb3 ${CFLAGS} -std=gnu99 -c -o $@ $<

%.o: %.c
	${CC} -O0 -ggdb3 ${CFLAGS} -std=gnu99 -c -o $@ $<

clean:
	-rm -f gitstat.o lsR.o fusegitif.o lsR fusegitif
