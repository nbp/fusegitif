include config.mk

lsR: lsR.c
	${CC} -O0 -ggdb3 ${GIT2_CFLAGS} ${GIT2_LDFLAGS} -std=gnu99 $< -o $@

