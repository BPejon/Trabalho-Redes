Alunos:
    Lucas Xavier Ebling Pereira - 10692183
    Breno Pejon Rodrigues- 10801152
    Julia Carolina Frare Peixoto - 10734727

 Sistemas Operacionais Utilizados:
    Linux Mint 18.3
    Linux Ubuntu 18.04.4

 Compilador Utilizado: 
    GCC

Instrucoes Extras:
        O Makefile apenas compila os dois programas presentes, tclient e tserver.
    Para funcionar é preciso primeiro rodar o server, é preciso fornecer, ao rodar
    o programa um numero de port valido e disponivel  (ex: ./server 51717), depois,
    ao rodar o programa cliente é preciso fornecer o nome da máquina e o port para
    conectar (ex: ./client computador-joao 51717). Nossos programas funcionam apenas
    quando o terminal "cliente" e "servidor" estão na mesma màquina. 

        Enquanto ambos estao em operação, será possível a troca de mensagens entre
    um terminal e outro. Mensagens vindas do outro terminal recebem um ">" a frente
    para facilitar a visualização. Mensagens tem de ser mandadas por meio do input do
    terminal, utilização de um arquivo para os inputs, infelizmente, não funciona. As 
    mensagens maiores que 4096 são tratadas apropriadamente, sendo mandadas como duas
    mensagens. 

        Para sair de um dos programas, seja ele servidor ou cliente, basta escrever
    "/sair" (sem aspas) e enviar como uma mensagem normal (a mensagem deve conter
    apenas "/sair"). Após a mensagem ser enviada ambos os terminais irão finalizar,
    retornando 0. 
