all:
	cd compiler; make all
	cd runtime; make all
#	cd sample; make all

clean:
	cd compiler; make clean
	cd runtime; make clean
#	cd sample; make clean
	rm -f GTAGS GPATH GRTAGS gmon.out

maintainer-clean:
	cd compiler; make maintainer-clean
	cd runtime; make maintainer-clean
	rm -rf aclocal.m4 autom4te.cache compile config.h config.h.in config.log config.status configure depcomp install-sh missing stamp-h1
	rm -f compiler/Makefile.in runtime/Makefile.in
