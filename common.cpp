#include "common.h"
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/types.h>

// receives len bytes and keeps them in buffer
int recv_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = (char*) buffer;

    while(bytes_remaining) {
      // receiving starting from buff + received
      ssize_t lenMsg = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
      if(lenMsg == -1){
        printf("recv failed\n");
        break;
      }
      bytes_received = bytes_received + lenMsg;
      bytes_remaining = bytes_remaining - lenMsg;
    }
  return bytes_received;
}

// sends len bytes from buffer
int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = (char*)buffer;
  
    while(bytes_remaining) {
      ssize_t lenMsg = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
      if(lenMsg == -1){
        printf("send failed\n");
        break;
      }
      bytes_sent = bytes_sent + lenMsg;
      bytes_remaining = bytes_remaining - lenMsg;
    }

  return bytes_sent;
}
