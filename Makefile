all: common equipment server 

equipment: equipment.c
	gcc -Wall equipment.c common.o -o equipment

server: server.c
	gcc -Wall server.c common.o -o server

common: common.c
	gcc -Wall -c common.c

clean:
	rm common.o equipment server

run_server_v4:
	./server v4 51511

run_equipment_v4:
	./equipment 127.0.0.1 51511

run_server_v6:
	./server v6 51511
	
run_equipment_v6:	
	./equipment ::1 51511