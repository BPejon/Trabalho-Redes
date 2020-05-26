#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

//nosso servidor atendera ate 100 clientes, se assim for.
#define MAX_CLIENTS 100


/*
    Como funciona o servidor:
Basicamente, enquanto anteriormente tinhamos apenas um cliente, agora temos varios.
Esses clientes serao guardados em uma string da struct cliente. Quando um deles manda uma
mensagem, correremos a string mandando a tal mensagem para todos os outros que nao ele mesmo.

Para cuidar da criacao/recebimento/envio de mensagem, estaremos usando threads. 
Rezo por deus que a questao do deadlock se resolva. Digo o mesmo para a dos ips.
*/

//Como estamos lidando com varios clientes, temos que criar varios sockaddr_in de clientes
//Temos quer guardar diferentes informacoes dele para poder funcionar corretamente. 
typedef struct client{
    //endereco do socket, isto era o cliente no ultimo trabalho. 
    struct sockaddr_in adress;

    //file descriptor do socket, quando fazemos o accept, ira para este int para este cliente. 
    int sockfd;

    //Id unico do usuario (um numero qualquer)
    int uid;

    //Nome que o usuario manda, a primeira coisa que ira com ele, guardamos aqui.  
    char name[32];
} CLI;


//inciamos o mutex;
//O que eh um mutex?
//Mutex eh o que nos usamos para garantir que coisas que sao usadas e modificadas por varios threads
//nao sejam usadas ao mesmo tempo, o que resulta em bagunca (e deadlocks), todas as funcoes que modificam a
//lista de clientes terao o dito mutex usado. 
pthread_mutex_t climutex = PTHREAD_MUTEX_INITIALIZER;

//ja inicializamos aqui tambem o vetor de clientes, ele guardara 100 enderecos para clientes;
CLI *clients[MAX_CLIENTS];

static _Atomic unsigned int c_count = 0;

//o que eh _Atomic?
//_Atomic da ao int uma "ordem de modificacao", isso significa que, quando temos varios treads, isso significa que o
//valor de c_count sera o mesmo para todos os threads, de modo que sua manipulacao fique constante e nao zoe completamente. 

//criamos um id estatico para usar em todas as funcoes. 
//Porque nao _Atomic?
//Nao eh necessario, pois ele e modificado e usado apenas na main, onde nao ha threads.
static int id = 1;

//Como temos uma "lista"  de clientes, temos entao que fazer um vetor guardando todos, uma fila basicamente.

void adicionar_cli(CLI *cliente){

    //trancamos climutex. 
    //Se este for o primeiro a utilizar essa tranca, ele continua normalmente 
    //se nao, esperaremos ate ser destrancado para usar.
    //desse modo, dois threads nao usam essa funcao ao mesmo tempo.
    pthread_mutex_lock(&climutex);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        //se for um espaco vazio
        if(!clients[i]){
            clients[i] = cliente;
            break;
        }

    }

    //desbloqueamos para um proximo poder usar.
    pthread_mutex_unlock(&climutex);
}

void retirar_cli(int id){

    //mesma ideia do anterior, tudo que modifica o vetor tem de haver um lock.
    pthread_mutex_lock(&climutex);
    
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i]){
            if(clients[i]->uid = id)
            {
                clients[i] = NULL;
                break;
            }
        }
    }

    pthread_mutex_unlock(&climutex);
}

