CC=g++
CFLAGS=-Wall -Werror -Wno-error=unused-variable -g

build: server subscriber

server: server.cpp common.cpp helpers.cpp
	$(CC) -o $@ $^ $(CFLAGS)

subscriber: subscriber.cpp common.cpp helpers.cpp
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf server subscriber

zip:
	zip 323CC_Iacob_Ioana-Delia_Tema2.zip *.cpp *.h Makefile readme.txt