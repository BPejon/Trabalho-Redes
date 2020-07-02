
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
    #include "readline.h"


    #define RESET "\x1B[0m"
    #define ITALICO "\x1B[3m"

    /*
        Versao 2.0, para o trablaho 2:
    
    Com o intuito de reutilizar o maximo possivel o codigo anterior, o cliente sofrera poucas modificacoes
    Seu objetivo original de escrever mensagens e envia-las ao servidor, e de tambem mostrar mensagens que o servidor
    traz se manteram, bastou apenas introduzir um pouco de codigo para acomodar os novos comandos.  
    */


    void error(char* msg)
    {
        perror(msg);
        exit(1);
    }


    //Usado para eliminar o CTRL + C, acho
    void siginthandler(int sig_num){
        signal(SIGINT, siginthandler);
        fflush(stdout);

    }

    int main(int argc, char* argv[])
    {
        signal(SIGINT, siginthandler);
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
           printf(ITALICO "Argumentos insuficientes\n" RESET);
           exit(1);
       }
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0)
        {
            printf(ITALICO "Problemas com Socket\n" RESET);
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

        printf("Bem vindo ao servidor!\nComandos:\n/connect: conectar ao servidor\n/quit: sair do servidor\n/ping: verificar conexao\n");

        //Antes de conectar, como pede o sistema, o usuario precisa escrever "/connect"
        //para isto, entao, vamos criar os comandos em um while para o usuario brincar. 

        while(1){

            input = read_line();

            if(strcmp(input,"") == 0){
                
            }
            //se o usuario usar quit, entao acaba tudo
            else if(command_interpreter(input) == 2){   //quit
                return 0;
            }
            else if(command_interpreter(input) == 3){   //ping
                printf(ITALICO "pong\n" RESET);
            }
            else if(command_interpreter(input) == 1){   //connect
                break;
            }
            else{
                printf(ITALICO);
                printf("Voce ainda nao esta conectado!\n");
                printf(RESET);
            }
    
        }


        //Agora conectamos:
        
        

        //A funcao connect leva 3 variaveis: Descritor do socket, endereco do host e o tamanho do endereco
        if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
        {
            printf(ITALICO "Erro de conexao\n" RESET);
            exit(1);
        }
       
        //precisamos mandar um nome
        //o usuario soh sai daqui se ele usar o comando /nickname seguido de um nome valido, ou /quit
        char valid_name = 0;
        while(valid_name == 0){
            printf("Por favor, escolha um nome de usuario com o comando '/nickname apelidoDesejado'\n");
            input = read_line();
            //printf("%s\n", input);
            char input_copy[sizeof(input)];
            strcpy(input_copy, input);
            int comando = command_interpreter(input_copy);
            //printf("%s\n", input_copy);
            //write(sockfd,input,sizeof(input));
            if(comando == 5){
                valid_name = 1;
                char *token = strtok(NULL, " ");
                char nome[50];  //a ideia eh copiar o token pra esse aqui
                strcpy(nome, token);
                printf("%s %ld\n", nome, strlen(nome));
                write(sockfd,nome,strlen(nome) + 1);
            }
            else if(comando == 2){
                write(sockfd,input,sizeof(input));
                return 0;
            }
        }

        //precisamos tambem escolher um chatroom, ou criar um novo
        char valid_chat = 0;
        while(valid_chat == 0){
            printf("Use o comando '/join nomeCanal' para entrar em algum canal ou criar um canal novo.\n");
            input = read_line();
            char input_copy[sizeof(input)];
            strcpy(input_copy, input);
            int comando = command_interpreter(input_copy);
            //write(sockfd,input,sizeof(input));
            if(comando == 4){
                valid_chat = 1;
                char *token = strtok(NULL, " ");
                char nome_sala[200];
                strcpy(nome_sala, token);
                printf("%s\n", nome_sala);
                write(sockfd,nome_sala,strlen(nome_sala) + 1);
            }
            else if(comando == 2){
                write(sockfd,input,sizeof(input));
                return 0;
            }
        }
        

        //Note que no cliente nao temos newsockfd, como temos no server. 
        //assim como fizemos no "server" fazemos no cliente para mandar e receber msgs
        pid_t pid = fork();

        if(pid < 0){
            perror(ITALICO "Fork falhou\n" RESET);
        }

        //precisei mudar a string para quit, conformando ao trabalho.
        char* sair = "/quit";

        //programa filho
        //responsável por enviar as mensagens
        if(pid == 0)
		{

			while(1){
				
                //criamos input
                input = read_line();

                char* empty = "";
                //se nao foi nada, nada faremos
                if(strcmp(input,empty) == 0)
                {
                
                }

                //se acabou a conversa
				if(strcmp(sair,input) == 0)
				{
					n = write(sockfd,sair,sizeof(sair));
                    //Envia para o processo pai sua morte
					exit(2);
				}

                //caso a mensagem ultrapasse 4096 bytes
                for(int i = 0; i <= (strlen(input)/4096); i++)
                {
                    bzero(buffer,sizeof(buffer));
                    strcpy(buffer, input + (i * 4097));
                    n = write(sockfd,buffer,sizeof(buffer));
                }

				if(n < 0)
				{
					printf("Erro ao escrever mensagem, terminando chatroom\n");
					exit(1);
				}

			}


		}
        
        //buffer de receber mensagens
        //programa pai
        else
		{


            char buffer2[4097];
			//char *buffer2;
			while (1)
			{
				bzero(buffer2,sizeof(buffer2));
				n = read(sockfd,buffer2,sizeof(buffer2) - 1); //lemos do buffer


                //Caso o cliente quiser sair da conversa
                int pidfilho, status;
                pidfilho = waitpid(pid, &status, WNOHANG); //verifica se o processo filho mandou algo
                if(pidfilho < 0){
                    perror("waitpid\n");
                }
                if(pidfilho > 0){
                    exit(0);
                }

				if(n < 0)
				{
					printf("Erro ao receber mensagem, ou acabou a transmissao.\n Saindo...\n");
					kill(pid, SIGTERM);//mata processo filho
                    exit(1);
				}
                char* empty = "";
                if(strcmp(buffer2,empty) == 0)
                {

                }
                //se o outro lado pediu para terminar a conversa, para ter certeza, terminamos aqui.
                else if(strcmp(buffer2,"Conversa Terminada X.X\n") == 0)
                {
                    printf("Voce saiu da conversa.\n");
                    kill(pid, SIGTERM);
                    exit(0);
                }
                else
                {
                    printf(">%s\n",buffer2);
                }        

			}
			
			
		}

		wait(NULL);

		return 0;

    }