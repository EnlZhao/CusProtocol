myPacket.o: 
	g++ -c myPacket/myPacket.cc -o ./myPacket/myPacket.o

client.o: myPacket.o client/client.cc
	g++ -c client/client.cc -o ./client/client.o

server.o: myPacket.o server/server.cc
	g++ -c server/server.cc -o ./server/server.o

all: client.o server.o myPacket.o
	mkdir -p exefile
	g++ myPacket/myPacket.o client/client.o -o ./exefile/client.exe -lpthread -lwsock32
	g++ myPacket/myPacket.o server/server.o -o ./exefile/server.exe -lpthread -lwsock32

clean:
	rm -f **/*.o
	rm -f exefile/*

help:
	@echo "make all: compile all files"
	@echo "make clean: clean all .o files and exefile/*"