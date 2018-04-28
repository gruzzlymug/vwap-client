CC=g++
CFLAGS=-g -I.
arc: arc.o
	#$(CC) -o arc arc.o $(CFLAGS)
	$(CC) -std=c++14 arc.o -o arc

arc.o: arc.cpp
	$(CC) -c -std=c++14 arc.cpp

