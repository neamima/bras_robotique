#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/commun.h"

int main() {
    int sock;
    struct sockaddr_in serveur_addr;
    char buffer[TAILLE_BUFFER] = "DEMANDE_DEUX_OUTILS 1 2";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Erreur socket");
        exit(1);
    }

    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_port = htons(PORT);
    serveur_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&serveur_addr, sizeof(serveur_addr)) < 0) {
        perror("Erreur connexion");
        close(sock);
        exit(1);
    }

    printf("Connecté au serveur.\n");
    send(sock, buffer, strlen(buffer), 0);
    printf("Message envoyé : %s\n", buffer);

    close(sock);
    return 0;
}
