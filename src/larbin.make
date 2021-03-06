ADNSDIR=$(BASEDIR)/../adns
LIBS:=

include $(BASEDIR)/../config.make

UTILS-OBJ:=string.o debug.o url.o connexion.o text.o \
    PersistentFifo.o hashDup.o mypthread.o rendezvous.o
INTERF-OBJ:=useroutput.o output.o
FETCH-OBJ:=site.o sequencer.o hashTable.o checker.o file.o \
	fetchOpen.o fetchPipe.o
MAIN-OBJ:=crawler.o main.o

ABS-UTILS-OBJ:=utils/string.o utils/debug.o utils/url.o \
    utils/connexion.o utils/text.o utils/PersistentFifo.o \
    utils/hashDup.o utils/mypthread.o utils/rendezvous.o
ABS-INTERF-OBJ:=interf/useroutput.o interf/output.o
ABS-FETCH-OBJ:=fetch/site.o fetch/sequencer.o fetch/hashTable.o \
    fetch/checker.o fetch/file.o fetch/fetchOpen.o fetch/fetchPipe.o
ABS-MAIN-OBJ:=$(MAIN-OBJ)

CFLAGS:=-O3 -Wall -D_REENTRANT
CXXFLAGS:= -Wno-deprecated -Wall -O3 -D_REENTRANT -iquote$(BASEDIR) -iquote$(ADNSDIR)
RM:=rm -f

first: all

dep-in:
	makedepend -f- -I$(BASEDIR) -Y *.cc 2> /dev/null > .depend

clean-in:
	$(RM) *.o
	$(RM) *~
	$(RM) *.bak

distclean-in: clean-in
	$(RM) .depend

redo-in: all

debug-in: CXXFLAGS += -g
debug-in: redo-in

prof-in: CXXFLAGS += -pg -DPROF
prof-in: redo-in

include .depend
