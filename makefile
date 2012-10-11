all:	MemoCalc.prc

clean:
	rm -f *.res *.bin *.grc *.o MemoCalc MemoCalc.prc

force:	clean	all

bin.res:	MemoCalc.rcp
	rm -f *.res *.bin
	pilrc MemoCalc.rcp
	touch bin.res

MathLib.o:	MathLib.c MathLib.h
	m68k-palmos-gcc -fno-builtin -o MathLib.o -I/m68k-palmos/include -c MathLib.c

MemoCalc.o:	MemoCalc.c MemoCalc.h
	m68k-palmos-gcc -fno-builtin -o MemoCalc.o -I/m68k-palmos/include -c MemoCalc.c

MemoCalcFunctions.o:	MemoCalcFunctions.c MemoCalcFunctions.h
	m68k-palmos-gcc -fno-builtin -o MemoCalcFunctions.o -I/m68k-palmos/include -c MemoCalcFunctions.c

MemoCalcLexer.h:        MemoCalcFunctions.h
MemoCalcLexer.o:	MemoCalcLexer.c MemoCalcLexer.h
	m68k-palmos-gcc -fno-builtin -o MemoCalcLexer.o -I/m68k-palmos/include -c MemoCalcLexer.c

MemoCalcParser.o:	MemoCalcParser.c MemoCalcParser.h
	m68k-palmos-gcc -fno-builtin -o MemoCalcParser.o -I/m68k-palmos/include -c MemoCalcParser.c

MemoCalc.prc:	MemoCalc bin.res
	build-prc MemoCalc.prc 'Memo Calc' MeCa *.bin *.grc

MemoCalc:	MemoCalc.o MemoCalcLexer.o MemoCalcParser.o MathLib.o MemoCalcFunctions.o
	rm -f *.grc
	m68k-palmos-gcc -o MemoCalc MemoCalc.o MemoCalcLexer.o MemoCalcParser.o MathLib.o MemoCalcFunctions.o -L/m68k-palmos/lib
	m68k-palmos-obj-res MemoCalc

