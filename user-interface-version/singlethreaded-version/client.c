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
#include <time.h>

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

/*
 Cliente envia mensagemEnviaNomeArquivoRequeridoParaServidor ao servidor e imprime resposta
 recebida do Servidor
 */

int main(int argc, char *argv[]) {

    struct dirent *arquivoProcurado;
    struct stat mystat;

    if (argc != 3) {
        fprintf(stderr, "use:./cliente [IP] [Porta]\n");
        return -1;
    } else if (!isdigit(*argv[2])) {
        fprintf(stderr, "Argumento invalido '%s'\n", argv[2]);
        fprintf(stderr, "use:./cliente [IP] [Porta]\n");
        return -1;
    }

    // variaveis
    int _socket, conexao, novaConexao;
    DIR *diretorioParaBuscarArquivo;
    struct sockaddr_in servidor;

    /*****************************************/
    /* Criando um _socket */
    // AF_INET = ARPA INTERNET PROTOCOLS
    // SOCK_STREAM = orientado a conexao
    // 0 = protocolo padrao para o tipo escolhido -- TCP
    _socket = socket(AF_INET, SOCK_STREAM, 0);

    if (_socket == -1) {
        printf("Nao foi possivel criar _socket\n");
        return -1;
    }

    /* Informacoes para conectar no servidor */
    // IP do servidor
    // familia ARPANET
    // Porta - hton = host to network short (2bytes)

    char* portaServidorAuxiliar = argv[2];
    int portaServidor = atoi(portaServidorAuxiliar);

    servidor.sin_addr.s_addr = inet_addr(argv[1]);
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(portaServidor);

    //Conectando no servidor remoto
    if (conexao = connect(_socket, (struct sockaddr *) &servidor, sizeof (servidor)) < 0) {
        printf("Nao foi possivel conectar\n");
        return -1;
    }
    
    printf("Conectado no servidor\n");
    
    char *mensagemEnviaNomeArquivoRequeridoParaServidor, *mensagemEnviaNomeDiretorioParaServidor;
    char respostaServidor[MAX_MSG];
    int tamanho;

    void *connection_handler(void *_socket, char *arquivo, char *diretorio) {
        char *mensagem;
        char resposta[MAX_MSG];
        int tamanho;
        char diretorioArquivo[50];
        char mensagemEnviaNomeArquivoRequeridoParaServidor[50];
        diretorioParaBuscarArquivo = opendir(diretorio);

        tamanho = strlen(arquivo);

        for(int i=0; i < tamanho; i++){
            printf("[%c]\n", arquivo[i]);
            resposta[i] = arquivo[i];
        }
        resposta[tamanho] = '\0';
        for(int i=0; i < tamanho; i++){
            printf("[%c]\n", resposta[i]);
        }
        printf("resposta: %s, %s", resposta, arquivo);

        if(diretorioParaBuscarArquivo == NULL ){fprintf(stderr, "Argumento invalido '%s'\n", diretorio);return -1;}
        
        char nomeArquivoAuxiliar[MAX_MSG];
        //fazendo cÃ³pia do nome do arquivo para variÃ¡vel auxiliar. tal variavel Ã© utilzada para localizar
        // o arquivo no diretorio.
        strncpy(nomeArquivoAuxiliar, resposta, MAX_MSG);
        //printf("ax_nomeArquivo: %s\n", nomeArquivoAuxiliar);

        /*********************************************************/
        if (diretorioParaBuscarArquivo != NULL) {

            //funÃ§Ã£o busca todo o diretÃ³rio buscando o arquivo na variavel nomeArquivoAuxiliar
            //struct stat s;
            while ((arquivoProcurado = readdir(diretorioParaBuscarArquivo)) != NULL) {

                stat(arquivoProcurado->d_name, &mystat);

                printf("Arquivo lido: %s, Arquivo procurado: %s, Comparação:%d\n", arquivoProcurado->d_name, resposta, strcmp(arquivoProcurado->d_name, resposta));
                if (strcmp(arquivoProcurado->d_name, resposta) == 0) {//arquivo existe
                    printf("Arquivo existe lado do cliente");
                    closedir(diretorioParaBuscarArquivo);
                    //Reiniciando variÃ¡veis da pesquisa do diretorio para a proxima thread
                    arquivoProcurado = NULL;
                    diretorioParaBuscarArquivo = NULL;
                    diretorioParaBuscarArquivo = opendir(arquivo);

                    //**************************************//
                    //      INICIO DO PROTOCOLO            //
                    //*************************************//

                    mensagem = "200";
                    printf("mensagem %s", mensagem);
                    //mensagem 2 - enviando confirmação que arquivo existe do lado do cliente
                    write(_socket, mensagem, strlen(mensagem));

                    //mensagem 3 - recebendo que arquivo OK do servidor
                    read(_socket, resposta, MAX_MSG);

                    //**************************************//
                    //      FIM DO PROTOCOLO               //
                    //*************************************//

                    //abrindo o arquivo e retirando o tamanho//
                    //fazendo cÃ³pia do nome do arquivo para variÃ¡vel auxiliar. tal variavel Ã© utilzada para localizar
                    // o arquivo no diretorio.

                    char abrirArquivo[1024]; 
                    strncpy(abrirArquivo, diretorio, 1024);
                    strcat(abrirArquivo, nomeArquivoAuxiliar);

                    FILE * file = fopen(abrirArquivo, "rb");
                    if((fseek(file, 0, SEEK_END))<0){printf("ERRO DURANTE fseek");}
                    int len = (int) ftell(file);                   
                    mensagem = (char*) len;
                    printf("Tamanho do arquivo: %d\n", len);
                    //convertendo o valor do tamanho do arquivo (int) para ser enviado em uma mensagem no scoket(char)
                    char tamanhoDoArquivoEmFormatoChar[32];
                    sprintf(tamanhoDoArquivoEmFormatoChar, "%d", len);
                    mensagem = tamanhoDoArquivoEmFormatoChar;

                    //mensagem 4 - enviando o tamanho do arquivo para o servidor
                    send(_socket, mensagem, strlen(mensagem), 0);

                    int fd = open(abrirArquivo, O_RDONLY);
                    off_t offset = 0;
                    int bytesEnviados = 0;
                    //arquivo = NULL;
                    if (fd == -1) {
                        fprintf(stderr, "Erro ao abrir arquivo: %s", strerror(errno));

                        exit(EXIT_FAILURE);
                    }

                    while (((bytesEnviados = sendfile(_socket, fd, &offset, BUFSIZ)) > 0) && (len > 0)) {

                        fprintf(stdout, "[+] Cliente enviou %d bytes do arquivo, offset agora é: %d e os dados restantes = %d\n", bytesEnviados, (int)offset, len);
                        len -= bytesEnviados;
                        if (len <= 0) {
                            fprintf(stdout, "[+] Cliente enviou %d bytes do arquivo, offset agora é: %d e os dados restantes = %d\n", bytesEnviados, (int)offset, len);
                            printf("[+] Cliente enviou todos os dados do arquivo com sucesso.\n");
                            printf("[+] Cliente terminando...\n");
                            exit(0);
                        }
                    }
                    while(1){
                    }
                    //closedir(diretorioParaBuscarArquivo);

                }
            }if(arquivoProcurado==NULL) {
                    //enviando mensagem para o cliente de arquivo nao encontrado.
                    mensagem = "404";//file not found
                    printf("\n//*********************************//\n");
                    printf("[-] Arquivo \"%s\" não existe no diretório: \"%s\"\n",nomeArquivoAuxiliar, diretorio);
                    //mensagem 2 - enviando confirmaÃ§Ã£o q arquivo existe
                    write(_socket, mensagem, strlen(mensagem));
                    //sempre que termina de pesquisar o diretorio de arquivos a variavel arquivoProcurado vai para null
                    // entao eh necessario preencher diretorioParaBuscarArquivo novamente com o argv[2] com o diretorio de pesquisa. 
                    //caso contrario novas thread nao acessaram o diretorio passado em argv[2]]
                    diretorioParaBuscarArquivo = opendir(diretorio);
                    //
                    printf("[-] Cliente finalizando...\n");
                    exit(0);
                    //closedir(diretorioParaBuscarArquivo);

            }
            if (diretorioParaBuscarArquivo != NULL) {
                closedir(diretorioParaBuscarArquivo);
                diretorioParaBuscarArquivo = NULL;
            }
        }

        if (strcmp(arquivo, "bye\n") == 0) {
            close(_socket);
            printf("Cliente finalizado...\n");
            return NULL;
        }
    }

    char post_ou_get[50];
    char arquivo[50];
    char diretorio[50];
    int flag = 1;
    while(conexao == 0){
        printf("Seja bem vindo ao servidor de arquivos!\n");
        printf("Operações disponíveis para efetuar no servidor: POST/GET\n");
            
        printf("Use: [METODO POST OU GET] [DIRETORIO] [ARQUIVO]:\n\n");
        printf("input$ ");
        scanf("%s %s %s", &post_ou_get, &diretorio, &arquivo);
        switch(keyfromstring(post_ou_get)){
            case(0):
                return -1;
            case(1):
                mensagemEnviaNomeArquivoRequeridoParaServidor = arquivo;
                mensagemEnviaNomeDiretorioParaServidor = diretorio;
                FILE *arquivoRecebido;
                arquivoRecebido = fopen(arquivo, "w");
                ssize_t len;
                char buffer[BUFSIZ];
                int quantidadeDeBytesRestanteParaSerGravado;

                if (send(_socket, post_ou_get, strlen(post_ou_get), 0) < 0) {
                    printf("Erro ao enviar post_ou_get\n");
                    return -1;
                }
                printf("Dados enviados %s\n", post_ou_get);

                memset(post_ou_get, 0, sizeof post_ou_get);
                memset(respostaServidor, 0, sizeof respostaServidor);

                if (send(_socket, mensagemEnviaNomeDiretorioParaServidor, strlen(mensagemEnviaNomeDiretorioParaServidor), 0) < 0) {
                    printf("Erro ao enviar mensagemEnviaNomeDiretorioParaServidor\n");
                    return -1;
                }
                printf("Dados enviados %s\n", mensagemEnviaNomeDiretorioParaServidor);

                memset(mensagemEnviaNomeDiretorioParaServidor, 0, sizeof mensagemEnviaNomeDiretorioParaServidor);
                memset(respostaServidor, 0, sizeof respostaServidor);

                //Enviando uma mensagemEnviaNomeArquivoRequeridoParaServidor
                //mensagemEnviaNomeArquivoRequeridoParaServidor 1 enviando nome do arquivo.  
                if (send(_socket, mensagemEnviaNomeArquivoRequeridoParaServidor, strlen(mensagemEnviaNomeArquivoRequeridoParaServidor), 0) < 0) {
                    printf("Erro ao enviar mensagemEnviaNomeArquivoRequeridoParaServidor\n");
                    return -1;
                }
                printf("Dados enviados %s\n", mensagemEnviaNomeArquivoRequeridoParaServidor);

                memset(mensagemEnviaNomeArquivoRequeridoParaServidor, 0, sizeof mensagemEnviaNomeArquivoRequeridoParaServidor);
                memset(respostaServidor, 0, sizeof respostaServidor);

                //Recebendo resposta do servidor
                //mensagemEnviaNomeArquivoRequeridoParaServidor 2 recebendo que arquivo existe   
                if((tamanho = read(_socket, respostaServidor, MAX_MSG)) < 0) {
                    printf("Falha ao receber resposta\n");
                    return -1;
                }

                printf("Resposta recebida: %s\n", respostaServidor);
                if (strcmp(respostaServidor, "200") == 0) {

                    mensagemEnviaNomeArquivoRequeridoParaServidor = "OK";
                    //mensagemEnviaNomeArquivoRequeridoParaServidor 3 enviado ok
                    write(_socket, mensagemEnviaNomeArquivoRequeridoParaServidor, strlen(mensagemEnviaNomeArquivoRequeridoParaServidor));
                    //mensagemEnviaNomeArquivoRequeridoParaServidor 4 recebendo o tamanho do arquivo;
                    memset(respostaServidor, 0, sizeof respostaServidor);
                    read(_socket, respostaServidor, 1024);

                    int tamanhoDoArquivo = atoi(respostaServidor);
                    printf("\nTamanho do arquivo a ser copiado: %s \n", respostaServidor);
                    quantidadeDeBytesRestanteParaSerGravado = tamanhoDoArquivo;

                }else{
                    fprintf(stderr, "Arquivo não encontrado no servidor'%s'\n", diretorio);
                    close(_socket);
                    printf("Cliente finalizado com sucesso!\n");
                    return 0;
                }

                while (((len = recv(_socket, buffer, BUFSIZ, 0)) > 0)&& (quantidadeDeBytesRestanteParaSerGravado > 0)) {
                    fwrite(buffer, sizeof (char), len, arquivoRecebido);
                    quantidadeDeBytesRestanteParaSerGravado -= len;
                    fprintf(stdout, "Recebidos %d bytes e aguardamos :- %d bytes\n", len, quantidadeDeBytesRestanteParaSerGravado);
                    if (quantidadeDeBytesRestanteParaSerGravado <= 0) {
                        break;
                    }
                }
                fclose(arquivoRecebido);
                close(_socket);
                printf("Cliente finalizado com sucesso!\n");
                flag = 0;
                break;
            case(2):
                if (send(_socket, post_ou_get, strlen(post_ou_get), 0) < 0) {
                    printf("Erro ao enviar post_ou_get\n");
                    return -1;
                }
                printf("Dados enviados %s\n", post_ou_get);

                memset(post_ou_get, 0, sizeof post_ou_get);
                memset(respostaServidor, 0, sizeof respostaServidor);

                if (send(_socket, arquivo, strlen(arquivo), 0) < 0) {
                    printf("Erro ao enviar nome do arquivo para o servidor.\n");
                    return -1;
                }
                
                memset(respostaServidor, 0, sizeof respostaServidor);

                printf("O cliente enviou nome do arquivo %s para o servidor\n", arquivo);
                connection_handler(_socket, arquivo, diretorio);
                flag = 0;
                break;            
            }
    }
    

    return 0;
}