#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <netinet/tcp.h>
#include <ctype.h>

#include "common.h"
#include "helpers.h"
#include <vector>
#include <iostream>

using namespace std;

void subscriber(int sock, char* id){
    char command[1551];
    // send the id to the server
    send_all(sock, id, 10);

    // a vector for managing the fds
    vector<struct pollfd> fds;
    
    // put in our ports
    fds.push_back(pollfd{sock, POLLIN, 0});
    // also put in the fd for stdin
    fds.push_back(pollfd{STDIN_FILENO, POLLIN, 0});

    while(1){
        int rc = poll(&fds[0], fds.size(), -1);
        DIE(rc < 0, "poll failure");
        // stdin notification
        if(fds[1].revents & POLLIN){
            memset(command, 0, 1551);
            fgets(command, 1550, stdin);
            if(strcmp(command, "exit\n") == 0){
                subscriberMessage newMsg(EXIT, id);
                send_all(sock, &newMsg, sizeof(subscriberMessage));
                close(sock);
                break;
            } else if(strncmp(command, "subscribe", 9) == 0){
                // copy the topic
                char temp[100];
                strcpy(temp, command + 10);

                subscriberMessage newMsg(SUBSCRIBE, id, temp);
                send_all(sock, &newMsg, sizeof(subscriberMessage));
                // delete the /n for printf
                temp[strlen(temp) - 1] = '\0';
                printf("Subscribed to topic %s.\n", temp);
            } else if(strncmp(command, "unsubscribe", 11) == 0){
                 // copy the topic
                char temp[100];
                strcpy(temp, command + 12);
                
                subscriberMessage newMsg(UNSUBSCRIBE, id, temp);
                send_all(sock, &newMsg, sizeof(subscriberMessage));
                // delete the /n for printf
                temp[strlen(temp) - 1] = '\0';
                printf("Unsubscribed from topic %s.\n", temp);
            } else{
                fprintf(stderr, "Usage:\nsubscribe <TOPIC> \nunsubscribe <TOPIC> \nexit \n");
            }
        }
        // server notification
        else if(fds[0].revents & POLLIN){
            tcpMessage tcpMsg;
            tcpMsg = {};
            
            size_t toBeReceived;
            recv_all(sock, &toBeReceived, sizeof(size_t));
            recv_all(sock, &tcpMsg, toBeReceived);
            if(tcpMsg.type == 4){
                break;
            }else showNotification(tcpMsg);
        }
    }
}

int main(int argc, char *argv[]){
    if(argc != 4){
        fprintf(stderr, "\n Usage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER> \n");
        return 1;
    }
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // parse the server port
    uint16_t port;
    int rc = sscanf(argv[3], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // complete in serv_addr the server address, family, and port
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);
    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    inet_aton(argv[2], &(serv_addr.sin_addr));

    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    setsockopt(sock, IPPROTO_TCP, SO_REUSEADDR | TCP_NODELAY, &enable, sizeof(int));
    DIE(sock < 0, "setsockopt fail");

    rc = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    subscriber(sock, argv[1]);
    close(sock);

}