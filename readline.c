#include <stdio.h>
#include <stdlib.h>

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