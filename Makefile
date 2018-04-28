CC=g++
CFLAGS=-g -c -Wall -std=c++14
LDFLAGS=
SOURCES=arc.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=arc

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o arc
