all: server.o main.o
	g++ -std=c++11 -o socks5server server.o main.o -g

server.o: server.h server.cpp
	g++ -std=c++11 -c server.cpp

main.o: server.h main.cpp
	g++ -std=c++11 -c main.cpp

.PHONY: clean
clean:
	rm -f *.o socks5server