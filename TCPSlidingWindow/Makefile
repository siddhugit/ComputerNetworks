all: reliable_sender reliable_receiver

reliable_sender: sender_main.cpp udp_socket.cpp
	g++ -g -o reliable_sender sender_main.cpp udp_socket.cpp

reliable_receiver: receiver_main.cpp udp_socket.cpp
	g++ -g -o reliable_receiver receiver_main.cpp udp_socket.cpp

clean:
	rm reliable_sender reliable_receiver
