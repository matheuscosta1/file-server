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
#include <assert.h>

void *connection_handler(void *);

#define MAX_MSG 1024
#define BADKEY -1
#define EXIT 0
#define GET 1
#define POST 2

typedef struct { char *key; int val; } key_value_struct;

static key_value_struct helptable[] = {
    { "EXIT", EXIT }, { "GET", GET }, { "POST", POST }
};

#define NUMBER_KEYS (sizeof(helptable)/sizeof(key_value_struct))

int keyfromstring(char *key)
{
    int i;
    for (i=0; i < NUMBER_KEYS; i++) {
        key_value_struct *key_value  = malloc(sizeof(key_value_struct));
        *key_value = helptable[i];
        if (strcmp(key_value->key, key) == 0)
            return key_value->val;
    }
    return BADKEY;
}

int main(int argc, char* argv[]) {

    DIR *diretorioParaBuscarArquivo;
    struct dirent *arquivoProcurado;
    struct stat mystat;
    
    char mensagem_post_ou_get[MAX_MSG];
    char respostaNomeArquivo[MAX_MSG];
    char respostaNomeDiretorio[MAX_MSG];

    if (argc != 2) {
        fprintf(stderr, "Utilize a seguinte especificação:\n./server [Porta]\n");
        return -1;
    } else if (!isdigit(*argv[1])) {
        fprintf(stderr, "Argumento inválido: '%s'\n", argv[1]);
        fprintf(stderr, "Utilize a seguinte especificação:\n./server [Porta]\n");
        return -1;
    }

    char* portaServidorAuxiliar = argv[1];
    int portaServidor = atoi(portaServidorAuxiliar);

    int _socket, conexao, c, novaConexao;
    struct sockaddr_in servidor, cliente;
    

    char *cliente_ip;
    int cliente_port;

    void *connection_handler(void *_socket) {
        char *mensagem;
        char respostaNomeArquivo[MAX_MSG];
        int tamanho;

        // lendo dados enviados pelo cliente
        //mensagem 1 recebido nome do arquivo   
        if ((tamanho = read(conexao, respostaNomeArquivo, MAX_MSG)) < 0) {
            perror("Erro ao receber dados do cliente: ");
            return NULL;
        }
        respostaNomeArquivo[tamanho] = '\0';
        printf("\nO cliente falou sobre o arquivo: %s\n", respostaNomeArquivo);
        
        mensagem = respostaNomeArquivo;
        write(conexao, mensagem, strlen(mensagem));
        printf("O servidor falou sobre ter recebido o nome do arquivo para o cliente: %s\n", mensagem);

        char nomeArquivoAuxiliar[MAX_MSG];

        strncpy(nomeArquivoAuxiliar, respostaNomeArquivo, MAX_MSG);

        if (diretorioParaBuscarArquivo != NULL) {

            while ((arquivoProcurado = readdir(diretorioParaBuscarArquivo)) != NULL) {

                stat(arquivoProcurado->d_name, &mystat);

                printf("Arquivo lido: %s, Arquivo procurado: %s\n", arquivoProcurado->d_name, respostaNomeArquivo);
                if (strcmp(arquivoProcurado->d_name, respostaNomeArquivo) == 0) {
                    printf("Chegou aquii.");
                    closedir(diretorioParaBuscarArquivo);

                    arquivoProcurado = NULL;
                    diretorioParaBuscarArquivo = NULL;
                    diretorioParaBuscarArquivo = opendir(respostaNomeDiretorio);

                    mensagem = "200";
                    //mensagem 2 - enviando confirmação que arquivo existe
                    write(conexao, mensagem, strlen(mensagem));

                    //mensagem 3 - recebendo que arquivo OK do cliente
                    read(conexao, respostaNomeArquivo, MAX_MSG);

                    char arquivo[1024]; 
                    strncpy(arquivo, respostaNomeDiretorio, 1024);
                    strcat(arquivo, nomeArquivoAuxiliar);

                    FILE * file = fopen(arquivo, "rb");
                    if((fseek(file, 0, SEEK_END))<0){ printf("ERRO DURANTE fseek"); }

                    uint32_t len = (int) ftell(file);

                    uint32_t converted_number = htonl(len);
                    
                    printf("[+] Tamanho do arquivo: %d", converted_number);
                    write(_socket, &converted_number, sizeof(converted_number));

                    int fd = open(arquivo, O_RDONLY);
                    off_t offset = 0;
                    int bytesEnviados = 0;

                    if (fd == -1) {
                        fprintf(stderr, "Erro ao abrir arquivo: %s", strerror(errno));

                        exit(EXIT_FAILURE);
                    }

                    while (((bytesEnviados = sendfile(conexao, fd, &offset, BUFSIZ)) > 0) && (len > 0)) {

                        fprintf(stdout, "[+] Servidor enviou %d bytes do arquivo, offset agora é: %d e os dados restantes = %d\n", bytesEnviados, (int)offset, len);
                        len -= bytesEnviados;
                        if (len <= 0) {
                            fprintf(stdout, "[+] Servidor enviou %d bytes do arquivo, offset agora : %d e os dados restantes = %d\n", bytesEnviados, (int)offset, len);
                            break;
                        }
                    }

                    while (1) {
                    }

                }
            }if(arquivoProcurado==NULL) {
                    mensagem = "404";

                    printf("\n//*********************************//\n");

                    printf("Arquivo \"%s\" não existe no diretório: \"%s\"\n",nomeArquivoAuxiliar, respostaNomeDiretorio);

                    write(conexao, mensagem, strlen(mensagem));

                    diretorioParaBuscarArquivo = opendir(respostaNomeDiretorio);

                    while (1) {
                    }

                    close(conexao);

            }
            if (diretorioParaBuscarArquivo != NULL) {
                closedir(diretorioParaBuscarArquivo);
                diretorioParaBuscarArquivo = NULL;
            }
        }

        if (strcmp(respostaNomeArquivo, "bye\n") == 0) {
            close(conexao);
            printf("[+] Servidor finalizado...\n");
            return NULL;
        }
    }

    void *connection_client(void *conexao){
        char respostaServidor[MAX_MSG];
        int tamanho;
        char *mensagemEnviaNomeArquivoRequeridoParaServidor;
        char *mensagem;
        if ((tamanho = read(conexao, respostaServidor, MAX_MSG)) < 0) {
            perror("[-] Erro ao receber dados do cliente.");
            return NULL;
        }
        printf("[+] Servidor recebeu o nome do arquivo a ser gravado: %s.\n", respostaServidor);

        write(conexao, respostaServidor, strlen(respostaServidor));
        printf("[+] O servidor falou sobre ter recebido a mensagem do nome do arquivo: %s\n", respostaServidor);

        FILE *arquivoRecebido;
        arquivoRecebido = fopen(respostaServidor, "w");
        ssize_t len;
        char buffer[BUFSIZ];
        int quantidadeDeBytesRestanteParaSerGravado;

        memset(arquivoRecebido, 0, sizeof arquivoRecebido);
        memset(respostaServidor, 0, sizeof respostaServidor);

        //Recebendo respostaNomeArquivo do servidor
        //mensagemEnviaNomeArquivoRequeridoParaServidor 2 recebendo que arquivo existe   
        if((tamanho = read(conexao, respostaServidor, MAX_MSG)) < 0) {
            printf("[-] Falha ao receber respostaNomeArquivo\n");
            return -1;
        }

        printf("[+] Status: %s da existência do arquivo do lado do cliente.\n", respostaServidor);
        if (strcmp(respostaServidor, "200") == 0) {

            mensagemEnviaNomeArquivoRequeridoParaServidor = "OK";
            //mensagemEnviaNomeArquivoRequeridoParaServidor 3 enviado ok
            write(conexao, mensagemEnviaNomeArquivoRequeridoParaServidor, strlen(mensagemEnviaNomeArquivoRequeridoParaServidor));
            //mensagemEnviaNomeArquivoRequeridoParaServidor 4 recebendo o tamanho do arquivo;
            memset(respostaServidor, 0, sizeof respostaServidor);

            uint32_t received_int;
            read(conexao, &received_int, sizeof(received_int));
            quantidadeDeBytesRestanteParaSerGravado = ntohl(received_int);
        }else{
            fprintf(stderr, "[-] Arquivo não encontrado no cliente.\n");
        }

        while (((len = recv(conexao, buffer, BUFSIZ, 0)) > 0) && (quantidadeDeBytesRestanteParaSerGravado > 0)) {
            fwrite(buffer, sizeof (char), len, arquivoRecebido);
            quantidadeDeBytesRestanteParaSerGravado -= len;
            fprintf(stdout, "[+] Recebidos %d bytes e aguardando: %d bytes\n", len, quantidadeDeBytesRestanteParaSerGravado);
            if (quantidadeDeBytesRestanteParaSerGravado <= 0) {
                break;
            }
        }
        fclose(arquivoRecebido);
        close(conexao);
        if (strcmp(respostaServidor, "200") == 0){
            printf("[+] Servidor recebeu arquivo %s com sucesso!\n", arquivoRecebido);
        }
    }

    _socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket == -1) {
        printf("[-] Não foi possivel criar o _socket\n");
        return -1;
    }

    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY; 
    servidor.sin_port = htons(portaServidor);

    if (bind(_socket, (struct sockaddr *) &servidor, sizeof (servidor)) < 0) {
        puts("[-] Erro ao fazer bind, tente outra porta\n");
        return -1;
    }

    puts("[+] Bind efetuado com sucesso\n");

    listen(_socket, 3);

    puts("[+] Aguardando por conexões...");
    c = sizeof (struct sockaddr_in);
    pthread_t thread;
    char *enviaMensagemParaCliente;

    while ((conexao = accept(_socket, (struct sockaddr *) &cliente, (socklen_t*) & c))){
        int tamanho;
        char respostaServidor[50];
        char *mensagemPostOuGet, *mensagemNomeDiretorio;
        // lendo dados enviados pelo cliente
        //mensagem 1 recebido nome do arquivo  

        cliente_ip = inet_ntoa(cliente.sin_addr);
        cliente_port = ntohs(cliente.sin_port);
        printf("[+] Cliente conectou: %s : [ %d ]\n", cliente_ip, cliente_port);

        if ((tamanho = read(conexao, mensagem_post_ou_get, MAX_MSG)) < 0) {
            perror("[-] Erro ao receber dados do cliente: ");
            return NULL;
        }

        mensagem_post_ou_get[tamanho] = '\0';
        printf("[+] O cliente falou sobre o Método: %s, tamanho %d\n", mensagem_post_ou_get, tamanho);

        mensagemPostOuGet = mensagem_post_ou_get;
        //mensagem 2 - enviando confirmação que arquivo existe do lado do cliente
        write(conexao, mensagemPostOuGet, strlen(mensagemPostOuGet));
        printf("[+] O servidor falou sobre ter recebido a mensagem de post ou get, para o cliente: %s\n", mensagemPostOuGet);

        if(keyfromstring(mensagem_post_ou_get) == 1){
            if ((tamanho = read(conexao, respostaNomeDiretorio, MAX_MSG)) < 0) {
                perror("[-] Erro ao receber dados do cliente: ");
                return NULL;
            }
            respostaNomeDiretorio[tamanho] = '\0';
            diretorioParaBuscarArquivo = opendir(respostaNomeDiretorio);
            if(diretorioParaBuscarArquivo == NULL ){fprintf(stderr, "Diretório %s não existe.\n", respostaNomeDiretorio);return -1;}
            printf("\n[+] O cliente falou sobre nome do diretório: %s\n", respostaNomeDiretorio);
            mensagemNomeDiretorio = respostaNomeDiretorio;
            write(conexao, mensagemNomeDiretorio, strlen(mensagemNomeDiretorio));
            printf("\n[+] O servidor falou sobre que recebeu o nome do diretório: %s\n", mensagemNomeDiretorio);
        }

        switch(keyfromstring(mensagem_post_ou_get)){
            case 1:
                
                if (conexao < 0) {
                    perror("[-] Erro ao receber conexao\n");
                    return -1;
                }

                novaConexao = (int) malloc(1);
                novaConexao = conexao;

                if (pthread_create(&thread, NULL, connection_handler, (void*) novaConexao) < 0) {
                    perror("[-] Não foi possível criar a thread.");
                    return 1;
                }
                puts("[+] GET: Conexão estabelecida");

                if (novaConexao < 0) {
                    perror("[-] Não foi possível estabelecer conexão com o cliente.");
                    return 1;
                }
                break;
            case 2:
                if (conexao < 0) {
                    perror("[-] Erro ao receber conexao\n");
                    return -1;
                }

                novaConexao = (int) malloc(1);
                novaConexao = conexao;

                if (pthread_create(&thread, NULL, connection_client, (void*) novaConexao) < 0) {
                    perror("[-] Não foi possível criar a thread.");
                    return 1;
                }
                puts("[+] POST: Conexão estabelecida");
            
                if (novaConexao < 0) {
                    perror("[-] Não foi possível estabelecer conexão com o cliente.");
                    return 1;
                }
                break;
            default:
                printf("Waiting..."); break;
        }
    }
    

    
}