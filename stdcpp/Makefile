all: server.o main.o testclient.o threadpool.o
	g++ -std=c++11 -pthread -o socks5server server.o main.o threadpool.o -g
	g++ -std=c++11 -o testclient testclient.o -g
	rm -f server.o main.o testclient.o threadpool.o

server.o: server.h server.cpp
	g++ -std=c++11 -c server.cpp

main.o: server.h main.cpp
	g++ -std=c++11 -c main.cpp

testclient.o: testclient.cpp
	g++ -std=c++11 -c testclient.cpp

threadpool.o: threadpool.h threadpool.cpp
	g++ -std=c++11 -c threadpool.cpp

.PHONY: clean
clean:
	rm -f socks5server testclient server.o main.o testclient.o threadpool.o