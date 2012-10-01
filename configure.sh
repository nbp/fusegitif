#!/bin/sh

: ${BUILDDIR=$(pwd)/_build}
: ${INSTDIR=$(pwd)/_inst}
: ${CC=gcc}
: ${FUSE_CFLAGS=$(pkg-config --cflags fuse)}
: ${FUSE_LDFLAGS=$(pkg-config --libs fuse)}
: ${GIT2_CFLAGS=$(pkg-config --cflags libgit2)}
: ${GIT2_LDFLAGS=$(pkg-config --libs libgit2)}

cat > config.mk <<EOF
BUILDDIR=$BUILDDIR
INSTDIR=$INSTDIR
CC=$CC
FUSE_CFLAGS=$FUSE_CFLAGS
FUSE_LDFLAGS=$FUSE_LDFLAGS
GIT2_CFLAGS=$GIT2_CFLAGS
GIT2_LDFLAGS=$GIT2_LDFLAGS
EOF

