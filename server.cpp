
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
#include <ctype.h>
#include <netinet/tcp.h>
#include <iostream>

#include "common.h"
#include "helpers.h"
#include <vector>
#include <map>

using namespace std;

void server(int tcpfd, int udpfd)
{
  // a vector for managing the fds
  vector<struct pollfd> fds;
  
  bool isRunning = true;
  // put in our ports
  fds.push_back(pollfd{tcpfd, POLLIN, 0});
  fds.push_back(pollfd{udpfd, POLLIN, 0});
  // also put in the fd for stdin
  fds.push_back(pollfd{STDIN_FILENO, POLLIN, 0});

  // map for subscribers and their ids
  map<string, subscriberInfo> subscribers;
  int rc;

  // tcpfd will listen
  rc = listen(tcpfd, 2);
  DIE(rc < 0, "listen");

  while (isRunning)
  {
    int rc = poll(&fds[0], fds.size(), -1);
    DIE(rc < 0, "poll failure");

    // look through our list of sockets
    for(uint64_t i = 0; i < fds.size(); i++){
      // check if something interesting happened
      if(fds[i].revents & POLLIN){
          // check if it's tcp
          // this can only mean a new client is trying to connect
          if(fds[i].fd == tcpfd){
            // receive message
            struct sockaddr_in tcpAddr;
            socklen_t tcpLen = sizeof(tcpAddr);

            // accept the connection
            int fd = accept(tcpfd, (struct sockaddr*)&tcpAddr, &tcpLen);
            DIE(fd < 0, "accept() failed");

            // set the options for this socket
            int enable = 1;
            setsockopt(fd, IPPROTO_TCP, SO_REUSEADDR | TCP_NODELAY, &enable, sizeof(int));
            DIE(fd < 0, "setsockopt fail");

            // add the client to the poll
            fds.push_back(pollfd{fd, POLLIN, 0});

            // receive the client's id
            char id[10];
            recv_all(fd, id, 10);

            // check if we have the client's id inside the map
            if(subscribers.find(id) == subscribers.end()){
              // if not, add it
              subscribers.insert({id, subscriberInfo(fd, true)});
              // show a cute little message
              printf("New client %s connected from %s:%hu.\n", id, inet_ntoa(tcpAddr.sin_addr), ntohs(tcpAddr.sin_port));
            }
            else{
              // if the client is already connected
              // we delete the fd we added dearlier out of our poll
              auto subscriber = subscribers.at(id);

              if(subscriber.online == true){
                printf("Client %s already connected.\n", id);

                // send a close connection packet to the subscriber
                tcpMessage tcpMsg("closing packet");
                size_t toBeSent = sizeof(tcpMessage);
                send_all(fd, &toBeSent, sizeof(size_t));
                send_all(fd, &tcpMsg, toBeSent);

                // remove it from the vector with file descriptors and close the connection
                fds.pop_back();
                close(fd);
              }
              // welcome back
              else{
                subscriber.online = true;
                subscriber.fd = fd;
                printf("New client %s connected from %s:%hu.\n", id, inet_ntoa(tcpAddr.sin_addr), ntohs(tcpAddr.sin_port));
              }
            }
          }
          // check if it's udp
          else if(fds[i].fd == udpfd){
            // receive message
            struct sockaddr_in udpAddr;
            socklen_t udpLen = sizeof(udpAddr);

            udpMessage notification;

            memset(&notification, 0, sizeof(udpMessage));
            int rec = recvfrom(udpfd, &notification, sizeof(notification), 0, (sockaddr*)&udpAddr, &udpLen);
            DIE(rec < 0, "recvfrom failed");
            strcat(notification.topic, "\n");

            // send the notification to all the people subscribed to this topic
            sendNotification(subscribers, tcpMessage(notification, udpAddr));
        // check if it's stdin
        }else if(fds[i].fd == STDIN_FILENO){
          char command[1551];
          fgets(command, 1550, stdin);
          if(strcmp(command, "exit\n") == 0){
            // send something to all connected subscribers to tell them we're closing
            sendClosingNotification(subscribers);
            isRunning = false;
          }
          else{
            fprintf(stderr, "Usage: exit\n");
          }
        }
        // message from one of the clients
        else{
          subscriberMessage message;
          recv_all(fds[i].fd, &message, sizeof(message));

          if(message.type == EXIT){
            printf("Client %s disconnected.\n", message.id);
            subscribers.at(message.id).online = false;

            // delete the fd out of the poll
            close(fds[i].fd);
            fds.erase(fds.begin() + i);
          } else if(message.type == SUBSCRIBE){
              // add the subscribed topic
              subscribers.at(message.id).topicList.insert({message.topic, parseString(message.topic)});
          } else if(message.type == UNSUBSCRIBE){
            // if the topic exists in the subscriber's database, remove it
            if(subscribers.at(message.id).topicList.find(message.topic) != subscribers.at(message.id).topicList.end()){
              subscribers.at(message.id).topicList.erase(message.topic);
            }
          }
        }
      }
    }
  }

  // close the created sockets
  for(auto &i : fds){
        close(i.fd);
  }
}


int main(int argc, char *argv[])
{
  // incorrect number of arguments
  if (argc != 2)
  {
    fprintf(stderr, "\n Usage: ./server <PORT>\n");
    return 1;
  }

  // disable stdout buffer
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  // parse the port
  uint16_t port;
  int rc = sscanf(argv[1], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  // TCP socket
  const int tcpfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(tcpfd < 0, "tcpfd failed");

  const int udpfd = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(udpfd < 0, "udpfd failed");

  // complete in serv_addr the server address, family, and port
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  // associate the address with the server
  rc = bind(tcpfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "bind tcp failed");

  rc = bind(udpfd, (const struct sockaddr*)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "bind udp failed");
  server(tcpfd, udpfd);

  return 0;
}
