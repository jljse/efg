LMN = $(basename $(wildcard *.lmn))
TARGET = $(addsuffix .exe, $(LMN)) $(addsuffix .il, $(LMN)) $(addsuffix .so, $(LMN))
OBJ = $(addsuffix .o, $(basename $(wildcard ../runtime/*.c)))

.PRECIOUS: %.o %.c

.PHONY: all clean %.make

all: $(TARGET)
	@echo -n ""
#	cd forslim; make

clean:
	rm -f *.exe *.c *.o *.il *.so *.stackdump gmon.out
	rm -f base/*.exe base/*.c base/*.o base/gmon.out
#	cd forslim; make clean

%.exe: %.o $(OBJ)
	gcc -pg -Wall $+ -o $@

%.o: %.c
	gcc -c -O2 -Wall -Wno-unused-label -I../runtime $< -o $@

%.c: %.lmn ../compiler/efg.exe
	../compiler/efg.exe $< >$@

%.il: %.lmn
	lmntal -O3 --slimcode $< >$@

%.so: %.il
	rm -f _tmp
	if [ -f $*.c ]; then mv $*.c _tmp; fi
	slim --translate $< >$*.c
	gcc -O2 -Wall -shared -fPIC -I ~/workspace/slim/src $*.c -o $@
	if [ -f _tmp ]; then mv _tmp $*.c; fi

