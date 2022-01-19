//
// Created by carson on 5/20/20.
//

#include "stdio.h"
#include "stdlib.h"
#include "server.h"
#include "char_buff.h"
#include "game.h"
#include "repl.h"
#include "pthread.h"
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h>    //inet_addr
#include<unistd.h>    //write

game_server *SERVER;
static pthread_mutex_t * LOCK = NULL;

int handle_client_connect(int player) {
    int read_size;
    int client_sock = SERVER->player_sockets[player];

    char raw_buffer[2000];
    struct char_buff *out_buffer = cb_create(2000);
    struct char_buff *in_buffer = cb_create(2000);

    cb_append(out_buffer, "Welcome to the battleBit server Player ");
    cb_append_int(out_buffer, player + 1);
    cb_append(out_buffer, "\n");

    cb_write(client_sock, out_buffer);

    cb_reset(out_buffer);

    cb_append(out_buffer, "\nbattleBit (? for help) > ");

    cb_write(client_sock, out_buffer);

    while ((read_size = recv(client_sock, raw_buffer, 2000, 0)) > 0) {
        pthread_mutex_lock(LOCK);
        cb_reset(in_buffer);
        cb_reset(out_buffer);
        raw_buffer[read_size] = '\0'; // null terminate the buffer
        cb_append(in_buffer, raw_buffer);

        char *command = cb_tokenize(in_buffer, " \r\n");
        if (command) {
            if (strcmp(command, "exit") == 0) {
                cb_append(out_buffer, "Goodbye!\n\n");
                write(client_sock, out_buffer->buffer, strlen(out_buffer->buffer));
                close(client_sock);
                break;
            } else if (strcmp(command, "say") == 0) {

                cb_append(out_buffer, "\nPlayer ");
                cb_append_int(out_buffer, player + 1);
                cb_append(out_buffer, " says: ");
                cb_append(out_buffer, in_buffer->buffer + 4);
                cb_append(out_buffer, "\nbattleBit (? for help) > ");

                server_broadcast(out_buffer);

            } else if (strcmp(command, "show") == 0) {

                repl_print_board(game_get_current(), player, out_buffer);
                cb_append(out_buffer, "\nbattleBit (? for help) > ");
                cb_write(client_sock, out_buffer);

            } else if (strcmp(command, "load") == 0) {

                char *arg1 = cb_next_token(in_buffer);
                game_load_board(game_get_current(), player, arg1);

                cb_reset(out_buffer);
                repl_print_board(game_get_current(), player, out_buffer);
                cb_append(out_buffer, "\nbattleBit (? for help) > ");

                cb_write(SERVER->player_sockets[player], out_buffer);

            } else if (strcmp(command, "fire") == 0) {
                char *arg1 = cb_next_token(in_buffer);
                char *arg2 = cb_next_token(in_buffer);
                int x = atoi(arg1);
                int y = atoi(arg2);

                cb_reset(out_buffer);
                if (x < 0 || x >= BOARD_DIMENSION || y < 0 || y >= BOARD_DIMENSION) {
                    cb_append(out_buffer, "Invalid coordinate");
                } else {
                    cb_reset(out_buffer);
                    cb_append(out_buffer, "Player ");
                    cb_append_int(out_buffer, player + 1);
                    cb_append(out_buffer, " fired at ");
                    cb_append_int(out_buffer, x);
                    cb_append(out_buffer, ", ");
                    cb_append_int(out_buffer, y);
                    int result = game_fire(game_get_current(), player, x, y);
                    if (result) {
                        cb_append(out_buffer, " -- HIT!");
                    } else {
                        cb_append(out_buffer, " -- Miss\n\n");
                    }
                    cb_append(out_buffer, "\nbattleBit (? for help) > ");

                    // broadcast results
                    server_broadcast(out_buffer);

                }

            } else if(strcmp(command, "?") == 0) {
                cb_reset(out_buffer);
                cb_append(out_buffer,"? - show help\n");
                cb_append(out_buffer,"load <string> - load a ship layout\n");
                cb_append(out_buffer,"show - shows the board \n");
                cb_append(out_buffer,"fire [0-7] [0-7] - fires at the given position\n");
                cb_append(out_buffer,"say <string> - Send the string to all players as part of a chat\n");
                cb_append(out_buffer,"exit\n");
                cb_append(out_buffer, "\nbattleBit (? for help) > ");
                write(client_sock, out_buffer->buffer, strlen(out_buffer->buffer));
            } else {
                cb_reset(out_buffer);
                cb_append(out_buffer, "Did not understand command: ");
                cb_append(out_buffer, command);
                cb_append(out_buffer, "\nbattleBit (? for help) > ");
                write(client_sock, out_buffer->buffer, strlen(out_buffer->buffer));

            }
        } else {
            cb_reset(out_buffer);
            cb_append(out_buffer, "\nbattleBit (? for help) > ");
        }
        pthread_mutex_unlock(LOCK);
    }

    if (read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    } else if (read_size == -1) {
        perror("recv failed");
    }

    cb_free(in_buffer);
    cb_free(out_buffer);
}

void server_broadcast(char_buff *out_buffer) {
    printf("%s", out_buffer->buffer);
    fflush(stdout);
    cb_write(SERVER->player_sockets[0], out_buffer);
    cb_write(SERVER->player_sockets[1], out_buffer);
}

int run_server() {
    int socket_desc, client_sock;
    struct sockaddr_in server, client;

    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
    }

    int yes = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(BATTLEBIT_PORT);

    //Bind
    if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc, 1);

    //Accept incoming connection
    puts("Waiting for incoming connections...");
    int c = sizeof(struct sockaddr_in);

    while (SERVER->player_sockets[0] == 0 || SERVER->player_sockets[1] == 0) {

        //accept connection from an incoming client
        client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t *) &c);
        if (client_sock < 0) {
            perror("accept failed");
            return 1;
        }
        puts("Connection accepted");
        if (SERVER->player_sockets[0] == 0) {
            SERVER->player_sockets[0] = client_sock;
            pthread_create(&SERVER->player_threads[0], NULL, (void *) handle_client_connect, (void *) 0);
        } else {
            SERVER->player_sockets[1] = client_sock;
            pthread_create(&SERVER->player_threads[1], NULL, (void *) handle_client_connect, (void *) 1);
        }
    }

    close(socket_desc);
}

int server_start() {
    if (SERVER == NULL) {
        SERVER = server_create();
        pthread_create(&SERVER->server_thread, NULL, (void *) run_server, NULL);
        LOCK = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(LOCK, NULL);
    } else {
        printf("Server already started");
    }
}

struct game_server *server_create() {
    struct game_server *server = malloc(sizeof(struct game_server));
    server->server_thread = 0;
    server->player_threads[0] = 0;
    server->player_threads[1] = 0;
    server->player_sockets[0] = 0;
    server->player_sockets[1] = 0;
    return server;
}
