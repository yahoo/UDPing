default: clean client server

CXX = g++
COMMONOBJ = options.o delay.o protocol.o stats.o 
CPPFLAGS = -I../include -I /home/y/include -O3
LDFLAGS = -L/home/y/lib64 -Wl,-rpath,/home/y/lib64
LIBS = -lrt -lm -lymonsb2

client: $(COMMONOBJ) client.o clientsession.o maclist.o ifinfo.o
	$(CXX) $(LDFLAGS) $(LIBS) -o udping_client client.o options.o delay.o protocol.o clientsession.o stats.o maclist.o ifinfo.o

server: $(COMMONOBJ) server.o serversession.o statswriter.o
	$(CXX) $(LDFLAGS) $(LIBS) -o udping_server server.o options.o delay.o protocol.o serversession.o stats.o statswriter.o

clean:
	rm -f udping_client udping_server *.o

