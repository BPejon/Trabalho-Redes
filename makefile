all:
	gcc readline.c tserver.c -o server -Wall
	gcc readline.c tclient.c -o client -Wall
