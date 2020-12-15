#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <ctype.h>
#include <pthread.h>

void *connection_handler(void *);

#define MAX_MSG 1024

/*
 Servidor aguarda por mensagem do cliente, imprime na tela
 e depois envia resposta e finaliza processo
 */

int main(int argc, char* argv[]) {

    //Variaveis auxiliares para encontrar o arquivo a ser transferido.
    DIR *diretorioParaBuscarArquivo, *diretorioParaGravarArquivo;
    struct dirent *myfile;
    struct stat mystat;
    //verificando se foi executando o comando corretamente
    if (argc != 5) {
        fprintf(stderr, "use:./server [Porta] [Diretório para buscar arquivo] [Diretório para gravar arquivo] [Tipo de Operação]\n");
        return -1;
    } else if (!isdigit(*argv[1])) {
        fprintf(stderr, "Argumento invalido '%s'\n", argv[1]);
        fprintf(stderr, "use:./server [Porta] [Diretório para buscar arquivo] [Diretório para gravar arquivo] [Tipo de Operação]\n");
        return -1;
    } else if (!isdigit(*argv[4])) {
        fprintf(stderr, "Argumento invalido '%s'\n", argv[4]);
        fprintf(stderr, "use:./server [Porta] [Diretório para buscar arquivo] [Diretório para gravar arquivo] [Tipo de Operação]\n");
        return -1;
    } 

    diretorioParaBuscarArquivo = opendir(argv[2]);
    diretorioParaGravarArquivo = opendir(argv[3]);

    //verificando se o diretorio existe
    if(diretorioParaBuscarArquivo == NULL ){fprintf(stderr, "Argumento invalido '%s'\n", argv[2]);return -1;}
    if(diretorioParaGravarArquivo == NULL ){fprintf(stderr, "Argumento invalido '%s'\n", argv[3]);return -1;}

    char* aux1 = argv[1];
        int portaServidor = atoi(aux1);

    //verificando se o diretorio existe

   //variaveis
    int socket_desc, conexao, c, nova_conex;
    struct sockaddr_in servidor, cliente;
    
    
    char* tipoTransacaoAuxiliar = argv[4];
    int tipoTransacao = atoi(tipoTransacaoAuxiliar);
    printf("%s", tipoTransacaoAuxiliar);

    // para pegar o IP e porta do cliente  
    char *cliente_ip;
    int cliente_port;

    //*********************************************************************//
    //      INICIO DO TRATAMENTO DA THREAD, localizaÃ§Ã£o e transferencia    // 
    //      do arquivo.                                                    // 
    //*********************************************************************//
    void *connection_handler(void *socket_desc) {
        char *mensagem;
        char resposta[MAX_MSG];
        int tamanho;

        /*********************************************************/

        /*********comunicao entre cliente/servidor****************/

        // pegando IP e porta do cliente
        cliente_ip = inet_ntoa(cliente.sin_addr);
        cliente_port = ntohs(cliente.sin_port);
        printf("cliente conectou: %s : [ %d ]\n", cliente_ip, cliente_port);

        // lendo dados enviados pelo cliente
        //mensagem 1 recebido nome do arquivo   
        if ((tamanho = read(conexao, resposta, MAX_MSG)) < 0) {
            perror("Erro ao receber dados do cliente: ");
            return NULL;
        }
        printf("tamanho: %d", tamanho);
        resposta[tamanho] = '\0';
        printf("O cliente falou: %s\n", resposta);

        char aux_nomeArquivo[MAX_MSG];
        //fazendo cÃ³pia do nome do arquivo para variÃ¡vel auxiliar. tal variavel Ã© utilzada para localizar
        // o arquivo no diretorio.
        strncpy(aux_nomeArquivo, resposta, MAX_MSG);
        //printf("ax_nomeArquivo: %s\n", aux_nomeArquivo);



        /*********************************************************/
        if (diretorioParaBuscarArquivo != NULL) {

            //funÃ§Ã£o busca todo o diretÃ³rio buscando o arquivo na variavel aux_nomeArquivo
            //struct stat s;
            while ((myfile = readdir(diretorioParaBuscarArquivo)) != NULL) {

                stat(myfile->d_name, &mystat);

                printf("Arquivo lido: %s, Arquivo procurado: %s\n", myfile->d_name, resposta);
                if (strcmp(myfile->d_name, resposta) == 0) {//arquivo existe
                   closedir(diretorioParaBuscarArquivo);
                    //Reiniciando variÃ¡veis da pesquisa do diretorio para a proxima thread
                    myfile = NULL;
                    diretorioParaBuscarArquivo = NULL;
                    diretorioParaBuscarArquivo = opendir(argv[2]);

                    //**************************************//
                    //      INICIO DO PROTOCOLO            //
                    //*************************************//


                    mensagem = "200";
                    //mensagem 2 - enviando confirmaÃ§Ã£o q arquivo existe
                    write(conexao, mensagem, strlen(mensagem));

                    //mensagem 3 - recebendo que arquivo OK do cliente
                    read(conexao, resposta, MAX_MSG);


                    //**************************************//
                    //      FIM DO PROTOCOLO               //
                    //*************************************//

                    //abrindo o arquivo e retirando o tamanho//
                    //fazendo cÃ³pia do nome do arquivo para variÃ¡vel auxiliar. tal variavel Ã© utilzada para localizar
                    // o arquivo no diretorio.


                    char localArquivo[1024]; 
                    strncpy(localArquivo, argv[2], 1024);
                    strcat(localArquivo,aux_nomeArquivo);

                    FILE * f = fopen(localArquivo, "rb");
                    if((fseek(f, 0, SEEK_END))<0){printf("ERRO DURANTE fseek");}
                    int len = (int) ftell(f);                   
                    mensagem = (char*) len;
                    printf("Tamanho do arquivo: %d\n", len);
                    //convertendo o valor do tamanho do arquivo (int) para ser enviado em uma mensagem no scoket(char)
                    char *p, text[32];
                    int a = len;
                    sprintf(text, "%d", len);
                    mensagem = text;

                    //mensagem 4 - enviando o tamanho do arquivo
                    send(conexao, mensagem, strlen(mensagem), 0);

                    int fd = open(localArquivo, O_RDONLY);
                    off_t offset = 0;
                    int sent_bytes = 0;
                    //localArquivo = NULL;
                    if (fd == -1) {
                        fprintf(stderr, "Error opening file --> %s", strerror(errno));

                        exit(EXIT_FAILURE);
                    }

                    while (((sent_bytes = sendfile(conexao, fd, &offset, BUFSIZ)) > 0) && (len > 0)) {

                        fprintf(stdout, "1. Servidor enviou %d bytes do arquivo, offset Ã© agora : %d e os dados restantes = %d\n", sent_bytes, (int)offset, len);
                        len -= sent_bytes;
                        fprintf(stdout, "2.Servidor enviou %d bytes do arquivo, offset Ã© agora : %d e os dados restantes = %d\n", sent_bytes, (int)offset, len);
                        if (len <= 0) {
                            break;
                        }
                    }
                    //closedir(diretorioParaBuscarArquivo);
                    while (1) {
                    }

                }
            }if(myfile==NULL) {
                    //enviando mensagem para o cliente de arquivo nao encontrado.
                    mensagem = "404";//file not found
                    printf("\n//*********************************//\n");
                    printf("Arquivo \"%s\" não existe no diretório: \"%s\"\n",aux_nomeArquivo, argv[2]);
                    //mensagem 2 - enviando confirmaÃ§Ã£o q arquivo existe
                    write(conexao, mensagem, strlen(mensagem));
                    //sempre que termina de pesquisar o diretorio de arquivos a variavel myfile vai para null
                    // entao eh necessario preencher diretorioParaBuscarArquivo novamente com o argv[2] com o diretorio de pesquisa. 
                    //caso contrario novas thread nao acessaram o diretorio passado em argv[2]]
                    diretorioParaBuscarArquivo = opendir(argv[2]);
                    //
                    while (1) {
                    }
                    close(conexao);
                    //closedir(diretorioParaBuscarArquivo);

            }
            if (diretorioParaBuscarArquivo != NULL) {
                closedir(diretorioParaBuscarArquivo);
                diretorioParaBuscarArquivo = NULL;
            }
        }

        if (strcmp(resposta, "bye\n") == 0) {
            close(conexao);
            printf("Servidor finalizado...\n");
            return NULL;
        }
    }

    //*********************************************************************//
    //      FIM DO TRATAMENTO DA THREAD, localizaÃ§Ã£o e transferencia    // 
    //      do arquivo.                                                    // 
    //*********************************************************************//



    //************************************************************
    /*********************************************************/
    //Criando um socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Nao foi possivel criar o socket\n");
        return -1;
    }

    //Preparando a struct do socket
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY; // Obtem IP do S.O.
    servidor.sin_port = htons(portaServidor);

    //Associando o socket a porta e endereco
    if (bind(socket_desc, (struct sockaddr *) &servidor, sizeof (servidor)) < 0) {
        puts("Erro ao fazer bind Tente outra porta\n");
        return -1;
    }
    puts("Bind efetuado com sucesso\n");

    // Ouvindo por conexoes
    listen(socket_desc, 3);
    /*********************************************************/

    //Aceitando e tratando conexoes

    puts("Aguardando por conexoes...");
    c = sizeof (struct sockaddr_in);
    char resposta_servidor[MAX_MSG];
    int tamanho;
    char *mensagemEnviaNomeArquivoRequeridoParaServidor;
    
    switch (tipoTransacao){
        case 1:
            while ((conexao = accept(socket_desc, (struct sockaddr *) &cliente, (socklen_t*) & c))) {
                if (conexao < 0) {
                    perror("Erro ao receber conexao\n");
                    return -1;
                }

                pthread_t sniffer_thread;
                nova_conex = (int) malloc(1);
                nova_conex = conexao;

                if (pthread_create(&sniffer_thread, NULL, connection_handler, (void*) nova_conex) < 0) {
                    perror("could not create thread");
                    return 1;
                }
                puts("Handler assigned");
            }
            if (nova_conex < 0) {
                perror("accept failed");
                return 1;
            }
        case 2:
            printf("Operação 2.");

            while ((conexao = accept(socket_desc, (struct sockaddr *) &cliente, (socklen_t*) & c))) {
                if (conexao < 0) {
                    perror("Erro ao receber conexao\n");
                    return -1;
                }

                if ((tamanho = read(conexao, resposta_servidor, MAX_MSG)) < 0) {
                    perror("Erro ao receber dados do cliente: ");
                    return NULL;
                }
                printf("%s", resposta_servidor);
               
                FILE *received_file;
                received_file = fopen(resposta_servidor, "w");
                ssize_t len;
                char buffer[BUFSIZ];
                int aba;
                int tamanho;

                memset(resposta_servidor, 0, sizeof resposta_servidor);

                //Recebendo resposta do servidor
                //mensagemEnviaNomeArquivoRequeridoParaServidor 2 recebendo que arquivo existe   
                if((tamanho = read(conexao, resposta_servidor, MAX_MSG)) < 0) {
                    printf("Falha ao receber resposta\n");
                    return -1;
                }

                printf("Resposta recebida: %s\n", resposta_servidor);
                if (strcmp(resposta_servidor, "200") == 0) {

                    mensagemEnviaNomeArquivoRequeridoParaServidor = "OK";
                    //mensagemEnviaNomeArquivoRequeridoParaServidor 3 enviado ok
                    write(conexao, mensagemEnviaNomeArquivoRequeridoParaServidor, strlen(mensagemEnviaNomeArquivoRequeridoParaServidor));
                    //mensagemEnviaNomeArquivoRequeridoParaServidor 4 recebendo o tamanho do arquivo;
                    memset(resposta_servidor, 0, sizeof resposta_servidor);
                    read(conexao, resposta_servidor, 1024);

                    int tamanhoDoArquivo = atoi(resposta_servidor);
                    printf("\nTamanho do arquivo a ser copiado: %s \n", resposta_servidor);
                    aba = tamanhoDoArquivo;

                }else{
                    fprintf(stderr, "Arquivo nao encontrado no servirdor'%s'\n", argv[3]);
                    close(conexao);
                    printf("Cliente finalizado com sucesso!\n");
                    return 0;
                }


                while (((len = recv(conexao, buffer, BUFSIZ, 0)) > 0)&& (aba > 0)) {
                    fwrite(buffer, sizeof (char), len, received_file);
                    aba -= len;
                    fprintf(stdout, "Recebidos %d bytes e aguardamos :- %d bytes\n", len, aba);
                    if (aba <= 0) {
                        break;
                    }
                }
                fclose(received_file);
                close(conexao);
                printf("Servidor recebeu arquivo %s com sucesso!\n", received_file);
            }
            if (nova_conex < 0) {
                perror("accept failed");
                return 1;
            }
        default:
            printf("Waiting...");
    }

    
}