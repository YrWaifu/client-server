CXX = g++

CXXFLAGS =-Wall -lpthread -lasound

all: client server

client: client.cpp
	$(CXX) client.cpp -o client $(CXXFLAGS)

server: server.cpp
	$(CXX) server.cpp -o server $(CXXFLAGS)

tool: tool.cpp
	$(CXX) tool.cpp -o tool $(CXXFLAGS)

clean:
	rm -rf *.wav tool client server

rebuild: clean client server