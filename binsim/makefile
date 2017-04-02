CPP = g++ 
CPPFLAGS = -g -m32 -DBINSIM
LIBCPPFLAGS = -g -m32 -shared -Wno-deprecated -fPIC -DVHPI -DBINSIM

binsim:	main.o dspdef.o parse.o bcode.o binsimcore.o binsimsupport.o stack.o dmem.o cordic.o Doxyfile 
	/bin/rm -f main.o binsimcore.o
	$(CPP) $(CPPFLAGS) -c main.cc dspdef.cc parse.cc bcode.cc binsimcore.cc binsimsupport.cc stack.cc dmem.cc cordic.cc
	$(CPP) $(CPPFLAGS) -o binsim main.o dspdef.o parse.o bcode.o binsimcore.o binsimsupport.o stack.o dmem.o cordic.o -lm
	doxygen Doxyfile 2> /dev/null

binsim.so:	main.o dspdef.o parse.o bcode.o binsimcore.o binsimsupport.o stack.o dmem.o cordic.o dsp.o 
	/bin/rm -f main.o binsimcore.o
	$(CPP) $(LIBCPPFLAGS) -c main.cc dspdef.cc parse.cc bcode.cc binsimcore.cc binsimsupport.cc stack.cc dmem.cc cordic.cc dsp.cc
	$(CPP) $(LIBCPPFLAGS) -o binsim.so main.o dspdef.o parse.o bcode.o binsimcore.o binsimsupport.o stack.o dmem.o cordic.o dsp.o -lm

all: binsim binsim.so

main.o:	main.cc binsim.h 

dspdef.o: dspdef.cc dspdef.h binsim.h

parse.o: parse.cc parse.h binsim.h

binsimcore.o: binsimcore.cc binsimcore.h binsim.h

binsimsupport.o: binsimsupport.cc binsimsupport.h binsim.h

bcode.o: bcode.cc bcode.h binsim.h

stack.o:	stack.cc stack.h binsim.h 

dmem.o:	dmem.cc dmem.h binsim.h 

cordic.o:	cordic.cc cordic.h binsim.h 

dsp.o:	dsp.cc	dsp.h

clean:
	/bin/rm -f core *.o
