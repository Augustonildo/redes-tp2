all: common equipment server 

equipment: equipment.c
	gcc -Wall equipment.c -lpthread common.o -o equipment

server: server.c
	gcc -Wall server.c common.o -lpthread -o server

common: common.c
	gcc -Wall -c common.c

clean:
	rm common.o equipment server

run_server:
	./server 51511

run_equipment:
	./equipment 127.0.0.1 51511