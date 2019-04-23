CC=g++
CFLAGS=-g -c -Wall -std=c++17

LDFLAGS=-pthread
CLIENT_SOURCES=bot/start_starc.cpp bot/starc.cpp mt_bot/arc.cpp
CLIENT_OBJECTS=$(CLIENT_SOURCES:.cpp=.o)
CLIENT_EXECUTABLE=start_starc

SERVER_SOURCES=server/start_server.cpp server/server.cpp
SERVER_OBJECTS=$(SERVER_SOURCES:.cpp=.o)
SERVER_EXECUTABLE=start_server

all: $(CLIENT_SOURCES) $(CLIENT_EXECUTABLE) $(SERVER_SOURCES) $(SERVER_EXECUTABLE)

$(CLIENT_EXECUTABLE): $(CLIENT_OBJECTS)
	$(CC) $(LDFLAGS) $(CLIENT_OBJECTS) -o $@

$(SERVER_EXECUTABLE): $(SERVER_OBJECTS)
	$(CC) $(LDFLAGS) $(SERVER_OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm bot/*.o
	rm mt_bot/*.o
	rm server/*.o
	rm $(CLIENT_EXECUTABLE) $(SERVER_EXECUTABLE)
