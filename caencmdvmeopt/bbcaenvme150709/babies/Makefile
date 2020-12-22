ALL: caenvmebabies

CFLAGS = -Wall -O2 -DLINUX
LOADLIBES = -lpthread -lCAENVME

libbabies.o : libbabies.h
libbbcaenvme.o : libbbcaenvme.h

MODULES = v792.o v1290.o

caenvmebabies : caenvmebabies.o libbabies.o libbbcaenvme.o bbpid.o $(MODULES)

clean :
	rm -f *.o
