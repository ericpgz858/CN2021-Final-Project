all: 
	g++ -std=c++11 -Wall -Wextra server.cpp -o server
	g++ -std=c++11 -Wall -Wextra client.cpp -o client
server: server.cpp
	g++ -std=c++11 -Wall -Wextra server.cpp -o server

client: client.cpp
	g++ -std=c++11 -Wall -Wextra client.cpp -o client

clean:
	rm -f server
	rm -f client

