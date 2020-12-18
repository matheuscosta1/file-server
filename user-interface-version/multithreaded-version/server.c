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

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

/*
 Servidor aguarda por mensagem do cliente, imprime na tela
 e depois envia respostaNomeArquivo e finaliza processo
 */

int main(int argc, char* argv[]) {

    //Variaveis auxiliares para encontrar o arquivo a ser transferido.
    DIR *diretorioParaBuscarArquivo;
    struct dirent *arquivoProcurado;
    struct stat mystat;
    char mensagem_post_ou_get[MAX_MSG];
    char respostaNomeArquivo[MAX_MSG];
    char respostaNomeDiretorio[MAX_MSG];
    //verificando se foi executando o comando corretamente
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

    //verificando se o diretorio existe

   //variaveis
    int _socket, conexao, c, novaConexao;
    struct sockaddr_in servidor, cliente;
    

    // para pegar o IP e porta do cliente  
    char *cliente_ip;
    int cliente_port;

    //*********************************************************************//
    //      INICIO DO TRATAMENTO DA THREAD, localizaÃ§Ã£o e transferencia    // 
    //      do arquivo.                                                    // 
    //*********************************************************************//
    void *connection_handler(void *_socket) {
        char *mensagem;
        char respostaNomeArquivo[MAX_MSG];
        int tamanho;

        /*********************************************************/

        /*********comunicao entre cliente/servidor****************/

        // pegando IP e porta do cliente
        cliente_ip = inet_ntoa(cliente.sin_addr);
        cliente_port = ntohs(cliente.sin_port);
        printf("cliente conectou: %s : [ %d ]\n", cliente_ip, cliente_port);

        // lendo dados enviados pelo cliente
        //mensagem 1 recebido nome do arquivo   
        if ((tamanho = read(conexao, respostaNomeArquivo, MAX_MSG)) < 0) {
            perror("Erro ao receber dados do cliente: ");
            return NULL;
        }
        respostaNomeArquivo[tamanho] = '\0';
        printf("\nO cliente falou sobre o arquivo: %s\n", respostaNomeArquivo);
        
        mensagem = "200";
        write(conexao, mensagem, strlen(mensagem));
        printf("O servidor falou sobre ter recebido o nome do arquivo para o cliente: %s\n", mensagem);

        char nomeArquivoAuxiliar[MAX_MSG];
        //fazendo cÃ³pia do nome do arquivo para variÃ¡vel auxiliar. tal variavel Ã© utilzada para localizar
        // o arquivo no diretorio.
        strncpy(nomeArquivoAuxiliar, respostaNomeArquivo, MAX_MSG);
        //printf("ax_nomeArquivo: %s\n", nomeArquivoAuxiliar);

        /*********************************************************/
        if (diretorioParaBuscarArquivo != NULL) {

            //funÃ§Ã£o busca todo o diretÃ³rio buscando o arquivo na variavel nomeArquivoAuxiliar
            //struct stat s;
            while ((arquivoProcurado = readdir(diretorioParaBuscarArquivo)) != NULL) {

                stat(arquivoProcurado->d_name, &mystat);

                printf("Arquivo lido: %s, Arquivo procurado: %s\n", arquivoProcurado->d_name, respostaNomeArquivo);
                if (strcmp(arquivoProcurado->d_name, respostaNomeArquivo) == 0) {//arquivo existe
                    printf("Chegou aquii.");
                    closedir(diretorioParaBuscarArquivo);
                    //Reiniciando variÃ¡veis da pesquisa do diretorio para a proxima thread
                    arquivoProcurado = NULL;
                    diretorioParaBuscarArquivo = NULL;
                    diretorioParaBuscarArquivo = opendir(respostaNomeDiretorio);

                    //**************************************//
                    //      INICIO DO PROTOCOLO            //
                    //*************************************//


                    mensagem = "200";
                    //mensagem 2 - enviando confirmação que arquivo existe
                    write(conexao, mensagem, strlen(mensagem));

                    //mensagem 3 - recebendo que arquivo OK do cliente
                    read(conexao, respostaNomeArquivo, MAX_MSG);

                    //**************************************//
                    //      FIM DO PROTOCOLO               //
                    //*************************************//

                    //abrindo o arquivo e retirando o tamanho//
                    //fazendo cÃ³pia do nome do arquivo para variÃ¡vel auxiliar. tal variavel Ã© utilzada para localizar
                    // o arquivo no diretorio.

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
                    //arquivo = NULL;
                    if (fd == -1) {
                        fprintf(stderr, "Error opening file --> %s", strerror(errno));

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
                    //closedir(diretorioParaBuscarArquivo);
                    while (1) {
                    }

                }
            }if(arquivoProcurado==NULL) {
                    //enviando mensagem para o cliente de arquivo nao encontrado.
                    mensagem = "404";//file not found
                    printf("\n//*********************************//\n");
                    printf("Arquivo \"%s\" não existe no diretório: \"%s\"\n",nomeArquivoAuxiliar, respostaNomeDiretorio);
                    //mensagem 2 - enviando confirmaÃ§Ã£o q arquivo existe
                    write(conexao, mensagem, strlen(mensagem));
                    //sempre que termina de pesquisar o diretorio de arquivos a variavel arquivoProcurado vai para null
                    // entao eh necessario preencher diretorioParaBuscarArquivo novamente com o argv[2] com o diretorio de pesquisa. 
                    //caso contrario novas thread nao acessaram o diretorio passado em argv[2]]
                    diretorioParaBuscarArquivo = opendir(respostaNomeDiretorio);
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

        if (strcmp(respostaNomeArquivo, "bye\n") == 0) {
            close(conexao);
            printf("Servidor finalizado...\n");
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
        printf("O servidor falou sobre ter recebido a mensagem do nome do arquivo: %s\n", respostaServidor);

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
            printf("return status: %d", ntohl(received_int));
            quantidadeDeBytesRestanteParaSerGravado = ntohl(received_int);
            printf("Tamanho do arquivo::: %d", quantidadeDeBytesRestanteParaSerGravado);
            //read(conexao, respostaServidor, 1024);  

            // printf("cliente disse que o tamanho eh: %s", respostaServidor);
            // int tamanhoDoArquivo = atoi(respostaServidor);
            // printf("\nTamanho do arquivo a ser copiado: %d \n", tamanhoDoArquivo);

            

            // int tamanhoDoArquivo = atoi(respostaServidor);
            // write(conexao, tamanhoDoArquivo, strlen(tamanhoDoArquivo));
            // printf("\nTamanho do arquivo a ser copiado: %s \n", respostaServidor);
            // memset(respostaServidor, 0, sizeof respostaServidor);
            // if((tamanho = read(conexao, respostaServidor, MAX_MSG)) < 0) {
            //     printf("[-] Falha ao receber respostaNomeArquivo\n");
            //     return -1;
            // }
            // if (strcmp(respostaServidor, "200") == 0){
            //     quantidadeDeBytesRestanteParaSerGravado = tamanhoDoArquivo;
            // }
            // else{
            //     printf("tamanho nao bate")
            // }

        }else{
            fprintf(stderr, "[-] Arquivo não encontrado no cliente.\n");
        }

        while (((len = recv(conexao, buffer, BUFSIZ, 0)) > 0) && (quantidadeDeBytesRestanteParaSerGravado > 0)) {
            fwrite(buffer, sizeof (char), len, arquivoRecebido);
            quantidadeDeBytesRestanteParaSerGravado -= len;
            fprintf(stdout, "Recebidos %d bytes e aguardando: %d bytes\n", len, quantidadeDeBytesRestanteParaSerGravado);
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

    //*********************************************************************//
    //      FIM DO TRATAMENTO DA THREAD, localizaÃ§Ã£o e transferencia    // 
    //      do arquivo.                                                    // 
    //*********************************************************************//



    //************************************************************
    /*********************************************************/
    //Criando um _socket
    _socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket == -1) {
        printf("[-] Não foi possivel criar o _socket\n");
        return -1;
    }

    //Preparando a struct do _socket
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY; // Obtem IP do S.O.
    servidor.sin_port = htons(portaServidor);

    //Associando o _socket a porta e endereco
    if (bind(_socket, (struct sockaddr *) &servidor, sizeof (servidor)) < 0) {
        puts("[-] Erro ao fazer bind, tente outra porta\n");
        return -1;
    }
    puts("[+] Bind efetuado com sucesso\n");

    // Ouvindo por conexoes
    listen(_socket, 3);
    /*********************************************************/

    //Aceitando e tratando conexoes

    puts("[+] Aguardando por conexões...");
    c = sizeof (struct sockaddr_in);
    pthread_t thread;
    char *enviaMensagemParaCliente;

    while ((conexao = accept(_socket, (struct sockaddr *) &cliente, (socklen_t*) & c))){
        int tamanho;
        char respostaServidor[50];
        char *mensagem;
        // lendo dados enviados pelo cliente
        //mensagem 1 recebido nome do arquivo   
        if ((tamanho = read(conexao, mensagem_post_ou_get, MAX_MSG)) < 0) {
            perror("Erro ao receber dados do cliente: ");
            return NULL;
        }
        
        mensagem_post_ou_get[tamanho] = '\0';
        printf("O cliente falou sobre o Método: %s, tamanho %d\n", mensagem_post_ou_get, tamanho);

        mensagem = mensagem_post_ou_get;
        //mensagem 2 - enviando confirmação que arquivo existe do lado do cliente
        write(conexao, mensagem, strlen(mensagem));
        printf("O servidor falou sobre ter recebido a mensagem de post ou get, para o cliente: %s\n", mensagem);

        //verificando se o diretorio existe
        if(keyfromstring(mensagem_post_ou_get) == 1){
            if ((tamanho = read(conexao, respostaNomeDiretorio, MAX_MSG)) < 0) {
                perror("Erro ao receber dados do cliente: ");
                return NULL;
            }
            respostaNomeDiretorio[tamanho] = '\0';
            diretorioParaBuscarArquivo = opendir(respostaNomeDiretorio);
            if(diretorioParaBuscarArquivo == NULL ){fprintf(stderr, "Diretório %s não existe.\n", respostaNomeDiretorio);return -1;}
            printf("\nO cliente falou sobre nome do diretório: %s\n", respostaNomeDiretorio);
            write(conexao, mensagem, strlen(mensagem));
            printf("\nO servidor falou sobre que recebeu o nome do diretório: %s\n", mensagem);
        }

        switch(keyfromstring(mensagem_post_ou_get)){
            case 1:
                
                if (conexao < 0) {
                    perror("Erro ao receber conexao\n");
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