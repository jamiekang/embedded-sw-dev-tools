CPP = g++ -g
CPPFLAGS = -g -m32 -DDSPLNK

dsplnk:	dspdef.o typetab.o extreftab.o ocode.o symtab.o module.o main.o Doxyfile
	$(CPP) $(CPPFLAGS) -c main.cc module.cc symtab.cc ocode.cc extreftab.cc typetab.cc dspdef.cc
	$(CPP) $(CPPFLAGS) -o dsplnk main.o module.o symtab.o ocode.o extreftab.o typetab.o dspdef.o
	doxygen Doxyfile

main.o:	main.cc dsplnk.h

ocode.o: ocode.cc dsplnk.h

module.o: module.cc dsplnk.h

symtab.o: symtab.cc dsplnk.h

extreftab.o: extreftab.cc dsplnk.h

typetab.o: typetab.cc dsplnk.h

dspdef.o: dspdef.cc dsplnk.h

clean:
	/bin/rm -f core *.o
