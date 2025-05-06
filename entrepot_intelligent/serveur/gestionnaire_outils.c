// serveur/gestionnaire_outils.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../include/commun.h"

void *gerer_client(void *arg) {
    int socket_client = *(int *)arg;
    free(arg);
    char buffer[TAILLE_BUFFER];

    while (1) {
        memset(buffer, 0, TAILLE_BUFFER);
        int lu = recv(socket_client, buffer, TAILLE_BUFFER - 1, 0);
        if (lu <= 0) {
            printf("Client déconnecté (socket %d).\n", socket_client);
            break;
        }
        printf("Reçu du client (socket %d) : %s\n", socket_client, buffer);
    }

    close(socket_client);
    return NULL;
}

int main() {
    int socket_ecoute;
    struct sockaddr_in serveur_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    socket_ecoute = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ecoute < 0) {
        perror("Erreur socket");
        exit(1);
    }

    serveur_addr.sin_family = AF_INET;
    serveur_addr.sin_addr.s_addr = INADDR_ANY;
    serveur_addr.sin_port = htons(PORT);

    if (bind(socket_ecoute, (struct sockaddr *)&serveur_addr, sizeof(serveur_addr)) < 0) {
        perror("Erreur bind");
        close(socket_ecoute);
        exit(1);
    }

    listen(socket_ecoute, 10);
    printf("Serveur en attente de connexions sur le port %d...\n", PORT);

    while (1) {
        int *socket_client = malloc(sizeof(int)); // alloué dynamiquement pour chaque thread
        *socket_client = accept(socket_ecoute, (struct sockaddr *)&client_addr, &client_len);
        if (*socket_client < 0) {
            perror("Erreur accept");
            free(socket_client);
            continue;
        }

        printf("Connexion acceptée depuis %s:%d (socket %d)\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), *socket_client);

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, gerer_client, socket_client) != 0) {
            perror("Erreur pthread_create");
            close(*socket_client);
            free(socket_client);
        }

        pthread_detach(thread_id);
    }

    close(socket_ecoute);
    return 0;
}
