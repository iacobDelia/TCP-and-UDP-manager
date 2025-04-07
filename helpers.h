#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <arpa/inet.h>
/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#endif
#define EXIT 0
#define SUBSCRIBE 1
#define UNSUBSCRIBE 2

using namespace std;

// used for transmitting messages from the clients to the server
struct subscriberMessage{
  int type;
  char topic[100];
  char id[12];
  subscriberMessage();
  subscriberMessage(int newType, char* newId);
  subscriberMessage(int newType, char* newId, char* newTopic);
};

// used for separating the topic levels
struct substringReference{
    int location;
    int size;
};

// used to store subscriber info
struct subscriberInfo{
  map<string, vector<substringReference>> topicList;
  bool online;
  int fd;
  subscriberInfo(int newFd, bool isOnline);
};

// used for the messages from the udp clients to the server
struct udpMessage{
	char topic[50];
	uint8_t type;
	char info[1500];
}__attribute__((packed));

// used for the messages from the server to the tcp clients
struct tcpMessage{
  uint8_t type;
  uint16_t portUpd;
  char ipUpd[16];
  char topic[51];
  char info[1500];
  tcpMessage();
  tcpMessage(string close);
  tcpMessage(udpMessage udpMsg, sockaddr_in udpAddr);
};

// returns the string resulted from an initial string and the reference
string getSubstring(substringReference ref, string myString);

// parses a string into a vector of char*
vector<substringReference> parseString(string topicString);

// searches for a topic in the map of topics
string searchTopic(std::map<string, vector<substringReference>> elementList, string toBeFound);

// notifies all subscribers that we're closing the server
void sendNotification(map<string, subscriberInfo> subscribers, tcpMessage tcpMsg);

void sendClosingNotification(map<string, subscriberInfo> subscribers);

void showNotification(tcpMessage tcpMsg);