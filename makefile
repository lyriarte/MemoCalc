all:	MemoCalc.prc

clean:
	rm -f *.res *.bin *.grc *.o MemoCalc MemoCalc.prc

force:	clean	all

bin.res:	MemoCalc.rcp
	rm -f *.res *.bin
	pilrc MemoCalc.rcp
	touch bin.res

MemoCalc.o:	MemoCalc.c MemoCalc.h
	m68k-palmos-gcc -fno-builtin -o MemoCalc.o -I/m68k-palmos/include -c MemoCalc.c

MemoCalcLexer.o:	MemoCalcLexer.c
	m68k-palmos-gcc -fno-builtin -o MemoCalcLexer.o -I/m68k-palmos/include -c MemoCalcLexer.c

MemoCalcParser.o:	MemoCalcParser.c
	m68k-palmos-gcc -fno-builtin -o MemoCalcParser.o -I/m68k-palmos/include -c MemoCalcParser.c

MemoCalc.prc:	MemoCalc bin.res
	build-prc MemoCalc.prc 'Memo Calc' MeCa *.bin *.grc

MemoCalc:	MemoCalc.o MemoCalcLexer.o MemoCalcParser.o
	rm -f *.grc
	m68k-palmos-gcc -o MemoCalc MemoCalc.o MemoCalcLexer.o MemoCalcParser.o -L/m68k-palmos/lib
	m68k-palmos-obj-res MemoCalc