//retirar e colocar sao funcoes bem simples, nao vamos complicar fazendo listas encadeadas ou metodos super
//eficientes de memoria. Apenas queremos colocar e retirar nomes da lista. 


 //Enviamos uma mensagem de forma simples, mandando escrever em todos os clientes menos a si proprio. 
 void send_message(char* message, int id){
     
     //como nessa funcao tambem navegamos pelo vetor, temos que trancar qualquer mudanca a ele. 
     //Se um pedido de acionar/deletar usuario foi feito antes, obviamente, esta mensagem nao sera mandada a ele.
     pthread_mutex_lock(&climutex);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i]){
            if(clients[i]->uid != id){

                //write retorna -1 caso tenha algum problema, como seria o caso de um final repentino na parte do cliente.
                //tentamos 5 vezes, se nao der certo, fechamos o usuario. 
                int errcounter = 0;
                while(write(clients[i]->sockfd, message,strlen(message))< 0 && errcounter < 5)
                {
                    errcounter++;
                }

                //se ja falhamos multiplas vezes, deletamos o usuario em questao.
                //Parte experimental do codigo, nao sei como seria o comportamento dos threads aqui. 
                //perguntarei para a professora.  
                //Lendo melhor o codigo, ainda preciso pensar em uma boa solucao.
                //Pensando melhor, teriamos de usar a parte de "deletar" presente no final de comm_cli aqui.
                /*
                    if(errcounter = 5){
                        pthread_mutex_unlock(&climutex);
                        retirar_cli(clients[i]->uid);
                    }
                */
            }
        }
    }

     pthread_mutex_unlock(&climutex);
 }


