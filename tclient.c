
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <sys/types.h> //contem varios tipos de data usados nos outros includes 
	#include <sys/socket.h> //definicao das structs para criar os sockets
	#include <netinet/in.h> //contem constantes e structs para enderecos de internet
	#include <sys/wait.h>
    #include <netdb.h>
    #include <errno.h>

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

       char buffer[4096];
       
       //tambem estamos usando args aqui, essa vez 2 deles
       if(argc < 3)
       {
           printf("Argumentos insuficientes\n");
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
            perror("Fork falhou\n");
        }

        char* sair = "/sair\n";

        //programa filho
        //responsável por enviar as mensagens
        if(pid == 0){
            

			while(1){
				bzero(buffer,sizeof(buffer));
				
                //criamos input
                fgets(buffer,4096,stdin);
                char* empty = "";
                //se nao foi nada, nada faremos
                if(strcmp(buffer,empty) == 0)
                {
                
                }

            
                //se acabou a conversa
				if(strcmp(sair,buffer) == 0) 
				{
					n = write(sockfd,"Conversa Terminada X.X\n",23);
                    //Envia para o processo pai sua morte



					exit(0);
				}

				n = write(sockfd,buffer,sizeof(buffer));
				if(n < 0)
				{
					printf("Erro ao escrever mensagem, terminando chatroom\n");
					exit(1);
				}

                //PPID armazena id do
			}


		}
        
        //buffer de receber mensagens
        //programa pai
        else{

            char buffer2[4096];
			while (1)
			{
				bzero(buffer2,sizeof(buffer));
				n = read(sockfd,buffer2,sizeof(buffer)); //lemos do buffer,
                int pidfilho, status;
                    pidfilho = waitpid(pid, &status, WNOHANG); //verifica se o processo filho mandou algo
                    if(pidfilho < 0){
                        perror("waitpid");
                    }
                
                       printf("%d\n", pidfilho);
                      printf(" status %d\n", status);
                    if(pidfilho > 0){
                      exit(0);
                    }


				if(n < 0)
				{
					printf("Erro em receber mensagem, ou acabou a transmissao.\n Saindo...\n");
                    kill(pid, SIGTERM);//mata processo filho
					exit(1); 
				}

                char* empty = "";
                if(strcmp(buffer2,empty) == 0)
                {   



                    int pidfilho, status;
                    pidfilho = waitpid(pid, &status, WNOHANG); //verifica se o processo filho mandou algo
                    if(pidfilho < 0){
                        perror("waitpid");
                    }
                
                       printf("%d\n", pidfilho);
                      printf(" status %d\n", status);
                    if(pidfilho > 0){
                      exit(0);
                    }

                }

                //se o outro lado pediu para terminar a conversa, para ter certeza, terminamos aqui.
                else if(strcmp(buffer2,"Conversa Terminada X.X\n") == 0){
                    printf("O outro usuario terminou a conversa.\n Saindo...\n");
                    kill(pid, SIGTERM); //mata processo filho
                    exit(0);
                }

               
                else
                {
                    printf(">%s",buffer2);

                    int pidfilho, status;
                    pidfilho = waitpid(pid, &status, WNOHANG); //verifica se o processo filho mandou algo
                    if(pidfilho < 0){
                        perror("waitpid");
                    }
                
                       printf("%d\n", pidfilho);
                      printf(" status %d\n", status);
                    if(pidfilho > 0){
                      exit(0);
                    }

                //caso o processo filho terminar/
                /*else if(1){
                    int pidfilho, status;
                    pidfilho = waitpid(pid, &status, WNOHANG); //verifica se o processo filho mandou algo
                    if(pidfilho < 0){
                        perror("waitpid");
                    }
                    else if(pidfilho == 0){
                        printf("%d", pidfilho);
                        break;
                    }
                }*/
                }          

			}
			
			
		}


		wait(NULL);

		return 0;

    }