
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <string.h>

#include "helpers.h"
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "common.h"
#include <complex>
#include <unistd.h>

// useful constructors for our structs
subscriberMessage::subscriberMessage(){
    
}

subscriberInfo::subscriberInfo(int newFd, bool isOnline){
    online = isOnline;
    fd = newFd;
}

subscriberMessage::subscriberMessage(int newType, char* newId){
    memset(topic, 0, 100);
    type = newType;
    strcpy(id, newId);
  }

subscriberMessage::subscriberMessage(int newType, char* newId, char* newTopic){
    memset(topic, 0, 100);
    type = newType;
    strcpy(id, newId);
    strcpy(topic, newTopic);
}
tcpMessage::tcpMessage(){

}
tcpMessage::tcpMessage(udpMessage udpMsg, sockaddr_in udpAddr){
    portUpd = udpAddr.sin_port;
    strcpy(ipUpd, inet_ntoa(udpAddr.sin_addr));

    memcpy(topic, udpMsg.topic, 50);

    if(strlen(udpMsg.topic) < 50){

        type = udpMsg.type;
    }
    else{
        type = 0;
        strcat(topic, "\n");
    }
    // strcpy doesnt work for info because it stops copying at /0
    // it took me 2 hours to debug this, end me
    memcpy(info, udpMsg.info, 1500);

}
tcpMessage::tcpMessage(string close){
    // this will mean that we're closing the server
    type = 4;
}


// returns the string resulted from an initial string and the reference
string getSubstring(substringReference ref, string myString){
    return myString.substr(ref.location, ref.size);
}

// parses a string into a vector of char*
vector<substringReference> parseString(string topicString){
    vector<substringReference> myVect;
    int pos = 0, prev = 0;
    string delimeter = "/";

    while(pos != (int)topicString.npos){
        if(pos != 0){
            if(prev == 0)
                myVect.push_back(substringReference{prev , pos - prev});
            else
                myVect.push_back(substringReference{prev + 1, pos - prev - 1});
        }        
        prev = pos;
        pos = topicString.find(delimeter, pos + 1);
    }
    
    myVect.push_back(substringReference{prev + 1, (int)(topicString.size() - prev - 2)});
    // for /n
    myVect.push_back(substringReference{(int)topicString.size() - 1, 1});
    return myVect;
}
// searches for a topic in the map of topics
string searchTopic(std::map<string, vector<substringReference>> elementList, string toBeFound){

    // if we find a topic that is exactly the same then return
    if(elementList.find(toBeFound) != elementList.end())
        return toBeFound;

    // split our word in substrings
    vector<substringReference> toBeFoundSubstr = parseString(toBeFound);
    string crtWordSubscriber, crtWordNewTopic;
    // search all stored topics
    for(auto i = elementList.begin(); i != elementList.end(); i++){
        if(i->first.compare("*\n") == 0)
            return "*";
        // check if it contains a wildcard
        // if it doesn't, there's no need to look inside of it
        if(i->first.find("*") != i->first.npos || i->first.find("+") != i->first.npos){

            int k = 0, j = 0;
            string nextAfterStar = "";
            // iterate through both topics
            while(k < (int)(toBeFoundSubstr.size() - 1) && j < (int)(i->second.size() - 1)){
                // get the current level
                crtWordSubscriber = getSubstring(i->second[j], i->first);
                crtWordNewTopic = getSubstring(toBeFoundSubstr[k], toBeFound);
                // if we have nothing in nextAfterStar that means we can check normally
                if(nextAfterStar == ""){
                    // if we found a * then we look inside the topic until the
                    // current word matches the one that follows *
                    if(crtWordSubscriber == "*"){
                        nextAfterStar = getSubstring(i->second[j + 1], i->first);
                    // the word isn't good
                    } else if((crtWordSubscriber != crtWordNewTopic && crtWordSubscriber != "+")
                    || (crtWordSubscriber == "+" && getSubstring(toBeFoundSubstr[k + 1], toBeFound) != getSubstring(i->second[j+1], i->first))){
                        j = 0;
                        break;
                    }
                    j++;
                    k++;
                }else{
                    // else, check if we found the end
                    if(nextAfterStar == crtWordNewTopic){
                        nextAfterStar = "";
                        j++;
                    }
                    k++;
                }
            }
            if(j == (int)(i->second.size() - 1))
                return i->first;
        }
    }
    return "";
}

