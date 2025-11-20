#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT "8080" // Porta definida como uma string para o getaddrinfo
#define BUFFER_LENGTH 4096

// Função para lidar com o recebimento tanto do IPv4 quanto do IPv6
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {
    int server_fd, client_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage client_addr;
    socklen_t sin_size;
    int yes = 1;
    char s[INET6_ADDRSTRLEN]; // Buffer para string do IP
    char buffer[BUFFER_LENGTH];
    int rv;

    // Utilizar o getaddrinfo para configurar para IPv4 e IPv6
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // Aceita IPv4 ou IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // Preenche IP automaticamente

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "Erro em getaddrinfo: %s\n", gai_strerror(rv));
        return EXIT_FAILURE;
    }

    // Chamar função socket e bind
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Servidor: socket");
            continue;
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        if (bind(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(server_fd);
            perror("Servidor: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo); // Liberar estrutura addrinfo após o bind

    if (p == NULL) {
        fprintf(stderr, "Servidor: falha ao fazer bind\n");
        return EXIT_FAILURE;
    }

    // Servidor escuta na porta 8080 até algum cliente estabelecer conexão
    if (listen(server_fd, 10) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor escutando na porta %s...\n", PORT);

    while(1) {
        sin_size = sizeof client_addr;
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &sin_size);

        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        // Converte o IP da forma binária para forma textual mais legível
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);
        printf("Servidor: conexão aceita de %s\n", s);

        // Esperar mensagem 'READY' do cliente
        int numbytes;
        memset(buffer, 0, BUFFER_LENGTH);

        if ((numbytes = recv(client_fd, buffer, BUFFER_LENGTH - 1, 0)) == -1) {
            perror("recv");
            close(client_fd);
            continue;
        }
        buffer[numbytes] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        // Enviar mensagem 'READY ACK' para o cliente
        if (strcmp(buffer, "READY") == 0) {
            printf("Comando READY recebido via TCP. Enviando ACK...\n");

            if (send(client_fd, "READY ACK", 9, 0) == -1) {
                perror("send");
            }
        } else {
            printf("Protocolo violado: Esperava READY, recebi '%s'\n", buffer);
            close(client_fd);
            continue;
        }

        // Receber um nome de diretório do cliente
        memset(buffer, 0, BUFFER_LENGTH);
        if ((numbytes = recv(client_fd, buffer, BUFFER_LENGTH - 1, 0)) <= 0) {
            perror("Erro ao receber nome do diretório");
            close(client_fd);
            continue;
        }
        buffer[numbytes] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        // Criar arquivos de saída para serem armazenados
        char output_filename[512];
        snprintf(output_filename, sizeof(output_filename), "server_dump_%s.txt", buffer);

        FILE *fp = fopen(output_filename, "w");
        if (fp == NULL) {
            perror("Erro ao abrir arquivo para gravação");
            close(client_fd);
            continue;
        }
        printf("Salvando lista de arquivos em: %s\n", output_filename);

        // Recepção dos nomes dos arquivos
        int keep_receiving = 1;
        while(keep_receiving) {
            memset(buffer, 0, BUFFER_LENGTH);
            numbytes = recv(client_fd, buffer, BUFFER_LENGTH - 1, 0);

            if (numbytes <= 0) {
                break;
            }
            buffer[numbytes] = '\0';

            char *line = strtok(buffer, "\n");

            while (line != NULL) {
                line[strcspn(line, "\r")] = 0;

                if (strcmp(line, "bye") == 0) {
                    printf("Comando 'bye' recebido. Encerrando conexão\n");
                    keep_receiving = 0;
                    break;
                } else if (strcmp(line, "/bye") == 0) {
                    // Byte Stuffing: Remove o caractere de escape '/' e salva 'bye'
                    fprintf(fp, "bye\n");
                    printf("Recebido arquivo 'bye' (escapado). Arquivo salvo.\n");
                } else {
                    fprintf(fp, "%s\n", line);
                }
                line = strtok(NULL, "\n");
            }
        }

        fclose(fp);
        close(client_fd);
        printf("Conexão encerrada com %s\n\n", s);
    }

    close(server_fd);
    return 0;
}