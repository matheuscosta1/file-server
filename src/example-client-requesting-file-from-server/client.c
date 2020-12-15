#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#define MAX_MSG 1024

/*
 Cliente envia mensagemEnviaNomeArquivoRequeridoParaServidor ao servidor e imprime resposta
 recebida do Servidor
 */



int main(int argc, char *argv[]) {

    struct dirent *myfile;
    struct stat mystat;

    if (argc != 6) {
        fprintf(stderr, "use:./cliente [IP] [Porta] [arquivo] [Diretório para buscar Arquivo] [Tipo de Operação]\n");
        return -1;
    } else if (!isdigit(*argv[2])) {
        fprintf(stderr, "Argumento invalido '%s'\n", argv[2]);
        fprintf(stderr, "use:./cliente [IP] [Porta] [arquivo] [Diretório para buscar Arquivo] [Tipo de Operação]\n");
        return -1;
    }else if (!isdigit(*argv[5])) {
        fprintf(stderr, "Argumento invalido '%s'\n", argv[5]);
        fprintf(stderr, "use:./cliente [IP] [Porta] [arquivo] [Diretório para buscar Arquivo] [Tipo de Operação]\n");
        return -1;
    }

    // variaveis
    int socket_desc, conexao, nova_conex;
    DIR *diretorioParaBuscarArquivo;
    struct sockaddr_in servidor;

    diretorioParaBuscarArquivo = opendir(argv[4]);

    if(diretorioParaBuscarArquivo == NULL ){fprintf(stderr, "Argumento invalido '%s'\n", argv[4]);return -1;}

    char* tipoTransacaoAuxiliar = argv[5];
    int tipoTransacao = atoi(tipoTransacaoAuxiliar);
    printf("%s", tipoTransacaoAuxiliar);

    /*****************************************/
    /* Criando um socket */
    // AF_INET = ARPA INTERNET PROTOCOLS
    // SOCK_STREAM = orientado a conexao
    // 0 = protocolo padrao para o tipo escolhido -- TCP
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_desc == -1) {
        printf("Nao foi possivel criar socket\n");
        return -1;
    }

    /* Informacoes para conectar no servidor */
    // IP do servidor
    // familia ARPANET
    // Porta - hton = host to network short (2bytes)

    char* aux1 = argv[2];
    int portaServidor = atoi(aux1);

    servidor.sin_addr.s_addr = inet_addr(argv[1]);
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(portaServidor);

    //Conectando no servidor remoto
    if (connect(socket_desc, (struct sockaddr *) &servidor, sizeof (servidor)) < 0) {
        printf("Nao foi possivel conectar\n");
        return -1;
    }
    printf("Conectado no servidor\n");
    /*****************************************/
    /*******COMUNICAO - TROCA DE MENSAGENS **************/
    
    char *mensagemEnviaNomeArquivoRequeridoParaServidor;
    char resposta_servidor[MAX_MSG];
    int tamanho;

    void *connection_handler(void *socket_desc) {
        char *mensagem;
        char resposta[MAX_MSG];
        int tamanho;
        mensagemEnviaNomeArquivoRequeridoParaServidor = argv[3];
        /*********************************************************/

        /*********comunicao entre cliente/servidor****************/

        // lendo dados enviados pelo cliente
        //mensagem 1 recebido nome do arquivo   
        if ((tamanho = strlen(mensagemEnviaNomeArquivoRequeridoParaServidor)) < 0) {
            perror("Erro ao receber dados do cliente: ");
            return NULL;
        }
        
        printf("tamanhoo: %d", tamanho);
        for(int i=0; i < tamanho; i++){
            resposta[i] = mensagemEnviaNomeArquivoRequeridoParaServidor[i];
        }
        resposta[tamanho] = '\0';

        for(int i=0; i < tamanho; i++){
            printf("indice: %d, resposta %c", i, resposta[i]);
        }

        if (send(socket_desc, resposta, strlen(resposta), 0) < 0) {
            printf("Erro ao enviar nome do arquivo\n");
            return -1;
        }

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

                printf("Arquivo lido: %s, Arquivo procurado: %s, Comparação:%d\n", myfile->d_name, resposta, strcmp(myfile->d_name, resposta));
                if (strcmp(myfile->d_name, resposta) == 0) {//arquivo existe
                   printf("Arquivo existe lado do cliente");
                   closedir(diretorioParaBuscarArquivo);
                    //Reiniciando variÃ¡veis da pesquisa do diretorio para a proxima thread
                    myfile = NULL;
                    diretorioParaBuscarArquivo = NULL;
                    diretorioParaBuscarArquivo = opendir(argv[3]);

                    //**************************************//
                    //      INICIO DO PROTOCOLO            //
                    //*************************************//


                    mensagem = "200";
                    //mensagem 2 - enviando confirmaÃ§Ã£o q arquivo existe
                    write(socket_desc, mensagem, strlen(mensagem));

                    //mensagem 3 - recebendo que arquivo OK do cliente
                    //read(socket_desc, resposta, MAX_MSG);


                    //**************************************//
                    //      FIM DO PROTOCOLO               //
                    //*************************************//

                    //abrindo o arquivo e retirando o tamanho//
                    //fazendo cÃ³pia do nome do arquivo para variÃ¡vel auxiliar. tal variavel Ã© utilzada para localizar
                    // o arquivo no diretorio.


                    char localArquivo[1024]; 
                    strncpy(localArquivo, argv[4], 1024);
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
                    send(socket_desc, mensagem, strlen(mensagem), 0);

                    int fd = open(localArquivo, O_RDONLY);
                    off_t offset = 0;
                    int sent_bytes = 0;
                    //localArquivo = NULL;
                    if (fd == -1) {
                        fprintf(stderr, "Error opening file --> %s", strerror(errno));

                        exit(EXIT_FAILURE);
                    }

                    while (((sent_bytes = sendfile(socket_desc, fd, &offset, BUFSIZ)) > 0) && (len > 0)) {

                        fprintf(stdout, "1. Cliente enviou %d bytes do arquivo, offset Ã© agora : %d e os dados restantes = %d\n", sent_bytes, (int)offset, len);
                        len -= sent_bytes;
                        fprintf(stdout, "2. Cliente enviou %d bytes do arquivo, offset Ã© agora : %d e os dados restantes = %d\n", sent_bytes, (int)offset, len);
                        if (len <= 0) {
                            printf("Cliente enviou todos os dados do arquivo com sucesso");
                            break;
                        }
                    }
                    //closedir(diretorioParaBuscarArquivo);

                }
            }if(myfile==NULL) {
                    //enviando mensagem para o cliente de arquivo nao encontrado.
                    mensagem = "404";//file not found
                    printf("\n//*********************************//\n");
                    printf("Arquivo \"%s\" não existe no diretório: \"%s\"\n",aux_nomeArquivo, argv[3]);
                    //mensagem 2 - enviando confirmaÃ§Ã£o q arquivo existe
                    write(socket_desc, mensagem, strlen(mensagem));
                    //sempre que termina de pesquisar o diretorio de arquivos a variavel myfile vai para null
                    // entao eh necessario preencher diretorioParaBuscarArquivo novamente com o argv[2] com o diretorio de pesquisa. 
                    //caso contrario novas thread nao acessaram o diretorio passado em argv[2]]
                    diretorioParaBuscarArquivo = opendir(argv[3]);
                    //
                    while (1) {
                    }
                    close(socket_desc);
                    //closedir(diretorioParaBuscarArquivo);

            }
            if (diretorioParaBuscarArquivo != NULL) {
                closedir(diretorioParaBuscarArquivo);
                diretorioParaBuscarArquivo = NULL;
            }
        }

        if (strcmp(resposta, "bye\n") == 0) {
            close(socket_desc);
            printf("Servidor finalizado...\n");
            return NULL;
        }
    }


    pthread_t sniffer_thread;

    switch (tipoTransacao){
        case 1:
            mensagemEnviaNomeArquivoRequeridoParaServidor = argv[3];
            FILE *received_file;
            received_file = fopen(argv[3], "w");
            ssize_t len;
            char buffer[BUFSIZ];
            int aba;

            //Enviando uma mensagemEnviaNomeArquivoRequeridoParaServidor
            //mensagemEnviaNomeArquivoRequeridoParaServidor 1 enviando nome do arquivo.  
            if (send(socket_desc, mensagemEnviaNomeArquivoRequeridoParaServidor, strlen(mensagemEnviaNomeArquivoRequeridoParaServidor), 0) < 0) {
                printf("Erro ao enviar mensagemEnviaNomeArquivoRequeridoParaServidor\n");
                return -1;
            }
            printf("Dados enviados %s\n", mensagemEnviaNomeArquivoRequeridoParaServidor);

            memset(mensagemEnviaNomeArquivoRequeridoParaServidor, 0, sizeof mensagemEnviaNomeArquivoRequeridoParaServidor);
            memset(resposta_servidor, 0, sizeof resposta_servidor);

            //Recebendo resposta do servidor
            //mensagemEnviaNomeArquivoRequeridoParaServidor 2 recebendo que arquivo existe   
            if((tamanho = read(socket_desc, resposta_servidor, MAX_MSG)) < 0) {
                printf("Falha ao receber resposta\n");
                return -1;
            }

            printf("Resposta recebida: %s\n", resposta_servidor);
            if (strcmp(resposta_servidor, "200") == 0) {

                mensagemEnviaNomeArquivoRequeridoParaServidor = "OK";
                //mensagemEnviaNomeArquivoRequeridoParaServidor 3 enviado ok
                write(socket_desc, mensagemEnviaNomeArquivoRequeridoParaServidor, strlen(mensagemEnviaNomeArquivoRequeridoParaServidor));
                //mensagemEnviaNomeArquivoRequeridoParaServidor 4 recebendo o tamanho do arquivo;
                memset(resposta_servidor, 0, sizeof resposta_servidor);
                read(socket_desc, resposta_servidor, 1024);

                int tamanhoDoArquivo = atoi(resposta_servidor);
                printf("\nTamanho do arquivo a ser copiado: %s \n", resposta_servidor);
                aba = tamanhoDoArquivo;

            }else{
                fprintf(stderr, "Arquivo nao encontrado no servirdor'%s'\n", argv[3]);
                close(socket_desc);
                printf("Cliente finalizado com sucesso!\n");
                return 0;
            }


            while (((len = recv(socket_desc, buffer, BUFSIZ, 0)) > 0)&& (aba > 0)) {
                fwrite(buffer, sizeof (char), len, received_file);
                aba -= len;
                fprintf(stdout, "Recebidos %d bytes e aguardamos :- %d bytes\n", len, aba);
                if (aba <= 0) {
                    break;
                }
            }
            fclose(received_file);
            close(socket_desc);
            printf("Cliente finalizado com sucesso!\n");
            exit(0);
        case 2:
            printf("Opção 2");
            connection_handler(socket_desc);
            exit(0);
        default:
            printf("Waiting..");
    }

    return 0;
}