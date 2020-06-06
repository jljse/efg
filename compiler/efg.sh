#!/bin/sh
# this script will be broken with space-separated path

EFG_HOME="`dirname "$0"`/.."
EFG_BIN="${EFG_HOME}/compiler/efg-bin.exe"

${EFG_BIN} "$@" >${1%.lmn}.c
gcc -c -O2 -Wall -Wno-unused-label -I "${EFG_HOME}/runtime" ${1%.lmn}.c -o ${1%.lmn}.o
gcc -pg -Wall ${1%.lmn}.o ${EFG_HOME}/runtime/*.o -o ${1%.lmn}.exe
# rm ${1%.lmn}.c
rm ${1%.lmn}.o

