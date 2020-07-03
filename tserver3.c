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
#include <time.h>
#include "readline.h"

//nosso servidor atendera ate 100 clientes, se assim for.
#define MAX_CLIENTS 100
#define MAX_CHATROOM 100


/*
    Como funciona o servidor:
Basicamente, enquanto anteriormente tinhamos apenas um cliente, agora temos varios.
Esses clientes serao guardados em uma string da struct cliente. Quando um deles manda uma
mensagem, correremos a string mandando a tal mensagem para todos os outros que nao ele mesmo.

Para cuidar da criacao/recebimento/envio de mensagem, estaremos usando threads.
*/

//Reseta a cor escolhida
#define RESET "\x1B[0m"
#define ITALICO "\x1B[3m"

//Função que identifica uma cor aleatória para cada usuário
void cor_aleatoria(char *cor){
    srand(time(NULL));

    int valor = rand() %7;

    switch (valor){
        case 0: //VERMELHO
            strcpy(cor,"\x1B[31m\0");
        break;
        case 1: //VERDE
            strcpy(cor,"\x1B[32m\0");
        break;
        case 2: //AMARELO
            strcpy(cor,"\x1B[33m\0");
        break;
        case 3: //AZUL
            strcpy(cor,"\x1B[34m\0");
        break;
        case 4: //ROXO
            strcpy(cor,"\x1B[35m\0");
        break;
        case 5: //AZUL MARINHO
            strcpy(cor,"\x1B[36m\0");
        break;
        case 6: //BRANCO
            strcpy(cor,"\x1B[37m\0");
        break;
    }
}

//Struct para guardar informaçoes da sala de bate papo
typedef struct chatroom{
    char nome[200];
    int qtdpessoas; //contagem de pessoas na sala

}CHAT;

//Como estamos lidando com varios clientes, temos que criar varios sockaddr_in de clientes
//Temos quer guardar diferentes informacoes dele para poder funcionar corretamente. 
typedef struct client{
    //endereco do socket, isto era o cliente no ultimo trabalho. 
    struct sockaddr_in adress;

    //file descriptor do socket, quando fazemos o accept, ira para este int para este cliente. 
    int sockfd;

    //Id unico do usuario (um numero qualquer)
    int uid;    
    
    //Booleano para verificar se a possoa está mutada ou não
    int mutado;

    //Booleano para verificar se o usuario é adiministrador da sala
    int admin;

    //Booleano para verificar se o usuário foi kickado da sala ou não
    int kickado;

    //Nome que o usuario manda, a primeira coisa que ira com ele, guardamos aqui.  
    char name[50];

    //Numero do chat que o usuario esta conectado
    int chatid; //-1 quando nao estiver conectado
} CLI;


//inciamos o mutex;
//O que eh um mutex?
//Mutex eh o que nos usamos para garantir que coisas que sao usadas e modificadas por varios threads
//nao sejam usadas ao mesmo tempo, o que resulta em bagunca (e deadlocks), todas as funcoes que modificam a
//lista de clientes terao o dito mutex usado. 
pthread_mutex_t climutex = PTHREAD_MUTEX_INITIALIZER;

//ja inicializamos aqui tambem o vetor de clientes, ele guardara 100 enderecos para clientes;
CLI *clients[MAX_CLIENTS];

//Vetor de ChatRooms
CHAT *chatrooms[MAX_CHATROOM];
int qtd_chats = 0;

//Incializa o vetor de chatroom com valor NULL
void inicializa_chatroom(void){
    for(int i=0; i<MAX_CHATROOM; ++i){
        chatrooms[i] = NULL;

    }

}

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

void retirar_cli(int id, int error){

    //mesma ideia do anterior, tudo que modifica o vetor tem de haver um lock.
    printf("We are here 3.1\n");
    pthread_mutex_lock(&climutex);
    int chatid;
    int i = 0;
    printf("We are here 3.2\n");
    for(i; i < MAX_CLIENTS; i++)
    {
        printf("We are here 3.3\n");
        if(clients[i]){
            printf("We are here 3.4\n");
            if(clients[i]->uid == id){
                printf("We are here 3.5\n");
                chatid = clients[i]->chatid;
                clients[i] = NULL;
                break;
            }
        }
    }

    //agora retira o usuario da sala
    //se a sala ficar vazia, entao exclua
    if(error){
        printf("We are here 3.6\n");
        printf("chatroom 0 - qtd = %d\n Chatroom we want to delete: %d", chatrooms[0]->qtdpessoas,chatid);
        if((--(chatrooms[chatid]->qtdpessoas) ) == 0){
            printf("We are here 3.7\n");
            CHAT* dummy = chatrooms[chatid];
            printf("We are here 3.8\n");
            chatrooms[chatid] = NULL;
            printf("We are here 3.9\n");
            free(dummy);

            /*free(chatrooms[chatid]);
            printf("We are here 3.8\n");
            chatrooms[chatid] = NULL;
            printf("We are here 3.9\n");*/
        }
    }

    printf("We are here 3.10\n");
    pthread_mutex_unlock(&climutex);
}

