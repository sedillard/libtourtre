CC = gcc
CPPFLAGS = -I../../include
CFLAGS = -std=c99 -Wall -Werror -g

tltree : main.o trilinear.o 
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ -static -ltourtre -L../../ -I../../include -lm

trilinear.o : trilinear.c trilinear.h sglib.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean :
	-rm *.o tltree
