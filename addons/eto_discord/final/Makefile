CXX = g++

CXXFLAGS =-Wall -lpthread -lasound

all: client server

client: client2.cpp
	$(CXX) client2.cpp -o client $(CXXFLAGS)

server: server2.cpp
	$(CXX) server2.cpp -o server $(CXXFLAGS)

tool: tool.cpp
	$(CXX) tool.cpp -o tool $(CXXFLAGS)

clean:
	rm -rf *.wav tool client server

rebuild: clean client server