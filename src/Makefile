default: client server

CXX = g++
COMMONOBJ = options.o delay.o protocol.o stats.o 
CPPFLAGS = -I../include -I /home/y/include -g
LDFLAGS = 
LIBS = -lrt -lm

client: $(COMMONOBJ) client.o clientsession.o maclist.o ifinfo.o client.o options.o delay.o protocol.o
	$(CXX) $(LDFLAGS) $(LIBS) -o udping_client client.o options.o delay.o protocol.o clientsession.o maclist.o ifinfo.o

server: $(COMMONOBJ) server.o serversession.o statswriter.o delay.o options.o protocol.o stats.o
	$(CXX) $(LDFLAGS) $(LIBS) -o udping_server server.o delay.o options.o protocol.o serversession.o stats.o statswriter.o

clean:
	rm -f udping_client udping_server *.o

