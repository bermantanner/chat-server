#include "http-server.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define TIMESTAMP_SIZE 20
#define USERNAME_SIZE 16
#define MESSAGE_SIZE 256
#define REACTION_SIZE 16 
#define MAX_REACTIONS 100


int global_num = 0; 

char const HTTP_404_NOT_FOUND[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n";
char const HTTP_400_CLIENT_ERROR[] =  "HTTP/1.1 400 Client Error\r\nContent-Type: text/plain\r\n\r\n";
char const HTTP_200_OK[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n";

//------------------STRUCTS/RELATED

struct chat_reaction {
	char username[USERNAME_SIZE];
	char message[REACTION_SIZE];
};

struct chat_msg{
	int32_t msgID; 
	char timestamp[TIMESTAMP_SIZE]; 
	char message_content[MESSAGE_SIZE];
	char username[USERNAME_SIZE];
	uint32_t num_reactions;
       	struct chat_reaction *reactions_list;
};

struct chat_msg* chat_list = NULL;
int chat_list_count = 0; //this might be a little redundent, but i think it opens possiblity for other features.
int chat_list_cap = 100;

//------------------STRUCTS/RELATED

//------------------ERROR HANDLING
void handle_404(int client_sock, char *path)  {
    printf("SERVER LOG: Got request for unrecognized path \"%s\"\n", path);

    char response_buff[BUFFER_SIZE];
    snprintf(response_buff, BUFFER_SIZE, "Error 404:\r\nUnrecognized path \"%s\"\r\n", path);

    write(client_sock, HTTP_404_NOT_FOUND, strlen(HTTP_404_NOT_FOUND));
    write(client_sock, response_buff, strlen(response_buff));
}
void handle_400(int client_sock, char *path)  {
    printf("SERVER LOG: Client error \"%s\"\n", path);

    char response_buff[BUFFER_SIZE];
    snprintf(response_buff, BUFFER_SIZE, "Error 400:\r\nClient error \"%s\"\r\n", path);

    write(client_sock, HTTP_400_CLIENT_ERROR, strlen(HTTP_400_CLIENT_ERROR));
    write(client_sock, response_buff, strlen(response_buff));
}

//-----------------ERROR HANDLING

//-----------------TESTING

void test_print(int client_sock, char* text) {
	char message[BUFFER_SIZE];
	snprintf(message, BUFFER_SIZE, "TEST PRINT: %s\n", text);
	write(client_sock, HTTP_200_OK, strlen(HTTP_200_OK));  
 	write(client_sock, message, strlen(message));
}

//------------------TESTING

//------------------HELPER FUNCTIONS

int is_hex(char c) {
	if (c >= '0' && c <= '9') 
		return 1;
	else if (c >= 'a' && c <= 'f')
		return 1;
	else if (c >= 'A' && c <= 'F')
		return 1;
	return 0;	

}

int hex_to_int(char c) {
	if (c >= '0' && c <= '9') 
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;	
}

//Going to write this so it replaces the original place in memory with decoded version.
void url_decode(char* url_str) {
	char* input = url_str;
	char* output = url_str;
	char first_char;
	char second_char;

	while (*input != 0) { //While we haven't reached end of original input...
		if (*input == '%') {
			first_char = input[1];
			if (first_char == '\0') { //This catches the case where % is last character. (saves from crashing)
				return;
			}
			second_char = input[2];
			if (is_hex(first_char) && is_hex(second_char)) {
				*output = (hex_to_int(first_char) << 4) | (hex_to_int(second_char));
				output++;
				input += 3;
			}
			else { // not valid hex, just copy and continue 
				*output = *input;
				output++;
				input++;
			}
		}
		else {
			*output = *input;
			output++;
			input++;
		}
	}
	*output = '\0';
}

void free_everything() {
    // free each of the chat_msg's reactions_lists
    for (int i = 0; i < chat_list_count; i++) {
        if (chat_list[i].reactions_list != NULL) {
            free(chat_list[i].reactions_list);
            chat_list[i].reactions_list = NULL; 
        }
    }

    // Free the chat_list itself
    if (chat_list != NULL) {
        free(chat_list);
        chat_list = NULL; 
    }
}

void reset() {

    free_everything();

    chat_list_count = 0;
    global_num = 0;
    chat_list_cap = 100;

    // 
    chat_list = malloc(chat_list_cap * sizeof(struct chat_msg));
    if (chat_list == NULL) {
        perror("Failed to allocate memory during reset");
        exit(1); 
    }
}

//------------------HELPER FUNCTIONS

//------------------DATA HANDLING 
//Returning 0 means this failed
uint8_t add_chat(char* username, char* message) {
	//First we need to check if the list capacity is full, and needs to be expanded.
	if (chat_list_count >= chat_list_cap) {
		chat_list_cap = chat_list_cap * 2;
		struct chat_msg* new_array_space = realloc(chat_list, chat_list_cap * sizeof(struct chat_msg));
		if (new_array_space == NULL) { //This should never happen, but for robustness
			perror("Failed to reallocate memory for new_array_space"); 
			return 0;
		}
		chat_list = new_array_space;
	}
	
	if (global_num >= 100000) {
		return 0;
	}
	struct chat_msg msg;
	msg.num_reactions = 0;
	msg.reactions_list = malloc(MAX_REACTIONS * sizeof(struct chat_reaction));


	global_num += 1;
	chat_list_count += 1;
	msg.msgID = global_num;
	
	strncpy(msg.username, username, USERNAME_SIZE - 1);
	msg.username[USERNAME_SIZE - 1] = '\0';
	
	strncpy(msg.message_content, message, MESSAGE_SIZE - 1);
	msg.message_content[USERNAME_SIZE - 1] = '\0';
    	
	char buffer[100];
    	time_t now = time(NULL);
    	struct tm *tm_info = localtime(&now);
   	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
	strncpy(msg.timestamp, buffer, TIMESTAMP_SIZE);
	
	chat_list[chat_list_count - 1] = msg;
	
	return 1;
}

uint8_t add_reaction(char* username, char* message, char* id) {
	uint32_t id_number = atoi(id);
	
	if (id_number > chat_list_count || id_number == 0) {
		return 0;
	}
	if (chat_list[id_number - 1].num_reactions >= MAX_REACTIONS) {
    		return 0;
	}

	
	struct chat_reaction new_reaction;

	strncpy(new_reaction.username, username, USERNAME_SIZE - 1);
	new_reaction.username[USERNAME_SIZE - 1] = '\0';

	strncpy(new_reaction.message, message, REACTION_SIZE - 1);
	new_reaction.message[REACTION_SIZE - 1] = '\0';

	chat_list[id_number - 1].reactions_list[chat_list[id_number - 1].num_reactions] = new_reaction;
	chat_list[id_number - 1].num_reactions++;

	return 1;
}

//-----------------DATA HANDLING

//-----------------REQUEST/RESPONSE HANDLING

void respond_with_chats(int client_sock) {
	int i;
	int j;
	char formatted_message[BUFFER_SIZE] = {0};

	write(client_sock, HTTP_200_OK, strlen(HTTP_200_OK));  
	for (i = 0; i < chat_list_count; i++) {
		snprintf(formatted_message, BUFFER_SIZE, "[#%-4d %-19s] %15s: %s\n",chat_list[i].msgID, chat_list[i].timestamp, chat_list[i].username, chat_list[i].message_content);
		write(client_sock, formatted_message, strlen(formatted_message));
		if (chat_list[i].num_reactions > 0) {
			for (j = 0; j < chat_list[i].num_reactions; j++) { 
				snprintf(formatted_message, BUFFER_SIZE, "\t\t\t(%s)  %s\n", chat_list[i].reactions_list[j].username, chat_list[i].reactions_list[j].message);
				write(client_sock, formatted_message, strlen(formatted_message));
			}
		}
	}	
}

void handle_post(char* path, int client_sock) {
	char* user_ptr;
	char* delimiter_ptr;
	char* message_ptr;
	int length;
	char* username = NULL;
	char* message = NULL;

	
	delimiter_ptr = strstr(path, "&message=");
	if (delimiter_ptr == NULL) {
		handle_400(client_sock, path);
		goto cleanup;
	}
	
	//Grabbing username...
	user_ptr = strstr(path, "user=");
	user_ptr = user_ptr + 5;
	length = delimiter_ptr - user_ptr;
    username = malloc(length + 1);
	strncpy(username, user_ptr, length); 
	username[length] = '\0';

	test_print(client_sock, username);

	//Check if username is empty
	if (strlen(username) <= 0) {
		handle_400(client_sock, path);
		goto cleanup;
	}

	//Decoding username 
	url_decode(username);

	//Getting decoded length / enforcing byte limit
	length = strlen(username);
	if (length > USERNAME_SIZE - 1) {
		handle_400(client_sock, path);
		goto cleanup;
	}
	
	//Grabbing message...	
	message_ptr = delimiter_ptr + 9;
	length = strlen(message_ptr);
	message = malloc(length + 1);
	strncpy(message, message_ptr, length);
	message[length] = '\0';
	
	//Check if message is empty
	if (strlen(message) <= 0) {
		handle_400(client_sock, path);
		goto cleanup;
	}
	
	//Decoding message
	url_decode(message);
	
	//Getting decoded length / enforcing byte limit
	length = strlen(message);
	if (length > MESSAGE_SIZE - 1) {
		handle_400(client_sock, path);
		goto cleanup;
	}

	if (add_chat(username, message) == 0) {
		handle_404(client_sock, path);
		goto cleanup;
	}
	respond_with_chats(client_sock);

	cleanup:
		free(username);
		free(message);
}

// path is a string like "/react?user=joe&message=hi&id=3"
// I am going to have lots of duplicate code here, but running low on time.
void handle_reaction(char* path, int client_sock) {
	char* user_ptr;
	char* id_ptr;
	char* delimiter_ptr_1;
	char* delimiter_ptr_2;
	char* message_ptr;
	char* username = NULL;
	char* message = NULL;
	char* msgID = NULL;

	int length;

	
	delimiter_ptr_1 = strstr(path, "&message=");
	if (delimiter_ptr_1 == NULL) {
		handle_400(client_sock, path);
		goto cleanup;
	}
	delimiter_ptr_2 = strstr(delimiter_ptr_1, "&id=");
	if (delimiter_ptr_2 == NULL) {
		handle_400(client_sock, path);
		goto cleanup;
	}
	
	//Grabbing username...
	user_ptr = strstr(path, "user=");
	user_ptr = user_ptr + 5;
	length = delimiter_ptr_1 - user_ptr;
    username = malloc(length + 1);
	strncpy(username, user_ptr, length); 
	username[length] = '\0';

	//Check if username is empty
	if (strlen(username) <= 0) {
		handle_400(client_sock, path);
		goto cleanup;
	}

	//Decoding username 
	url_decode(username);

	//Getting decoded length / enforcing byte limit
	length = strlen(username);
	if (length > USERNAME_SIZE - 1) {
		handle_400(client_sock, path);
		goto cleanup;
	}
	
	//Grabbing message...	
	message_ptr = delimiter_ptr_1 + 9;
	length = delimiter_ptr_2 - message_ptr;
	message = malloc(length + 1);
	strncpy(message, message_ptr, length);
	message[length] = '\0';
	
	//Check if message is empty
	if (strlen(message) <= 0) {
		handle_400(client_sock, path);
		goto cleanup;
	}
	
	//Decoding message
	url_decode(message);
	
	//Getting decoded length / enforcing byte limit
	length = strlen(message);
	if (length > REACTION_SIZE - 1) {
		handle_400(client_sock, path);
		goto cleanup;
	}

	//Grabbing ID...
	id_ptr = delimiter_ptr_2 + 4;
	length = strlen(id_ptr);
	msgID = malloc(length + 1);
	strncpy(msgID, id_ptr, length);
	msgID[length] = '\0';

	if (!add_reaction(username, message, msgID)) {
		handle_400(client_sock, path);
   		return;		
	}	
	respond_with_chats(client_sock);

	cleanup:
		free(username);
		free(message);
		free(msgID);
}


void handle_response(char *request, int client_sock) {
    char path[256];

    printf("\nSERVER LOG: Got request: \"%s\"\n", request);

    // Parse the path out of the request line (limit buffer size; sscanf null-terminates)
    if (sscanf(request, "GET %255s", path) != 1) {
        printf("Invalid request line\n");
        return;
    }
    
    if(strncmp(path, "/post", 5) == 0) {
	if (strncmp(path, "/post?user=", 11) != 0) {
		handle_400(client_sock, path);
		return;
	}
	handle_post(path, client_sock);
	return;
    }
    else if(strncmp(path, "/react", 6) == 0) {
	if (strncmp(path, "/react?user=", 12) != 0) {
		handle_400(client_sock, path);
		return;
	}
	handle_reaction(path, client_sock);
	return;
    }
    else if(strcmp(path, "/chats") == 0) {
	respond_with_chats(client_sock);
	return;
    }
    else if (strncmp(path, "/reset", 6) == 0) {
	reset();
	return;
    } 

    else {
	handle_404(client_sock, path);
    }

    // strstr if there might be shared prefixes, like looking for "/post" in the PA
    // save strstr for later

}

//-----------------REQUEST/RESPONSE HANDLING

int main(int argc, char *argv[]) {
    
	chat_list = malloc(chat_list_cap * sizeof(struct chat_msg));
	if (chat_list == NULL) {
        	perror("Failed to allocate memory");
        	return 1;
   	}
	
	int port = 0;
       	if(argc >= 2) { // if called with a port number, use it
        	port = atoi(argv[1]);
	}

    start_server(&handle_response, port);
	free_everything();
}
