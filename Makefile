CC=clang++
CCFLAGS=-std=c++14 -O3 -Wall -Werror -Wpedantic
LDPATH=/usr/lib
INCPATH=/usr/include

OBJ=main.o bloom.o
HEADERS=bloom.hh
SRC=$(OBJ:.o=.cc)
LIB=libbloom.so
TARGET=bloom-test

all: CCFLAGS += -fPIC
all: $(OBJ) $(LIB)

install:
	cp $(LIB) $(LDPATH)
	cp $(HEADERS) $(INCPATH)

uninstall:
	cd $(LDPATH) && rm -f $(LIB)
	cd $(INCPATH) && rm -f $(HEADERS)

%.o: %.cc
	$(CC) -c $(CCFLAGS) -o $@ $<

lib%.so: %.o
	$(CC) -shared -Wl -o $@ $<

test: $(OBJ)
	$(CC) -o $(TARGET) $(OBJ)

clean:
	rm -f *.o *.so $(TARGET)
