all:
	gcc readline.c tserver2.c -pthread -o server -Wall
	gcc readline.c tclient2.c -o client -Wall
