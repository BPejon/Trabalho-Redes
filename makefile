all:
	gcc tserver.c -o server -Wall
	gcc tclient.c -o client	-Wall