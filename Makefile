all: chat-server
  
chat-server: http-server.c chat-server.c
	gcc http-server.c chat-server.c -std=c11 -o chat-server -Wall -Wno-unused-variable -fsanitize=address -g

clean:
	rm -f chat-server
