
NetCp: ./src/Netcp.cc Socket.o File.o
	g++ -std=c++17 -g -Wall -o Netcp Netcp.cc File.o Socket.o -pthread

Socket.o: ./src/Socket.cc
	g++ -c ./src/Socket.cc

File.o: ./src/File.cc
	g++ -c ./src/File.cc

clean:
	rm Socket.o File.o ./src/Netcp