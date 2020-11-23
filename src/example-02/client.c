#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_MSG 1024

/*
 Cliente envia mensagem ao servidor e imprime resposta
 recebida do Servidor
 */

int main(int argc, char *argv[]) {
    FILE *received_file;
    received_file = fopen(argv[3], "w");
    ssize_t len;
    char buffer[BUFSIZ];
    int aba;

      if (argc != 4) {
        fprintf(stderr, "use:./cliente [IP] [Porta] [arquivo]\n");
        return -1;
    } else if (!isdigit(*argv[2])) {
        fprintf(stderr, "Argumento invalido '%s'\n", argv[2]);
        fprintf(stderr, "use:./cliente [IP] [Porta] [arquivo]\n");
        return -1;
    }

    // variaveis
    int socket_desc;
    struct sockaddr_in servidor;
    char *mensagem;
    char resposta_servidor[MAX_MSG];
    int tamanho;
    mensagem = argv[3];
    char* aux1 = argv[2];
    int portaServidor = atoi(aux1);

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

    //Enviando uma mensagem
    //mensagem 1 enviando nome do arquivo.  
    if (send(socket_desc, mensagem, strlen(mensagem), 0) < 0) {
        printf("Erro ao enviar mensagem\n");
        return -1;
    }
    printf("Dados enviados\n");

    memset(mensagem, 0, sizeof mensagem);
    memset(resposta_servidor, 0, sizeof resposta_servidor);

    //Recebendo resposta do servidor
    //mensagem 2 recebendo que arquivo existe   
    if((tamanho = read(socket_desc, resposta_servidor, MAX_MSG)) < 0) {
        printf("Falha ao receber resposta\n");
        return -1;
    }

    printf("Resposta recebida: %s\n", resposta_servidor);
    if (strcmp(resposta_servidor, "200") == 0) {

        mensagem = "OK";
        //mensagem 3 enviado ok
        write(socket_desc, mensagem, strlen(mensagem));
        //mensagem 4 recebendo o tamanho do arquivo;
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
    return 0;
}