all: ls_router manager_send

ls_router: ls_router.cpp udp_socket.cpp commands.cpp topology.cpp Dijkstra.cpp
	g++ -g -Wall -o ls_router ls_router.cpp udp_socket.cpp commands.cpp topology.cpp Dijkstra.cpp -lpthread

manager_send: manager_send.c
	gcc -o manager_send manager_send.c

clean:
	rm ls_router manager_send
