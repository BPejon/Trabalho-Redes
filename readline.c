#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_line(){
	char *str;
	int i = 0, flag = 0;
	
	str = (char *)malloc(sizeof(char));
	
	while(flag == 0){
		if(i == 0){
			scanf("%c", &str[i]);
			i++;
			str = (char *)realloc(str, sizeof(char) * i+1);
		}
		else if(i > 0){
			scanf("%c", &str[i]);
			i++;
			str = (char *)realloc(str, sizeof(char) * i+1);
		}
		if(str[i-1] == '\n'){
			flag = 1;
			str[i-1] = '\0';
		}
	}
	
	return str;
}

//ATENCAO: COPIAR INPUT ORIGINAL ANTES DE USAR ESSE COMANDO, PQ O STRTOK COLOCA UM \0 QUANDO ACHA O TOKEN
int command_interpreter(char *str){
	char *comm;
	comm = strtok (str, " ");

	if(!strcmp(comm, "/connect"))
		return 1;
	else if(!strcmp(comm, "/quit"))
		return 2;
	else if(!strcmp(comm, "/ping"))
		return 3;
	else if(!strcmp(comm, "/join"))
		return 4;
	else if(!strcmp(comm, "/nickname"))
		return 5;
	else if(!strcmp(comm, "/kick"))
		return 6;
	else if(!strcmp(comm, "/mute"))
		return 7;
	else if(!strcmp(comm, "/unmute"))
		return 8;
	else if(!strcmp(comm, "/whois"))
		return 9;
	
	return 0;	//mensagem normal
}