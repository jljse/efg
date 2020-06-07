#!/bin/sh
# this script will be broken with space-separated path

EFG_HOME="`dirname "$0"`/.."
EFG_BIN="${EFG_HOME}/compiler/efg-bin.exe"

${EFG_BIN} "$@" >${1%.lmn}.c
[ $? -ne 0 ] && exit 1
gcc -c -g -Wall -Wno-unused-label -Wno-unused-but-set-variable -I "${EFG_HOME}/runtime" ${1%.lmn}.c -o ${1%.lmn}.o
gcc -g -pg -Wall ${1%.lmn}.o ${EFG_HOME}/runtime/*.o -o ${1%.lmn}.exe
# rm ${1%.lmn}.c
rm ${1%.lmn}.o

