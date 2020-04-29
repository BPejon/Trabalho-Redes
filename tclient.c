
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <sys/types.h> //contem varios tipos de data usados nos outros includes 
	#include <sys/socket.h> //definicao das structs para criar os sockets
	#include <netinet/in.h> //contem constantes e structs para enderecos de internet
	#include <sys/wait.h>
    #include <netdb.h>
    #include "readline.h"

    void error(char* msg)
    {
        perror(msg);
        exit(1);
    }

    int main(int argc, char* argv[])
    {
        int sockfd, portno,n; //similares a servidor;
        struct sockaddr_in serv_addr;
        
        
        /*  hostent e uma stuct nao usada em server
            ela guarda: 
            char* h_name, nome do host
            char** h_aliases, apelidos do host
            int h_addrtype, tipo de endereco do host
            int h_lenght, comprimento do endereco
            char** h_addr_list, lista de enderecos do server desse nome
            #define h_addr h_addr_list[0], basicamente o endereco "oficial" e o primeiro
        */
       struct hostent *server;

       char *input, buffer[4097];
       
       //tambem estamos usando args aqui, essa vez 2 deles
       if(argc < 3)
       {
           printf("Argumentos insuficientes");
           exit(1);
       }
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0)
        {
            printf("Problemas com Socket\n");
            exit(1);
        }
       //Agora pegamos o servidor usando o seu nome
       //nota: char* h_addr contem o ip, pode ser util depois
       server = gethostbyname(argv[1]);
       if(server == NULL)
       {
           printf("Erro, host nao encontrado\n");
           exit(1);
       }

        //limpar serv_addr, zerando-o
        bzero((char*) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        //copiando informacoes de server para serv_addr;
        bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
        portno = atoi(argv[2]);
        serv_addr.sin_port = htons(portno);

        //Agora conectamos:
        //A funcao connect leva 3 variaveis: Descritor do socket, endereco do host e o tamanho do endereco
        if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
        {
            printf("Erro de conexao\n");
            exit(1);
        }
       


        //Note que no cliente nao temos newsockfd, como temos no server. 
        //assim como fizemos no "server" fazemos no cliente para mandar e receber msgs
        pid_t pid = fork();

        if(pid < 0){
            perror("Fork falhou");
        }

        char* sair = "/sair";

        //child cuida de mandar msgs novamente;

        //No momento, nao fechamos automaticamente quando pedimos a saida/recebemos a saida
        //Para resolver isso, usaremos pipes.
        //O pipe ira mandar, na eventualidade de saida, um aviso ao processo pai que podemos fechar tudo!
        //Ao mesmo tempo, se o pipe pai receber tal comando de saida, ele tambem avisara o filho!
        //Desse jeito, se um termina, ambos terminam!  
        /*
        int pipefd1[2]; //comunicacao sentido child --> parent
        int pipefd2[2]; //comunicacao sentido parent --> child 

        pipe(pipefd1);
        pipe(pipefd2);
        if(pipe(pipefd1) == -1 || pipe(pipefd2) == -1)
        {
            printf("Erro ao criar pipes\n");
            return 1;
        }
        */
        if(pid == 0)
		{
            //fechamos a parte de ler do 1
            //close(pipefd1[0]);
            //fehcamos a parte de escrever do 2
            //close(pipefd2[1]);

			while(1){
				
                //criamos input
                input = read_line();

                char* empty = "";
                //se nao foi nada, nada faremos
                if(strcmp(input,empty) == 0)
                {
                
                }

                //se queremos acabar a conversa, mandamos um sinal para la.
                //isso quer dizer que temos que mandar, para o pipe, um aviso de que acabamos.
				if(strcmp(sair,input) == 0) //se acabou a conversa
				{
					n = write(sockfd,"Conversa Terminada X.X\n",23);
                    //write(pipefd1[1],"over",strlen("over"));
					exit(0);
				}

                for(int i = 0; i <= (strlen(input)/4096); i++)
                {
                    bzero(buffer,sizeof(buffer));
                    strcpy(buffer, input + (i * 4097));
                    n = write(sockfd,buffer,sizeof(buffer));
                }

				if(n < 0)
				{
					printf("Erro ao escrever mensagem, terminando chatroom");
					exit(1);
				}

                //nessa parte checamos se o parent recebeu alguma mensagem de que acabou (no caso do servidor)
                /*char* checkover;
                read(pipefd2[0],checkover,10);
                if(strcmp(checkover,"over") == 0)
                {
                    exit(0);
                }*/

			}


		}
        
        //buffer de receber mensagens
        else
		{
            /* //fechamos a parte de escrever do 1
            close(pipefd1[1]);
            //fehcamos a parte de ler do 2
            close(pipefd2[0]);
            */

            char buffer2[4097];
			//char *buffer2;
			while (1)
			{
				bzero(buffer2,sizeof(buffer2));
				n = read(sockfd,buffer2,sizeof(buffer2) - 1); //lemos do buffer

				if(n < 0)
				{
					printf("Erro em receber mensagem, ou acabou a transmissao, por favor escreva /sair para terminar o chat");
					break; //nao fazemos exit imediatamente para nao criar processos orfaos
				}
                char* empty = "";
                if(strcmp(buffer2,empty) == 0)
                {

                }
                //se o outro lado pediu para terminar a conversa, para ter certeza, terminamos aqui.
                else if(strcmp(buffer2,"Conversa Terminada X.X\n") == 0)
                {
                    printf("O outro usuario terminou a conversa, escrevra /sair\n");
                    //se terminada a conversa, temos que avisar o child!
                    //write(pipefd2[1],"over",strlen("over"));
                    break;
                }
                else
                {
                    printf(">%s\n",buffer2);
                }

                //depois, checar se o child mandou uma mensagem de que temos que terminar
               /* char* checkover;
                read(pipefd1[0],checkover,10);
                if(strcmp(checkover,"over") == 0)
                {
                    printf("Conversa Terminada");
                    exit(0);
                }*/           

			}
			
			
		}

		wait(NULL);

		return 0;

    }