CC = gcc
CPPFLAGS = -I./include
CFLAGS = -ansi -pedantic -Wall -Werror -fPIC -O2

AR = ar
ARFLAGS = -r

all : shared static

shared : libtourtre.so

static : libtourtre.a

objs =  src/tourtre.o     \
	src/ctArc.o       \
	src/ctBranch.o    \
	src/ctComponent.o \
	src/ctNode.o      \
	src/ctQueue.o     \
	src/ctNodeMap.o

libtourtre.a : $(objs)
	$(AR) $(ARFLAGS) $@ $^
	
libtourtre.so : $(objs)
	$(CC) -shared -o $@ $^

src/tourtre.o : src/tourtre.c include/tourtre.h src/ctMisc.h include/ctArc.h include/ctNode.h src/ctComponent.h include/ctNode.h src/ctQueue.h src/ctAlloc.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

src/ctArc.o : src/ctArc.c include/tourtre.h src/ctMisc.h include/ctArc.h 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

src/ctBranch.o : src/ctBranch.c include/tourtre.h src/ctMisc.h include/ctBranch.h 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

src/ctComponent.o : src/ctComponent.c include/tourtre.h src/ctMisc.h src/ctComponent.h 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

src/ctNode.o : src/ctNode.c include/tourtre.h src/ctMisc.h include/ctNode.h 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

src/ctQueue.o : src/ctQueue.c include/tourtre.h src/ctMisc.h src/ctQueue.h 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

src/ctNodeMap.o : src/ctNodeMap.c src/ctNodeMap.h include/ctNode.h src/ctQueue.h src/sglib.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean :
	-rm -rf src/*.o libtourtre.a libtourtre.so doc/html

	
# src/test : 	libtourtre.a test/test.c
# 	$(CC) $(CPPFLAGS) $(CFLAGS) -o test/test test/test.c -L. -ltourtre
# 
# test: src/test
# 	test/test
