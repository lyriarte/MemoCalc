all:	MemoCalc.prc

clean:
	rm -f *.res *.bin *.grc *.o MemoCalc MemoCalc.prc


bin.res:	MemoCalc.rcp
	rm -f *.res *.bin
	pilrc MemoCalc.rcp
	touch bin.res

MemoCalc.o:	MemoCalc.c MemoCalc.h
	m68k-palmos-gcc -fno-builtin -o MemoCalc.o -I/m68k-palmos/include -c MemoCalc.c

MemoCalcLexer.o:	MemoCalcLexer.c
	m68k-palmos-gcc -fno-builtin -o MemoCalcLexer.o -I/m68k-palmos/include -c MemoCalcLexer.c

MemoCalc.prc:	MemoCalc bin.res
	build-prc MemoCalc.prc 'Memo Calc' MeCa *.bin *.grc

MemoCalc:	MemoCalc.o MemoCalcLexer.o
	rm -f *.grc
	m68k-palmos-gcc -o MemoCalc MemoCalc.o MemoCalcLexer.o -L/m68k-palmos/lib
	m68k-palmos-obj-res MemoCalc

