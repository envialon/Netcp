
NetCp:Netcp.cc Socket.o File.o
	g++ -std=c++17 -g -Wall -o Netcp Netcp.cc File.o Socket.o -pthread

Socket.o: Socket.cc
	g++ -c Socket.cc

File.o: File.cc
	g++ -c File.cc

clean:
	rm Socket.o File.o Netcp