//esta eh a funcao que vai quando ocorre o thread, ela e a porcao "servidor/cliente"
//eh resposabilidade desta funcao receber os inputs (tamanho 4096) e envialo aos outros.
//Aqui tambem cuidamos de "/ping" e "/quit", "/connect" lidamos no usuario. 
//O main contara, basicamente, com as configuracoes e inicio do servidor.
 void* comm_cli(void *arg){
    
     char input[4096];
     //recebemos o nome e colocaremos nesta string
     char nome[32];
     //flag que usaremos caso o usuario saia
     int sair = 1;
    
     //Novo usuario! Adicionamos a conta.   
     c_count++;
    
     //arg eh o cliente vindo da criacao do thread na main. Temos que dar a ele o cast de CLI de novo pois
     //quando ele veio para nos ele veio com o cast de void*, requerimento do pthread_create()
     CLI* cliente = (CLI*) arg;

     //Primeira coisa que o usuario ira mandar eh o seu nome, recebemos ele agora.
     if(read(cliente->sockfd,nome,32) <=0 || strlen(nome) < 2|| strlen(nome)>=31){
         //acima checamos se recebemos um nome, ou se o nome eh muito pequeno (pois pode ser um simples " ", o que nao queremos)
         //ou tambem se ele ficou acima do valor desejado (queremos 30 caracteres, com 2 de folga)
         //Posivelmente tratamos isto no proprio cliente. 
         printf("Nome nao recebido/problema ao receber/nome invalido");
     }
     else{
         //completamos a "ficha do cliente"
         strcpy(cliente->name,nome);

        //sprintf permite que nos "printemos" em uma string, basicamente para nao ter que dar print no server e
        //nos usuarios com uma string diferente. 
        sprintf(input, "%s entrou no chat", cliente->name);
        
        //uid sera dado ao cliente na main.
        send_message(input, cliente->uid);
     }

    //limpamos o input. 
    bzero(input,4096);

    //loop em que recebemos mensagens do user e mandamos para os outros. 
    while(sair){

        //funcao de ler, como visto antes, normal...
        int readv = read(cliente->sockfd,input,4096);

        //a funcao retorna um int dizendo quanto leu, ou se leu. 
        //Se for acima de 0, lemos algo e nao deu errado. 
        if(readv > 0){
            
            //se recebemos ping, mandamos pong de volta para o usuario. 
            if(strcmp(input,"/ping")== 0){
                char* pong = "pong\n";
                int failscount = 0;
                //temos que checar se estamos escrevendo sem problemas, se tiver problemas, temos, entao, que tirar. 
               while( write(cliente->sockfd,pong,strlen(pong)) < 0 && failscount < 5)
                {
                    failscount++;    
                }
                if(failscount != 0){
                    sair = 0;
                }
            }

            //se o usuario quiser sair, terminamos o loop
            else if(strcmp(input,"/quit") == 0){
                sair = 0;
            }

            //eliminando os "vazios"
            else if(strcmp(input,"") == 0){

            }
            //se for uma mensagem comum.
            else{
                //modelo nick: msg
                //Logo, precisamos de mais um pouco de espaco. 
                //Aqui entao, criamos uma nova string, e usamos o sprintf
                char bigtext[5100];
                sprintf(bigtext,"%s: %s",cliente->name,input);
                send_message(bigtext,cliente->uid);
                //para melhor ver o server funcionando, postamos aqui tambem a mensagem. 
                printf("%s\n",bigtext);
            }



        }

    }

    //Se voce esta aqui, quer dizer que voce decidiu sair. 

    //mandamos a "mensagem da morte" para o usuario
    //usamos esta mensagem no ultimo cliente e, no intuito de mudar o minimo possivel, continuamos usando.   
    char* ender = "Conversa Terminada X.X\n";
    write(cliente->sockfd,ender,strlen(ender));

    //fechamos o file descriptor
    close(cliente->sockfd);
    //libramos a memoria
    free(cliente);

    //um cliente a menos
    c_count--;

    //nos livramos deste thread, agora nao mais necessario;
    pthread_detach(pthread_self());

    //funcao do tipo void* precisa de retorno, eu acho
    //Engracado, nao?
    return NULL;

 }

 //Estamos agora na famosa main
 //Aqui cuidamos de iniciar o servidor e de esperar/adicionar novas conexoes. 
 //Tinha pensado em usar forks tambem para possiveis "comandos moderadores", mas por enquanto prefiro nao arriscar.
 //Como anteriormente, para comecar o servidor usamos ./tserver portnumber
 int main(int argc, char**argv){
     
     if(argc < 2){
         printf("Erro, numero de Port nao informado");
         return 1;
     }

     //novamente criamos um sockadrr_in para o server e para o cliente. 
     //   
     struct sockaddr_in serv_addr, cli_addr;
     pthread_t tid;
     int portno, sockfd,newsockfd, n;
    

     //Criamos o socket file descriptor, como no anterior
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if(sockfd < 0){
         printf("Erro ao abrir socket\n");
         exit(1);
     }

     //zeramos o endereco do servidor 
     bzero((char*) &serv_addr,sizeof(serv_addr));
     
     //numero do port retirado do argv
     //colocamos aqui as informacoes do server
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_port = htons(portno);
     //pegamos o ip
     serv_addr.sin_addr.s_addr = INADDR_ANY;

     //ignorar sinais de pipe
     //no exemplo que eu vi, este era usado, ainda preciso entender
     signal(SIGPIPE,SIG_IGN);

     //Fazemos o bind 
     if(bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
     {
         printf("Erro no binding\n");
         exit(1);
     }

     //O servidor passa a estar "Online"
     //Vamos agora ouvir.

     if(listen(sockfd,10) < 0){
         printf("Erro no listen");
         exit(1);
     } 

     //Agora, estamos no servidor. 
     //Aqui, temos um while aonde iremos settar clientes
     //Adiciona-los ao vetor e abrir um thread.  

     //para fechar esse servidor, teremos de dar um Ctrl + C
     while(1){

        socklen_t clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd,(struct sockaddr*)&cli_addr,&clilen);
        //Primeiro vemos se temos o numero maximo de clientes
        if((c_count+1)==MAX_CLIENTS){
            printf("Maximo de clientes atingido");
            //aqui usamo continue.
            //O que continue faz?
            //Continue, diferente de break, manda "rodar no inicio"
            //o while novamente, basicamente, pula o resto do while.
            //fazemos isso porque usuarios podem sair ou entrar a qualquer momento
            //uma tentativa de entrar de novo pode ocorrer. 
            continue;
        }

        //Aceitar e adicionar um novo cliente;
        CLI* cli = (CLI*)malloc(sizeof(CLI));
        cli->adress = cli_addr;
        cli->sockfd = newsockfd;
        cli->uid = id++;

        //adicionamos o cliente ao vetor e criamos um thread
        adicionar_cli(cli);
        pthread_create(&tid,NULL,&comm_cli, (void*)cli);

        //para reduzir o uso do CPU
        sleep(1);

     }

     return EXIT_SUCCESS;
 }