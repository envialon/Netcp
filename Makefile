#Cursed ass makefile made by hand but it works and im not doing
#anything more complicated than this


NetCp: ./src/Netcp.cpp Socket.o File.o 
	g++ -std=c++17 -g -Wall -o Netcp ./src/Netcp.cpp File.o Socket.o -pthread

Socket.o: ./src/Socket.cpp
	g++ -c ./src/Socket.cpp 

File.o: ./src/File.cpp
	g++ -c ./src/File.cpp 

cleanObjects: 
	rm Socket.o File.o 

clean: 
	rm Socket.o File.o Netcp