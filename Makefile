include config.mk

lsR: lsR.o gitstat.o
	${CC} -O0 -ggdb3 ${GIT2_CFLAGS} ${GIT2_LDFLAGS} -std=gnu99 -o $@ $^

%.o: %.c %.h
	${CC} -O0 -ggdb3 ${GIT2_CFLAGS} ${GIT2_LDFLAGS} -std=gnu99 -c -o $@ $<

%.o: %.c
	${CC} -O0 -ggdb3 ${GIT2_CFLAGS} ${GIT2_LDFLAGS} -std=gnu99 -c -o $@ $<

