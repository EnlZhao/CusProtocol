myPacket.o: myPacket/myPacket.cc myPacket/myPacket.hh
	g++ -c myPacket/myPacket.cc -o ./myPacket/myPacket.o -lpthread -lwsock32

client.o: myPacket.o
	g++ -c client/client.cc -o ./client/client.o -lpthread -lwsock32 

server.o: myPacket.o
	g++ -c server/server.cc -o ./server/server.o -lpthread -lwsock32

all: client.o server.o
	mkdir -p exefile
	g++ client/client.o myPacket/myPacket.o -o ./exefile/client.exe -lpthread -lwsock32
	g++ server/server.o myPacket/myPacket.o -o ./exefile/server.exe -lpthread -lwsock32

clean:
	rm -f ./**/*.o
	rm -f ./**/*.exe