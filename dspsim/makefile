LEX = flex -I
YACC = yacc

CC = gcc 
CFLAGS = -DYYDEBUG=1 -g -m32 
#LIBCFLAGS = -DYYDEBUG=1 -g -m32 -shared -fPIC -DVHPI -DDSPSIM
ASMCFLAGS = -DYYDEBUG=1 -g -m32 -DDSPASM
CPP = g++ 
CPPFLAGS = -g -m32 -L. -DDSPSIM
#LIBCPPFLAGS = -g -m32 -shared -Wno-deprecated -fPIC -DVHPI -L.
ASMCPPFLAGS = -g -m32 -DDSPASM -L.

dspsim:	main.o y.tab.o lex.yy.o symtab.o icode.o dmem.o optab.o dspdef.o simsupport.o simcore.o stack.o cordic.o secinfo.o memref.o Doxyfile
	/bin/rm -f main.o simcore.o
	$(CC) $(CFLAGS) -c lex.yy.c -o lex.yy.o
	$(CPP) $(CPPFLAGS) -c y.tab.cc main.cc symtab.cc icode.cc dmem.cc optab.cc dspdef.cc simcore.cc simsupport.cc stack.cc cordic.cc secinfo.cc memref.cc 
	$(CPP) $(CPPFLAGS) -o dspsim main.o y.tab.o lex.yy.o symtab.o icode.o dmem.o optab.o dspdef.o simcore.o simsupport.o stack.o cordic.o secinfo.o memref.o -ll -lm
	doxygen Doxyfile 2> /dev/null

#dspsim.so:	main.o y.tab.o lex.yy.o symtab.o icode.o dmem.o optab.o dspdef.o simcore.o simsupport.o stack.o cordic.o secinfo.o memref.o dsp.o 
#	/bin/rm -f main.o simcore.o
#	$(CC) $(LIBCFLAGS) -c lex.yy.c -o lex.yy.o
#	$(CPP) $(LIBCPPFLAGS) -c y.tab.cc main.cc symtab.cc icode.cc dmem.cc optab.cc dspdef.cc simcore.cc simsupport.cc stack.cc cordic.cc secinfo.cc memref.cc dsp.cc
#	$(CPP) $(LIBCPPFLAGS) -o dspsim.so main.o y.tab.o lex.yy.o symtab.o icode.o dmem.o optab.o dspdef.o simcore.o simsupport.o stack.o cordic.o secinfo.o memref.o dsp.o -ll -lm

dspasm:	main.o y.tab.o lex.yy.o symtab.o icode.o dmem.o optab.o dspdef.o simcore.o simsupport.o stack.o cordic.o secinfo.o memref.o Doxyfile
	/bin/rm -f main.o simcore.o
	$(CC) $(ASMCFLAGS) -c lex.yy.c -o lex.yy.o
	$(CPP) $(ASMCPPFLAGS) -c y.tab.cc main.cc symtab.cc icode.cc dmem.cc optab.cc dspdef.cc simcore.cc simsupport.cc stack.cc cordic.cc secinfo.cc memref.cc 
	$(CPP) $(ASMCPPFLAGS) -o dspasm main.o y.tab.o lex.yy.o symtab.o icode.o dmem.o optab.o dspdef.o simcore.o simsupport.o stack.o cordic.o secinfo.o memref.o -ll -lm
	doxygen Doxyfile 2> /dev/null

#all: dspsim dspsim.so dspasm
all: dspsim dspasm

lex.yy.o:	lex.yy.c y.tab.h

main.o:	main.cc dspsim.h symtab.h

lex.yy.o y.tab.o: dspsim.h

y.tab.cc y.tab.h: dspsim.y
	$(YACC) -v -d -o y.tab.cc dspsim.y
	mv y.tab.cc.h y.tab.h

lex.yy.c: dspsim.l
	$(LEX) dspsim.l

symtab.o:	symtab.cc symtab.h dspsim.h 

icode.o:	icode.cc icode.h dspsim.h 

dmem.o:	dmem.cc dmem.h dspsim.h 

optab.o:	optab.cc optab.h dspsim.h 

dspdef.o:	dspdef.cc dspdef.h dspsim.h 

simcore.o:	simcore.cc simcore.h dspsim.h 

simsupport.o:	simsupport.cc simsupport.h dspsim.h 

stack.o:	stack.cc stack.h dspsim.h 

cordic.o:	cordic.cc cordic.h dspsim.h 

secinfo.o:	secinfo.cc secinfo.h dspsim.h

memref.o:	memref.cc memref.h dspsim.h

dsp.o:	dsp.cc	dsp.h

clean:
	rm -f core *.o
	rm -f lex.yy.* y.tab.*
	rm -f dspasm dspsim 
#	rm -f dspasm dspsim dspsim.so
