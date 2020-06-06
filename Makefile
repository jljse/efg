all:
	make -C compiler all
	make -C runtime all
#	cd sample; make all

clean:
	make -C compiler clean
	make -C runtime clean
#	cd sample; make clean
	rm -f GTAGS GPATH GRTAGS gmon.out

maintainer-clean:
	make -C compiler maintainer-clean
	make -C runtime maintainer-clean
	rm -rf aclocal.m4 autom4te.cache compile config.h config.h.in config.log config.status configure depcomp install-sh missing stamp-h1
	rm -f compiler/Makefile.in runtime/Makefile.in
