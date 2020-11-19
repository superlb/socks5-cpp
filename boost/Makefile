all: server.o main.o
	g++ -std=c++11 -pthread -o socks5server server.o main.o -g
	rm -f server.o main.o

server.o: server.h server.cpp
	g++ -std=c++11 -c server.cpp

main.o: server.h main.cpp
	g++ -std=c++11 -c main.cpp

.PHONY: clean
clean:
	rm -f socks5server