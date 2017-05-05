default: test

CXX = g++
CXXOBJ = test.o t_options.o t_maclist.o t_protocol.o
LIBOBJ = ../src/maclist.o ../src/protocol.o ../src/options.o
CPPFLAGS = -I../include -I/home/y/include -g
LDFLAGS = -L/home/y/lib64 -Wl,-rpath,/home/y/lib64
LIBS = -lrt -lm -lcppunit

clean:
	rm -f *.o test results.xml

test: $(CXXOBJ)
	$(CXX) $(LDFLAGS) $(LIBS) -o test $(CXXOBJ) $(LIBOBJ)
	./test
	cat results.xml

t_maclist: $(CXXOBJ) ../src/maclist.o
	$(CXX) $(LDFLAGS) $(LIBS) -o t_maclist t_maclist.o ../src/maclist.o
