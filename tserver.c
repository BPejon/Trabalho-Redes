/*
	Esse arquivo servira como "servidor" 
	Na primeira fase do trabalho, nao precisamos tratar servidor e cliente como coisas diferentes, trataremos disso depois, porem
*/

/*
	Esse arquivo contem explicacoes sobre cada linha em forma de comentario
*/

	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <sys/types.h> //contem varios tipos de data usados nos outros includes 
	#include <sys/socket.h> //definicao das structs para criar os sockets
	#include <netinet/in.h> //contem constantes e structs para enderecos de internet
	#include <sys/wait.h>

	//funcao para notificar erros 
	void error(const char *msg)
	{
    	perror(msg);
    	exit(1);
	}

	//estaremos usando entradas de argv
	int main(int argc, char *argv[])
	{
		//sockfd e newsockfd sao descritores de arquivo, guardam os valores da system call socket e accept 
		//respectivamente. portno guarna o numero do port que aceitara as conexoes. N guarda os valores de retorno de read e write
		int sockfd, newsockfd, portno, n; 
		
		//Os dados que vem do socket sao passados para um buffer, no nosso caso de 4096 chars (talvez eu tenha que alocar isso)
		char buffer[4096];

		//struckt sockaddr_in contem um endereco de internet, por enquanto temos apenas dois, um para o servidor e um para o cliente.
		struct sockaddr_in serv_addr, cli_addr;

     	socklen_t clilen;

     	//como usamos argc, precisamos receber dois inputs.
     	if(argc < 2)
     	{
     		printf("Erro, nenhum port selecionado \n");
     		exit(1);
     	}

     	//Criamos o socket:
     	//Para criar o socket vao 3 variaveis:
     	// AF_INET nos fala que usaremos o dominio de enderecos da internet, se estivesemos usando 
     	// para processos que compartilham o mesmo sistema de arquivo usariamos AF_UNIX
     	// SOCK_STREAM nos diz que tipo de socket queremos, o sistema TCP e uma stream constante de informacoes
     	// guardadas em um buffer, se fosse UDP (para datagram) usariamos SOCK_DGRAM
     	// o 0 indica que tipo de protocolo estamos usando, por ser 0 ele vai, entao, escolher o mais apropriado
     	// para SOCK_STREAM, seria o TCP, protocolo que vamos usar. 
     	sockfd = socket(AF_INET, SOCK_STREAM, 0);

     	//se sockfd < 0 (ie -1) quer dizer que o socket nao foi criado 
     	if(sockfd < 0)
     	{
     		printf("Erro ao abrir socket");
     		exit(1);
     	}

     	//bzero zera o buffer, precisa do endereco do buffer e o seu tamanho; 
     	bzero((char*) &serv_addr, sizeof(serv_addr));

     	//numero do port e passado por uma string em argv[1]
     	portno = atoi(argv[1]); 

     	//definimos qual e a familia de sinais que estamos tratando. Pesquisar sobre a struct para mais informacoes
     	serv_addr.sin_family = AF_INET;

     	//O valor que estamos mandando esta em portno sobre qual port usamso
     	//porem o valor tem que estar em network byte order, entao usamos htons, que faz tal coisa.
     	serv_addr.sin_port = htons(portno);

     	//Setamos o endereco do servidor, no caso o endereco da maquina que o hosteia.
     	//INADDR_ANY e o valor que pega esse endereco para nos;
     	serv_addr.sin_addr.s_addr = INADDR_ANY;


     	//Fazemos a ligacao entre o socket com o endereco do host e o numero do port;
     	//leva o discritor do socket, o endereco para unir e o tamanho do mesmo; 
     	if(bind(sockfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
     	{
     		printf("Erro no binding");
     		exit(1);
     	}

     	//A partir de agora o servidor esta "Online"

     	//Escutamos por conexoes ao socket; 5 indica quantas conexoes podemos deixar esperando enquanto conectamos a primeira
     	//Se 6 clientes tentassem conctar ao mesmo tempo um daria erro. Como lidamos so com um por enquanto, estamos bem. 
     	//Se formos tratar de varios clientes ao mesmo tempo, e aqui que provavelmente teriamso de criar novos processos
     	listen(sockfd, 5);

     	clilen = sizeof(cli_addr);

     	//Accept conecta o servidos com o cliente, esperando ate dar certo. 
     	//Quando da, tal conexao e marcada em newsockfd, toda a comunicacao com esse cliente sera feita por newsockfd
     	//multiplos clientes = multiplos newsockfd
     	newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
     	if(newsockfd < 0)
     	{
     		printf("Erro na conexao");
     		exit(1);
     	}
		printf("conexao estabelecida\n");
     	//para mandar coisas para o cliente, usamos write
     	//colocamos o descritor de arquivo do socket, o buffer (nesse caso string) de conteudo e o tamanho.
     	n = write(newsockfd,"conexao estabelecida\n",21);
     	
     	//n guarda quantos caracteres foram mandados, se nao mandou nada = erro
     	if(n < 0)
     	{
     		printf("Erro ao mandar texto");
     		exit(1);
     	}

     	//Para fazer com que a conversa de um lado para o outro seja mais genuina, precisamos, entao, fazer um fork.

		pid_t pid = fork();

		if(pid < 0 ){
			perror("Fork Falhou");
		}

		//nesse caso, vamos colocar o escrever mensagens no child, nao acho que muda muito	
		char* sair = "/sair\n";
		
        //Processo filho
		if(pid == 0)
		{
			while(1){
				bzero(buffer,sizeof(buffer));

				fgets(buffer,4096,stdin);
                char* empty = "";
                if(strcmp(buffer,empty) == 0)
                {

                }
            
				

				if(strcmp(sair,buffer) == 0) //se acabou a conversa
				{
					n = write(newsockfd,"Conversa Terminada X.X\n",23);
					exit(0);
				}


				n = write(newsockfd,buffer,sizeof(buffer));
				if(n < 0)
				{
					printf("Erro ao escrever mensagem, terminando chatroom");
					exit(1);
				}
			}


		}
		//recebendo mensagens e colocando na tela do terminal
        //processo pai
		else
		{
			char buffer2[4096];
			while (1)
			{
				bzero(buffer2,sizeof(buffer));
				n = read(newsockfd,buffer2,sizeof(buffer2)); //lemos do buffer
				if(n < 0)
				{
					printf("Erro em receber mensagem, ou acabou a transmissao. Saindo...\n");
                    kill(pid, SIGTERM); //mata processo filho
					exit(1);
				}
				char* empty = "";
                if(strcmp(buffer2,empty) == 0)
                {

                }
				else if(strcmp(buffer2,"Conversa Terminada X.X\n") == 0)
                {
                    printf("O outro usuario terminou a conversa\n Saindo...\n");
                    kill(pid, SIGTERM);//mata processo filho
                    exit(0);
                }
                else
				printf(">%s",buffer2);
			}
			
			
		}

		wait(NULL);

		return 0;


	}


