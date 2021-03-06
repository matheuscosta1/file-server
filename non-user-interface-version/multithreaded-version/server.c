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
    DIR *diretorioParaBuscarArquivo;
    struct dirent *arquivoProcurado;
    struct stat mystat;
    //verificando se foi executando o comando corretamente
    if (argc != 4) {
        fprintf(stderr, "Utilize a seguinte especificação:\n./server [Porta] [Diretório para buscar arquivo] [Tipo de Operação]\n");
        return -1;
    } else if (!isdigit(*argv[1])) {
        fprintf(stderr, "Argumento inválido: '%s'\n", argv[1]);
        fprintf(stderr, "Utilize a seguinte especificação:\n./server [Porta] [Diretório para buscar arquivo] [Tipo de Operação]\n");
        return -1;
    } else if (!isdigit(*argv[3])) {
        fprintf(stderr, "Argumento inválido: '%s'\n", argv[3]);
        fprintf(stderr, "Utilize a seguinte especificação:\n./server [Porta] [Diretório para buscar arquivo] [Tipo de Operação]\n");
        return -1;
    } 

    diretorioParaBuscarArquivo = opendir(argv[2]);

    //verificando se o diretorio existe
    if(diretorioParaBuscarArquivo == NULL ){fprintf(stderr, "Argumento invalido '%s'\n", argv[2]);return -1;}

    char* portaServidorAuxiliar = argv[1];
        int portaServidor = atoi(portaServidorAuxiliar);

    //verificando se o diretorio existe

   //variaveis
    int _socket, conexao, c, novaConexao;
    struct sockaddr_in servidor, cliente;
    
    
    char* tipoTransacaoAuxiliar = argv[3];
    int tipoTransacao = atoi(tipoTransacaoAuxiliar);

    // para pegar o IP e porta do cliente  
    char *cliente_ip;
    int cliente_port;

    //*********************************************************************//
    //      INICIO DO TRATAMENTO DA THREAD, localizaÃ§Ã£o e transferencia    // 
    //      do arquivo.                                                    // 
    //*********************************************************************//
    void *connection_handler(void *_socket) {
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
        printf("Tamanho: %d", tamanho);
        resposta[tamanho] = '\0';
        printf("O cliente falou: %s\n", resposta);

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

                printf("Arquivo lido: %s, Arquivo procurado: %s\n", arquivoProcurado->d_name, resposta);
                if (strcmp(arquivoProcurado->d_name, resposta) == 0) {//arquivo existe
                   closedir(diretorioParaBuscarArquivo);
                    //Reiniciando variÃ¡veis da pesquisa do diretorio para a proxima thread
                    arquivoProcurado = NULL;
                    diretorioParaBuscarArquivo = NULL;
                    diretorioParaBuscarArquivo = opendir(argv[2]);

                    //**************************************//
                    //      INICIO DO PROTOCOLO            //
                    //*************************************//


                    mensagem = "200";
                    //mensagem 2 - enviando confirmação que arquivo existe
                    write(conexao, mensagem, strlen(mensagem));

                    //mensagem 3 - recebendo que arquivo OK do cliente
                    read(conexao, resposta, MAX_MSG);

                    //**************************************//
                    //      FIM DO PROTOCOLO               //
                    //*************************************//

                    //abrindo o arquivo e retirando o tamanho//
                    //fazendo cÃ³pia do nome do arquivo para variÃ¡vel auxiliar. tal variavel Ã© utilzada para localizar
                    // o arquivo no diretorio.

                    char arquivo[1024]; 
                    strncpy(arquivo, argv[2], 1024);
                    strcat(arquivo, nomeArquivoAuxiliar);

                    FILE * file = fopen(arquivo, "rb");
                    if((fseek(file, 0, SEEK_END))<0){printf("ERRO DURANTE fseek");}
                    int len = (int) ftell(file);                   
                    mensagem = (char*) len;
                    printf("Tamanho do arquivo: %d\n", len);
                    //convertendo o valor do tamanho do arquivo (int) para ser enviado em uma mensagem no scoket(char)
                    char tamanhoDoArquivoEmFormatoChar[32];

                    sprintf(tamanhoDoArquivoEmFormatoChar, "%d", len);

                    mensagem = tamanhoDoArquivoEmFormatoChar;

                    //mensagem 4 - enviando o tamanho do arquivo
                    send(conexao, mensagem, strlen(mensagem), 0);

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
                    printf("Arquivo \"%s\" não existe no diretório: \"%s\"\n",nomeArquivoAuxiliar, argv[2]);
                    //mensagem 2 - enviando confirmaÃ§Ã£o q arquivo existe
                    write(conexao, mensagem, strlen(mensagem));
                    //sempre que termina de pesquisar o diretorio de arquivos a variavel arquivoProcurado vai para null
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

    void *connection_client(void *conexao){
        char respostaServidor[MAX_MSG];
        int tamanho;
        char *mensagemEnviaNomeArquivoRequeridoParaServidor;
        if ((tamanho = read(conexao, respostaServidor, MAX_MSG)) < 0) {
            perror("[-] Erro ao receber dados do cliente.");
            return NULL;
        }
        printf("[+] Servidor recebeu o nome do arquivo a ser gravado: %s.\n", respostaServidor);
        
        FILE *arquivoRecebido;
        arquivoRecebido = fopen(respostaServidor, "w");
        ssize_t len;
        char buffer[BUFSIZ];
        int quantidadeDeBytesRestanteParaSerGravado;

        memset(arquivoRecebido, 0, sizeof arquivoRecebido);
        memset(respostaServidor, 0, sizeof respostaServidor);

        //Recebendo resposta do servidor
        //mensagemEnviaNomeArquivoRequeridoParaServidor 2 recebendo que arquivo existe   
        if((tamanho = read(conexao, respostaServidor, MAX_MSG)) < 0) {
            printf("[-] Falha ao receber resposta\n");
            return -1;
        }

        printf("[+] Status: %s da existência do arquivo do lado do cliente.\n", respostaServidor);
        if (strcmp(respostaServidor, "200") == 0) {

            mensagemEnviaNomeArquivoRequeridoParaServidor = "OK";
            //mensagemEnviaNomeArquivoRequeridoParaServidor 3 enviado ok
            write(conexao, mensagemEnviaNomeArquivoRequeridoParaServidor, strlen(mensagemEnviaNomeArquivoRequeridoParaServidor));
            //mensagemEnviaNomeArquivoRequeridoParaServidor 4 recebendo o tamanho do arquivo;
            memset(respostaServidor, 0, sizeof respostaServidor);
            read(conexao, respostaServidor, 1024);

            int tamanhoDoArquivo = atoi(respostaServidor);
            printf("\nTamanho do arquivo a ser copiado: %s \n", respostaServidor);
            quantidadeDeBytesRestanteParaSerGravado = tamanhoDoArquivo;

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
    
    switch (tipoTransacao){
        case 1:
            while ((conexao = accept(_socket, (struct sockaddr *) &cliente, (socklen_t*) & c))) {
                if (conexao < 0) {
                    perror("Erro ao receber conexao\n");
                    return -1;
                }

                pthread_t thread;
                novaConexao = (int) malloc(1);
                novaConexao = conexao;

                if (pthread_create(&thread, NULL, connection_handler, (void*) novaConexao) < 0) {
                    perror("[-] Não foi possível criar a thread.");
                    return 1;
                }
                puts("[+] Conexão estabelecida");
            }
            if (novaConexao < 0) {
                perror("[-] Não foi possível estabelecer conexão com o cliente.");
                return 1;
            }
        case 2:
            while ((conexao = accept(_socket, (struct sockaddr *) &cliente, (socklen_t*) & c))) {
                if (conexao < 0) {
                    perror("Erro ao receber conexao\n");
                    return -1;
                }

                pthread_t thread;
                novaConexao = (int) malloc(1);
                novaConexao = conexao;

                if (pthread_create(&thread, NULL, connection_client, (void*) novaConexao) < 0) {
                    perror("[-] Não foi possível criar a thread.");
                    return 1;
                }
                puts("[+] Conexão estabelecida");
            }
            if (novaConexao < 0) {
                perror("[-] Não foi possível estabelecer conexão com o cliente.");
                return 1;
            }
        default:
            printf("Waiting...");
    }

    
}