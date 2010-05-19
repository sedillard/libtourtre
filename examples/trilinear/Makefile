CC = gcc
CPPFLAGS = -I../../include
CFLAGS = -std=c99 -Wall -Werror -O2

tltree : main.o trilinear.o 
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^  -ltourtre -L../../ -I../../include -lm -lz

trilinear.o : trilinear.c trilinear.h sglib.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean :
	-rm *.o tltree
