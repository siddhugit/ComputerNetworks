# CS438 - spring 2017 MP1
#


CC=/usr/bin/g++
CC_OPTS=-g -Wall
CC_LIBS=-lpthread
CC_DEFINES=
CC_INCLUDES=
CC_SERVER_ARGS=${CC_OPTS} ${CC_DEFINES} ${CC_INCLUDES} ${CC_LIBS}
CC_CLIENT_ARGS=${CC_OPTS} ${CC_DEFINES} ${CC_INCLUDES}

# clean is not a file
.PHONY=clean

#target "all" depends on all others
all: http_client http_server

http_client: http_client.cpp tcp_socket.cpp  url_util.cpp
	@${CC} ${CC_OPTS} -o http_client http_client.cpp tcp_socket.cpp  url_util.cpp
	
http_server: http_server.cpp tcp_socket.cpp url_util.cpp
	@${CC} ${CC_OPTS} -o http_server http_server.cpp tcp_socket.cpp url_util.cpp ${CC_LIBS}

	
clean:
	@rm -f http_client http_server output *.o
