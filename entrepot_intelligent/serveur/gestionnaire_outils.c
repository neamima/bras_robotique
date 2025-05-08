// serveur/gestionnaire_outils.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../include/commun.h"



void *gerer_client(void *arg) {
    client_info *client = (client_info *)arg;
    int socket_client = *client->socket_client;
    char buffer[TAILLE_BUFFER];

    while (1) {
        memset(buffer, 0, TAILLE_BUFFER);
        int lu = recv(socket_client, buffer, TAILLE_BUFFER - 1, 0);
        if (lu <= 0) {
            printf("Client déconnecté %s:%d (socket %d).\n" 
                    ,inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port),socket_client);
            break;
        }

        buffer[lu] = '\0'; // très important
        printf("📨 Message reçu du client : %s\n", buffer);

        if (strncmp(buffer, "DEMANDE_DEUX_OUTILS", strlen("DEMANDE_DEUX_OUTILS")) == 0) {
            int id1, id2;
            sscanf(buffer, "DEMANDE_DEUX_OUTILS %d %d", &id1, &id2);
            printf("🔧 Client demande les outils %d et %d\n", id1, id2);

            if (id1 > id2) { int tmp = id1; id1 = id2; id2 = tmp; }

            pthread_mutex_lock(&outils[id1].verrou);
            if (outils[id1].occupe) {
                pthread_mutex_unlock(&outils[id1].verrou);
                send(socket_client, "OCCUPE", 6, 0);
                continue;
            }

            pthread_mutex_lock(&outils[id2].verrou);
            if (outils[id2].occupe) {
                pthread_mutex_unlock(&outils[id2].verrou);
                pthread_mutex_unlock(&outils[id1].verrou);
                send(socket_client, "OCCUPE", 6, 0);
                continue;
            }

            outils[id1].occupe = 1;
            outils[id2].occupe = 1;
            pthread_mutex_unlock(&outils[id2].verrou);
            pthread_mutex_unlock(&outils[id1].verrou);

            send(socket_client, "OK\n", 3, 0);
        } else if (strncmp(buffer, "LIBERATION_OUTIL", strlen("LIBERATION_OUTIL")) == 0) {
            // gérer plusieurs lignes
            char *ligne = strtok(buffer, "\n");
            while (ligne != NULL) {
                int id;
                if (sscanf(ligne, "LIBERATION_OUTIL %d", &id) == 1) {
                    printf("🔓 Libération de l’outil %d\n", id);
                    pthread_mutex_lock(&outils[id].verrou);
                    outils[id].occupe = 0;
                    pthread_mutex_unlock(&outils[id].verrou);
                }
                ligne = strtok(NULL, "\n");
            }
        } else {
            printf("❓ Message non reconnu : %s\n", buffer);
        }
    }

    close(socket_client);
    return NULL;
}

int main() {
    int socket_ecoute;
    struct sockaddr_in serveur_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    logs = fopen("logs.txt", "a");

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

    for (int i = 0; i < NB_OUTILS; i++) {
        outils[i].id = i;
        outils[i].occupe = 0;
        pthread_mutex_init(&outils[i].verrou, NULL);
    }

    listen(socket_ecoute, 10);
    printf("🟢 Serveur en attente de connexions sur le port %d...\n", PORT);

    while (1) {
        int *socket_client = malloc(sizeof(int));
        *socket_client = accept(socket_ecoute, (struct sockaddr *)&client_addr, &client_len);
        if (*socket_client < 0) {
            perror("Erreur accept");
            free(socket_client);
            continue;
        }

        client_info *client=malloc(sizeof(client_info));
        client->socket_client = socket_client;
        client->addr = client_addr;

        printf("✅ Connexion acceptée depuis %s:%d (socket %d)\n",
               inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port), *client->socket_client);

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, gerer_client, client) != 0) {
            perror("Erreur pthread_create");
            close(*socket_client);
            free(socket_client);
        }

        pthread_detach(thread_id);
    }

    close(socket_ecoute);
    fclose(logs);
    return 0;
}