size_t calculateSize(tcpMessage tcpMsg){
    // int
    if(tcpMsg.type == 0)
        return 5;
    //short real
    if(tcpMsg.type == 1)
        return 2;
    if(tcpMsg.type == 2)
        return 6;
    
    return strlen(tcpMsg.info);
}

// sends the notification to all the subscribers
void sendNotification(map<string, subscriberInfo> subscribers, tcpMessage tcpMsg){
  for(auto sub : subscribers){
    if(sub.second.online && searchTopic(sub.second.topicList, tcpMsg.topic) != ""){
        // send the size first
        size_t sizePachet = 2 + 2 + 16 + 51 + calculateSize(tcpMsg);

        send_all(sub.second.fd, &sizePachet, sizeof(size_t));
        send_all(sub.second.fd, &tcpMsg, sizePachet);
    }
  }
}

// notifies all subscribers that we're closing the server
void sendClosingNotification(map<string, subscriberInfo> subscribers){
    tcpMessage tcpMsg("closing packet");
  for(auto sub : subscribers){
    if(sub.second.online){
        size_t sizePachet = 2 + 2 + 16 + 51;
        send_all(sub.second.fd, &sizePachet, sizeof(size_t));
        send_all(sub.second.fd, &tcpMsg, sizePachet);
    }
  }
}

// removes the trailing zeroes for float
string removeZeroes(string number){
    if(number.find('.') != string::npos) {
        number = number.substr(0, number.find_last_not_of('0')+1);
        if(number.find('.') == number.size()-1)
            number = number.substr(0, number.size()-1);
    }
    return number;
}

void showNotification(tcpMessage tcpMsg){
    // make sure the strings end well
    tcpMsg.topic[strlen(tcpMsg.topic) - 1] = '\0';
    tcpMsg.topic[49] = '\0';
    tcpMsg.topic[1499] = '\0';
    // int
    if(tcpMsg.type == 0){
        uint8_t signByte = tcpMsg.info[0];
        uint32_t* numberAddr = (uint32_t*)(&(tcpMsg.info[1]));
        long long number = ntohl(*numberAddr);

        if(signByte == 0)
            printf("%s:%hu - %s - INT - %lld\n",
            tcpMsg.ipUpd, tcpMsg.portUpd, tcpMsg.topic, number);
        else
            printf("%s:%hu - %s - INT - %lld\n",
            tcpMsg.ipUpd, tcpMsg.portUpd, tcpMsg.topic, (number * (-1)));
    //short real
    } else if(tcpMsg.type == 1){
        double number = 1.0 * ntohs(*(uint16_t *)(tcpMsg.info)) / 100;
        printf("%s:%hu - %s - SHORT_REAL - %.2f\n",
        tcpMsg.ipUpd, tcpMsg.portUpd, tcpMsg.topic, number);
    // float        
    } else if(tcpMsg.type == 2){
        uint8_t signByte = tcpMsg.info[0];
        double number = ntohl(*(uint32_t *)(tcpMsg.info + 1)) / pow(10, tcpMsg.info[5]);
        
        if(signByte == 0)
            signByte = 1;
        else
            signByte = -1;
        string str = removeZeroes(to_string(number * signByte));
        char charArray[100];
        strcpy(charArray, str.c_str());
        printf("%s:%hu - %s - FLOAT - %s\n",
        tcpMsg.ipUpd, tcpMsg.portUpd, tcpMsg.topic, charArray);
    }// string
    else{
        printf("%s:%hu - %s - STRING - %s\n",
        tcpMsg.ipUpd, tcpMsg.portUpd, tcpMsg.topic, tcpMsg.info);
    }
}
