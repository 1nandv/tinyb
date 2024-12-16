#!/usr/bin/env bash

set -e

CC="gcc"
CFLAGS="-Wall -Wextra"
NAME="tinyb"

exec $CC $NAME.c $CFLAGS -o $NAME
