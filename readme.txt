# APPLICATION FOR MANAGING TCP AND UDP MESSAGES

This project aims to implement a server between multiple TCP and UDP clients.
The TCP clients serve the role of subscribers, while the UDP ones are the information providers,
sending datagrams to the server.

## SERVER FLOW
The server starts two sockets:
- a TCP socket that listens for any upcoming subscribers
- a UDP socket for the clients

These two sockets, alongside the stdin socket, are added to a vector that is used for multiplexing using `poll`.

The server then enters a while loop, where we check whether or not an event has happened for one of the sockets inside the vector:
### The TCP socket
If an event is happening for the tcp socket, that means that a new subscriber is trying to connect.
As such, we add its file descriptor to the poll vector, and wait for the subscriber to send its ID.
- if its ID is already present inside the subscribers map, we delete it out of the socket vector and close the newly created socket
- if its ID is not present inside the subscribers map, we continue by adding the new subscriber to it

### The subscriber sockets
The server can receive an `exit`, `subscribe` or `unsubscribe` command from one of the clients.
Since the message also carries the subscriber's id, the algorithm can look up the client in the map and update its information.
Each subscriber instance carries information about the topics it's subscribed to,
in the form of the `map<string, vector<substringReference>> topicList` variable.
To save space, each string is correlated to a vector of `substringReference`, which contains information about where each level starts and how long it is.
For example, an entry in the map can look like:
`upb/ec/temperature/1 -> 0 - 3; 4 - 2; 7 - 11; 12 - 1`

### The UDP socket
When a UDP client sends a datagram, each online subscriber is checked using the `searchTopic` function,
which iterates through the list of the topics a client is subscribed to.
Each topic is parsed into substrings using its corresponding `substringReference` entry and,
if it contains wildcards, it is then checked to see if it matches the current topic.

When one match is found, the function returns and the server generates a `tcpMessage`, where all the information necessary for showing the notification is stored.
Depending on its type, the server will first send the length of the message to the subscriber, then the message itself.

### stdin
The only acceptable command is `exit`, which causes the algorithm to send a closing notification to all the connected subscribers.
The program then breaks out of the loop and closes all the sockets.

## SUBSCRIBER FLOW
The server also starts its own socket, sends off its ip to the server, then waits for an event coming from it or stdin.
### stdin
The subscriber generates and sends a packet to the server containing its type and id, and if the command is of the `subscribe` or `unsubscribe` kind, it also includes the topic.

### the TCP socket
The subscriber shows its given notification or breaks out of the while loop and closes the socket if the `exit` command is given from the server.

## OTHER OBSERVATIONS
I used the lab 7 for the making of this project.
The inspiration for the trailing zeroes algorithm was taken from here: `stackoverflow.com/questions/57882748/remove-trailing-zero-in-c`

Unfortunately the checker fails several tests, among which is one of the earliest, data_subscribed.
Running the tests manually (subscribing to each topic one by one then running the udp client) seems to give the correct output,
leaving me and a few of my fellow peers who I asked to look over my code stumped.
I also asked one of the people responsible for the checker for help, but he did not want to look over my code (which is fair enough), and my laboratory teacher,
but given some unfortunate timing, he was away at the time and didn't have time to help.

I don't blame anyone for this, but I do wish the checker gave a more verbose output, at least for the people of next year:)
If the person checking this homework can figure out why these tests fail while running the checker, please leave the answer somewhere in the feedback
I tried plenty of things trying to get it to work and I am genuinely curious where the problem is!