#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/time.h>

#define BUFFER_LENGTH 4096

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <host do servidor> <porta> <nome do diretório>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *host = argv[1];
    char *port = argv[2];
    char *dir_name = argv[3];

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    char buffer[BUFFER_LENGTH];

    // Variáveis para medição futura do throughput
    struct timeval start, end;
    long total_bytes_sent = 0;

    // Configurar rede tanto para IPv4, como para IPv6
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return EXIT_FAILURE;
    }

    // Tentar conectar em um dos endereços retornados
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("Client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Client: falha ao conectar\n");
        return EXIT_FAILURE;
    }

    // Converte o IP válido versão 6, ou 4, para uma string
    void *addr;
    if (p->ai_family == AF_INET) {
        addr = &(((struct sockaddr_in*)p->ai_addr)->sin_addr);
    } else {
        addr = &(((struct sockaddr_in6*)p->ai_addr)->sin6_addr);
    }
    inet_ntop(p->ai_family, addr, s, sizeof s);
    printf("Client: conectando em %s\n", s);

    freeaddrinfo(servinfo);

    // Envia 'READY' para o servidor
    char *msg_ready = "READY";
    send(sockfd, msg_ready, strlen(msg_ready), 0);

    // Início da medição de tempo para calcular futuramente o throughput
    gettimeofday(&start, NULL);
    printf("Client: READY enviado. Aguardando ACK...\n");

    // Esperar 'READY ACK' do servidor
    int numbytes;
    memset(buffer, 0, BUFFER_LENGTH);
    if ((numbytes = recv(sockfd, buffer, BUFFER_LENGTH - 1, 0)) == -1) {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buffer[numbytes] = '\0';
    buffer[strcspn(buffer, "\r\n")] = 0;

    if (strcmp(buffer, "READY ACK") != 0) {
        fprintf(stderr, "Erro: Servidor não enviou ACK esperado. Recebi: '%s'\n", buffer);
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("Client: ACK recebido. Iniciando transmissão de arquivos...\n");

    // Envia nome do diretório para o servidor
    sprintf(buffer, "%s\n", dir_name);
    int bytes = send(sockfd, buffer, strlen(buffer), 0);
    total_bytes_sent += bytes;

    // Leitura de diretório e envio dele
    DIR *d;
    struct dirent *dir;
    d = opendir(dir_name);

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                continue;
            }

            // Byte Stuffing: Se o arquivo se chama 'bye', envia '/bye'
            char file_packet[512];
            if (strcmp(dir->d_name, "bye") == 0) {
                snprintf(file_packet, sizeof(file_packet), "/bye\n");
                printf("Enviando arquivo com escape: /bye\n");
            } else {
                snprintf(file_packet, sizeof(file_packet), "%s\n", dir->d_name);
            }

            // Envia o nome do arquivo para o servidor
            int sent = send(sockfd, file_packet, strlen(file_packet), 0);
            if (sent == -1) {
                perror("send");
                break;
            }
            total_bytes_sent += sent;
        }
        closedir(d);
    } else {
        perror("Erro ao abrir diretório");
        return EXIT_FAILURE;
    }

    // Se enviar 'bye', fecha a conexão com o servidor
    char *msg_bye = "bye";
    int sent = send(sockfd, msg_bye, strlen(msg_bye), 0);
    total_bytes_sent += sent;

    // Fim da medição de tempo para calcular o throughput
    gettimeofday(&end, NULL);

    close(sockfd);

    // Calcula o tempo decorrido em segundos
    double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;

    // Calcula Throughput (Bytes / Segundo)
    double throughput = total_bytes_sent / time_taken;

    printf("\n--- Relatório de Execução ---\n");
    printf("Tempo total: %.6f segundos\n", time_taken);
    printf("Bytes enviados: %ld bytes\n", total_bytes_sent);
    printf("Throughput: %.2f bytes/segundo\n", throughput);

    return EXIT_SUCCESS;
}