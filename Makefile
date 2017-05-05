default: clean local test

screwdriver: clean local test

test:
	cd tests; make

local:
	cd src; make
	cp src/udping_client src/udping_server bin

clean:
	cd src; make clean
	cd tests; make clean
	cd bin; rm -f udping_client udping_server