//retirar e colocar sao funcoes bem simples, nao vamos complicar fazendo listas encadeadas ou metodos super
//eficientes de memoria. Apenas queremos colocar e retirar nomes da lista. 


 //Enviamos uma mensagem de forma simples, mandando escrever em todos os clientes menos a si proprio. 
 void send_message(char* message, int id , int chatid){
     
     //como nessa funcao tambem navegamos pelo vetor, temos que trancar qualquer mudanca a ele. 
     //Se um pedido de acionar/deletar usuario foi feito antes, obviamente, esta mensagem nao sera mandada a ele.
     pthread_mutex_lock(&climutex);

    for(int i = 0; i < c_count; i++){
        //Se o cliente esta na mesma sala que o remetente
        if(clients[i]->chatid == chatid && clients[i]->kickado == 0){
            //nao enviar mesmasem duplicada para o remetente
            if(clients[i]->uid != id){

                //write retorna -1 caso tenha algum problema, como seria o caso de um final repentino na parte do cliente.
                int errcounter = 0;
                while(write(clients[i]->sockfd, message,strlen(message)) < 0 && errcounter < 5)
                {

                    errcounter++;
                }
            }
        }
    }

     pthread_mutex_unlock(&climutex);
 }

//Funcao que escaneia todos as salas de chat procurando a sala com o mesmo nome,
 //se não existir, entao cria e retorna 0
 //Se existir, entao o usuario entrara nela e retornara o id da sala
 int verificar_chatroom(char nome_sala[200], CLI *cliente){
    pthread_mutex_lock(&climutex);

    int adicionado =0;
    int i;
    for(i = 0; i<MAX_CHATROOM; i++){
        //se existe a sala, entre nela
        if(chatrooms[i] != NULL){
            if(strcmp(chatrooms[i]->nome,nome_sala) == 0)
            {
                chatrooms[i]->qtdpessoas++;
                cliente->chatid = i;
                adicionado = 1;
            }
        }
        
        
        /*
        if(chatrooms[i] == NULL){
            continue;
        }
        else if(strcmp(chatrooms[i]->nome,nome_sala) == 0){
            ++(chatrooms[i]->qtdpessoas);
            cliente->chatid = i;
            adicionado = 1;            
        }*/
    }

    //se a sala não existe, então procure um espaço vago e crie-a.
    if(adicionado == 0){
        for(i=0; i<MAX_CHATROOM; ++i){
            //se existe a sala, entre nela
            if(chatrooms[i] == NULL){
                chatrooms[i] =(CHAT*) malloc(sizeof(CHAT));
                strcpy(chatrooms[i]->nome,nome_sala);
                chatrooms[i]->qtdpessoas = 1;
                cliente->admin = 1;                     //o usuario q criou a sala é o admin 
                cliente->chatid = i;
                printf("Chatid = %d\n", i);
                break;
             }
        }
    }

    pthread_mutex_unlock(&climutex);

    if(adicionado != 0){
        return i;
    }
    else{
            return -1;
        }


 }

 //Funcoes para lidar com comandos, Ha uma funcao para cada, seja kick, mute e etc.
