CXX = g++

all: server.out client.out

server.out: server.cpp
	$(CXX) -o $@ $<

client.out: client.cpp
	$(CXX) -o $@ $<
	
clean:
	rm -f server.out client.out