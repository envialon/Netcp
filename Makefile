
NetCp:Netcp.cc Socket.o File.o
	g++ -std=c++14 -g -Wall -o Netcp Netcp.cc File.o Socket.o -pthread

Socket.o: Socket.cc
	g++ -c Socket.cc

File.o: File.cc
	g++ -c File.cc

clean:
	rmdir out
	rm Socket.o File.o NetcpRecieve NetcpSend output.txt