//sem problemas
//problema ocorre fora da funcao
int kick(int chatid, char* name)
{
    printf("We are here 1.1\n");
    int didwedoit = 0;
    pthread_mutex_lock(&climutex);
    printf("We are here 1.2\n");

    for(int i = 0; i < MAX_CLIENTS; i++)
    {   
        printf("We are here 1.3\n");
        //checamos se ha um cliente para comecar
        if(clients[i] != NULL)
        {   
            
            //se for quem procuramos, kickamos ele
            if(strcmp(name, clients[i]->name) == 0 && clients[i]->chatid == chatid){
                
                clients[i]->kickado = 1;
                //mudamos a chatroom dele tambem para impedir que, por alguma razao, mensagens chegem a ele
                didwedoit = 1;
               
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&climutex);
    
    return didwedoit;
}
//sem problemas
int mute(int chatid, char* name)
{
   
    int didwedoit = 0;
    pthread_mutex_lock(&climutex);
    
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
          
        //checamos se ha um cliente para comecar
        if(clients[i] != NULL)
        {   
            
            //se for quem procuramos, mutamos ele
            if(strcmp(name, clients[i]->name) == 0 && clients[i]->chatid == chatid){
               
                clients[i]->mutado = 1;
               
                didwedoit = 1;
                break;
            }
        }
    }

    
    pthread_mutex_unlock(&climutex);
    
    return didwedoit;
}
//sem problemas
int unmute(int chatid, char* name)
{
    
    int didwedoit = 0;
    pthread_mutex_lock(&climutex);
    

    for(int i = 0; i < MAX_CLIENTS; i++)
    {   
        
        //checamos se ha um cliente para comecar
        if(clients[i] != NULL)
        {   
            
            //se for quem procuramos, desmutamos ele
            if(strcmp(name, clients[i]->name) == 0 && clients[i]->chatid == chatid){
               
                clients[i]->mutado = 0;
                didwedoit = 1;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&climutex);
    
    return didwedoit;
}

//Com problema, ja achei onde
int whois(int chatid, char* name, char* ipadress)
{
    printf("We are here 1.1\n");
    int didwedoit = 0;
    pthread_mutex_lock(&climutex);
    
    printf("We are here 1.2\n");
    for(int i = 0; i < MAX_CLIENTS; i++)
    {   
        printf("We are here 1.3\n");
        //checamos se ha um cliente para comecar
        if(clients[i] != NULL)
        {   
            printf("We are here 1.4\n");
            //se for quem procuramos, desmutamos ele
            if(strcmp(name, clients[i]->name) == 0 && clients[i]->chatid == chatid){
                printf("We are here 1.5\n");
                /*Problema se encontra aqui*/  
                strcpy(ipadress,inet_ntoa(clients[i]->adress.sin_addr));
                //ipadress = inet_ntoa(clients[i]->adress.sin_addr);
                printf("We are here 1.6 -> %s\n",ipadress);
                didwedoit = 1;
                break;
            }
        }
    }

    printf("We are here 1.7\n");
    pthread_mutex_unlock(&climutex);
    printf("We are here 1.8\n");
    return didwedoit;
}


//esta eh a funcao que vai quando ocorre o thread, ela e a porcao "servidor/cliente"
//eh resposabilidade desta funcao receber os inputs (tamanho 4096) e envia-lo aos outros.
//Aqui tambem cuidamos de "/ping" e "/quit", "/connect" lidamos no usuario. 
//O main contara, basicamente, com as configuracoes e inicio do servidor.
 void* comm_cli(void *arg){
    char cor[9];

     char input[4096];
     //recebemos o nome e colocaremos nesta string
     char nome[50];
     //flag que usaremos caso o usuario saia
     int sair = 1;
    
     //flag anti seg-faulter
     int earlyerror = 1;

     //Novo usuario! Adicionamos a conta.   
     c_count++;
    
     //arg eh o cliente vindo da criacao do thread na main. Temos que dar a ele o cast de CLI de novo pois
     //quando ele veio para nos ele veio com o cast de void*, requerimento do pthread_create()
     CLI* cliente = (CLI*) arg;
        
     //Primeira coisa que o usuario ira mandar eh o seu nome, recebemos ele agora.
     if(read(cliente->sockfd,nome,50) <=0 || strlen(nome) < 2|| strlen(nome)>=50){
         //acima checamos se recebemos um nome, ou se o nome eh muito pequeno (pois pode ser um simples " ", o que nao queremos)
         //ou tambem se ele ficou acima do valor desejado (queremos 50 caracteres, com 2 de folga)
         //Posivelmente tratamos isto no proprio cliente. 
         printf(ITALICO "Nome nao recebido/problema ao receber/nome invalido\n" RESET);
         sair = 0;
         earlyerror = 0;
        
     }

    //Se o usuario quiser sair durante a seleção do nome
     else if(strcmp(nome,"/quit") == 0){
            sair = 0;
        }

     else{
        //completamos a "ficha do cliente"
        //strcpy((*cliente).name, nome);
        for(int i = 0; i < strlen(nome) + 1; i++){
            (*cliente).name[i] = nome[i];
        }
        //O usuario recebe uma cor propria         
        cor_aleatoria(cor);
        
     }

     //se o usuario não quiser sair, então crie uma sala ou entre em uma
     if(sair != 0){
        char nome_sala[200];
        if(read(cliente->sockfd,nome_sala,200) <= 0 || strlen(nome_sala) < 2 || strlen(nome_sala) >=200){
            printf(ITALICO "Nome nao recebido/problema ao receber/nome invalido\n" RESET);
            sair = 0;
            earlyerror = 0;
            
        }
        //Insere o usuario na sala ou cria uma nova sala 
        else{
            verificar_chatroom(nome_sala, cliente);
        }

        
     }

    //sprintf permite que nos "printemos" em uma string, basicamente para nao ter que dar print no server e
    //nos usuarios com uma string diferente. 
    if(sair){
        sprintf(input, ITALICO "%s entrou no chat\n" RESET, cliente->name);
            
        //uid sera dado ao cliente na main.
        send_message(input, cliente->uid, cliente->chatid);
    }
    //limpamos o input. 
    bzero(input,4096);

    int errcounter = 0;
   
    //loop em que recebemos mensagens do user e mandamos para os outros. 
    while(sair){
       
        //verifica se o usuario foi kickado
        if(cliente->kickado){
            printf("We are here 2.1\n");
            char* kicked = "Voce foi kickado pelo admin.\n";
            //sprintf(kicked, ITALICO "Voce foi kickado pelo admin.\n" RESET);
            printf("We are here 2.2\n");
            write(cliente->sockfd,kicked,strlen(kicked));
            sair = 0;
            printf("We are here 2.3\n");
            break;
        }

        else
        {
            //caso um usuario nao-admin tentar usar comandos de admin, ele recebera essa mensagem
            char* not_admin = "Voce nao eh administrador do canal.\n";

            //funcao de ler, como visto antes, normal...
            int readv = read(cliente->sockfd,input,4096);
            printf("readv do user %s: %d\n errcounter: %d\n", cliente->name, readv,errcounter);

            //a funcao retorna um int dizendo quanto leu, ou se leu. 
            //Se for acima de 0, lemos algo e nao deu errado. 
            if(readv > 1){
                
                /*printf("Value of Admin is %d/n", cliente->admin);
                if(cliente->admin == 1)
                {
                    printf("This is the admin!/n");
                }*/
                char input_copy[4096];  //copia o input para usar no command_interpreter
                int comando;
                if(input != NULL){
                    strcpy(input_copy, input);
                    comando = command_interpreter(input_copy);
                    printf("Comando = %d\n",comando);
                }
                
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
                else if(cliente->kickado){
                    char* kicked = "Voce foi kickado pelo admin.\n";
                    //sprintf(kicked, ITALICO "Voce foi kickado pelo admin.\n" RESET);
                    write(cliente->sockfd,kicked,strlen(kicked));
                    sair = 0;
                }

                //comandos kick, mute, unmute e whois
                else if(comando == 6 || comando == 7 || comando == 8 || comando == 9){
                    if(cliente->admin == 0){
                        write(cliente->sockfd,not_admin,strlen(not_admin));
                    }
                    else{
                        int found_user = 0; //indica se o usuario-alvo foi encontrado no chatroom
                        char *target_name = strtok(NULL, " ");
                        //procurar nome do usuario na lista de usuarios e verificar se ele esta no chatroom
                        switch(comando){
                                
                            case 6: //kick
                                
                                found_user = kick(cliente->chatid, target_name);
                                break;
                                
                            case 7: //mute
                                
                                found_user = mute(cliente->chatid, target_name);
                                break;
                                
                            case 8: //unmute
                                
                                found_user = unmute(cliente->chatid, target_name);
                                break;

                            case 9: ;   //whois
                                printf("We are here1\n");
                                char* target_ip;
                                target_ip = (char*)malloc(40);
                                found_user = whois(cliente->chatid,target_name,target_ip);
                                printf("We are here 1.9\n");    
                                //parte ainda nao debugada
                                if(found_user == 1){
                                    printf("%s\n",target_ip);
                                    printf("We are here 1.10\n");
                                    write(cliente->sockfd,target_ip,strlen(target_ip));
                                    printf("We are here 1.11\n");
                                }
                                printf("We are here 2\n");
                                free(target_ip);
                                break;
                                
                            default:    //bom, isso aqui eh pra ser impossivel de acontecer
                                break;
                        }
                            
                        
                        //se essas condicoes nao forem atendidas, avisar o admin
                        if(found_user == 0){
                            char* no_user = "Nao existe um usuario com esse nome nesse canal.\n";
                            write(cliente->sockfd,no_user,strlen(no_user));
                        }
                    }
                }

                //eliminando os "vazios"
                else if(strcmp(input,"") == 0){

                }
                //se for uma mensagem comum e estiver mutado
                else if (cliente->mutado == 1){
                    char* muted = "VOCE ESTA MUTADO\n";
                    //sprintf(muted, ITALICO "VOCE ESTA MUTADO\n" RESET);
                    write(cliente->sockfd,muted,strlen(muted));
                    
                }
                else{
                    //modelo nick: msg
                    //Logo, precisamos de mais um pouco de espaco. 
                    //Aqui entao, criamos uma nova string, e usamos o sprintf
                    char bigtext[5100];
                    sprintf(bigtext,"%s%s: %s" RESET,cor, cliente->name,input); //CHECK
                    send_message(bigtext,cliente->uid, cliente->chatid);
                    //para melhor ver o server funcionando, postamos aqui tambem a mensagem. 
                    printf("%s%s\n",cor, bigtext);
                }



            }
            else{

                //se leu algo menor ou igual a 0, temos um problema
                //testamos 5 vezes, se der errado nas 5, removemos o usuario
                //readv = 1 eh um carinha interessante, que da segfault en nossos trabalhos
                if(readv < 1){
                    errcounter++;
                    if(errcounter >= 5){
                        sair = 0;
                    }
                }
            }
        }
    }

    
    //Se voce esta aqui, quer dizer que voce decidiu sair ou foi removido por problemas de conexao.
    //Se voce nao saiu antes mesmo de entrar.
    if(earlyerror){
        printf("We are here 2.4\n");
        //sprintf permite que nos "printemos" em uma string, basicamente para nao ter que dar print no server e
        //nos usuarios com uma string diferente. 
        sprintf(input, ITALICO "%s saiu do chat" RESET, cliente->name);
            
        //uid sera dado ao cliente na main.
        //Como ele nao tem chatid ainda, da segfault
        send_message(input, cliente->uid, cliente->chatid);
        
        printf("User %s left the chat\n", cliente->name);
    }

       
        //mandamos a "mensagem da morte" para o usuario
        //usamos esta mensagem no ultimo cliente e, no intuito de mudar o minimo possivel, continuamos usando.   
        printf("We are here 2.5\n");
        char* ender = "Conversa Terminada X.X\n";
        write(cliente->sockfd,ender,strlen(ender));

       
        printf("We are here 2.6\n");
        retirar_cli(cliente->uid,earlyerror);

        printf("We are here 2.7\n");   
        //fechamos o file descriptor
        
        close(cliente->sockfd);
        //libramos a memoria
        printf("We are here 2.8\n");
        free(cliente);
    
    //um cliente a menos
    c_count--;

    printf("We are here 2.9\n");
    //nos livramos deste thread, agora nao mais necessario;
    pthread_detach(pthread_self());
    
    //funcao do tipo void* precisa de retorno, eu acho
    //Engracado, nao?
    return NULL;

 }

 //Estamos agora na famosa main
 //Aqui cuidamos de iniciar o servidor e de esperar/adicionar novas conexoes. 
 //Como anteriormente, para comecar o servidor usamos ./tserver portnumber
 int main(int argc, char**argv){
     
     if(argc < 2){
         printf(ITALICO "Erro, numero de Port nao informado" RESET);
         return 1;
     }

     //inicializa o vetor de chat rooms
     inicializa_chatroom();

     //novamente criamos um sockadrr_in para o server e para o cliente. 
     //   
     struct sockaddr_in serv_addr, cli_addr;
     pthread_t tid;
     int portno, sockfd,newsockfd;
    

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
     signal(SIGPIPE,SIG_IGN);

     //Fazemos o bind 
     if(bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
     {
         printf(ITALICO "Erro no binding\n" RESET);
         exit(1);
     }

     //O servidor passa a estar "Online"
     //Vamos agora ouvir.

     if(listen(sockfd,10) < 0){
         printf(ITALICO "Erro no listen" RESET);
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
            printf(ITALICO "Maximo de clientes atingido" RESET);
            //aqui usamos continue.
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
        cli->mutado = 0;
        cli->kickado = 0;
        cli->admin = 0;
        cli->chatid = -1;


        //adicionamos o cliente ao vetor e criamos um thread
        adicionar_cli(cli);
        pthread_create(&tid,NULL,&comm_cli, (void*)cli);

        //para reduzir o uso do CPU
        sleep(1);

     }

     return EXIT_SUCCESS;
 }