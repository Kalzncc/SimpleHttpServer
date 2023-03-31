all: http-server

http-server: src/*.c
	gcc -Wall -g src/*.c src/utils/*c -o bin/http-server -lssl -lcrypto -lpthread -D__DEBUG

clean:
	@rm http-server
