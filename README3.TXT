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
    O Makefile (use make all) apenas compila os dois programas presentes, tclient e tserver.
    Para funcionar é preciso primeiro rodar o server, é preciso fornecer, ao rodar
    o programa um numero de port valido e disponivel  (ex: ./server 51717), depois,
    ao rodar o programa cliente é preciso fornecer o nome da máquina e o port para
    conectar (ex: ./client computador-joao 51717). Nossos programas funcionam apenas
    quando o terminal "cliente" e "servidor" estão na mesma màquina. Nao foi possivel
    testar o funcionamento em redes locais pois nenhum aluno se encontra na mesma
    cidade. 

    --> O servidor:
    O servidor deve ser iniciado primeiro, como indicado antes, e, salvo erros, 
    resultada em nenhuma mensagem no terminal. Quando usuarios comecarem a se
    conectar no servidor, suas mensagems serao mostradas nao so no para os outros
    usuarios como tambem para o terminal do servidor. As mensagens de salas de chat
    diferentes serao todas mandadas para o servidor, porem sem indicacao de que chat
    se tratam. 
    
    O servidor atende todas as questoes pedidas, tendo, inclusive, a contencao caso
    nao seja possivel se conectar com o usuario depois de 5 tentativas. Existem salas
    de chat, moderadores com seus comandos.   

    Para terminar o servidor e preciso usar CTRL + C.

    --> O usuario:
    Quando conectarmos ao usuario, havera mensagens para "instruir" o usuario,
    o usuario podera, entao, usar todos os comandos inciais (connect, ping e quit). 
    Como o usuario nao esta conectado, as respostas sao feitas por meio de ifs. 

    Ao selecionar "/connect" o programa pede para o usuario inserir o seu nome por
    meio do comando "/nickname nomeexemplo". Quando o servidor trata se o nome e permitido
    ou nao ou mesmo repetido. Caso ele seja um nome nao permitido ou repetido, o programa
    pede para um novo nome ser inserido.

    Depois o programa pedira para inserir o nome do servior, da mesma forma, o servidor
    devera conformar as regras de nomenclatura, sendo pedido para inserir novamente
    caso nao esteja de acordo. O primeiro usuario da dita sala se torna o administrador.
    Durante todo esse tempo o usuario pode decidir interromper o processo em seu lado usando
    "/quit".

    Quando conectado, o usuario podera, a partir de entao, mandar mensagens na sala e 
    tambem recebe-las de outros usuarios conectados. Os outros clientes serao marcados
    por seus nomes ao lado de suas mensagens e de uma cor diferenciada, enquanto as do 
    cliente deste terminal sao da cor padrao e nao possuem nome. Para sair, basta usar
    /quit, /ping funciona perfeitamente e CTRL + C, como pedido, sera ignorado. 

    Se o usuario for um admin ele tera acesso a todos os comandos pedidos. Em nosso
    programa nao foi implementado a criacao de novos administradores para a sala entao,
    se o administrador original sair da sala, ela estara no modo "Velho Oeste" sem nenhuma
    moderacao. 