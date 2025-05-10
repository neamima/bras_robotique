#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include "../include/commun.h"

time_t t;
struct tm *tm;
char stime[80];

char *get_current_time() {
    time(&t);
    tm = localtime(&t);
    strftime(stime, sizeof(stime), "%Y-%m-%d %H:%M:%S", tm);
    return stime;
}

volatile sig_atomic_t stop = 0;
int socket_ecoute;

void handle_sigint(int sig) {
    stop = 1;
    close(socket_ecoute);
    printf("\n🛑 Signal SIGINT reçu. Arrêt du serveur en cours...\n");
}




void *gerer_client(void *arg) {
    client_info *client = (client_info *)arg;
    int socket_client = *client->socket_client;
    char buffer[TAILLE_BUFFER];

    while (1) {
        memset(buffer, 0, TAILLE_BUFFER);
        int lu = recv(socket_client, buffer, TAILLE_BUFFER - 1, 0);
        if (lu <= 0) {
            fprintf(logs, "[%s] Client %s:%d (socket %d) déconnecté.\n", get_current_time(),
                   inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port), socket_client);
            fflush(logs);
            printf("Client %s:%d (socket %d) déconnecté.\n" 
                    ,inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port),socket_client);
            break;
        }

        buffer[lu] = '\0';
        fprintf(logs,"[%s] 📨 Message reçu du client %s:%d (socket %d) : %s\n",get_current_time(),
                inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port), socket_client,buffer);
        fflush(logs);
        printf("📨 Message reçu du client %s:%d (socket %d) : %s\n",
                   inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port), socket_client,buffer);

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


            fprintf(logs,"[%s] Outils %d et %d donner au client %s:%d (socket %d)\n",get_current_time(),
                    id1,id2,inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port),socket_client);
            fflush(logs);
            outils[id1].occupe = 1;
            outils[id2].occupe = 1;
            pthread_mutex_unlock(&outils[id2].verrou);
            pthread_mutex_unlock(&outils[id1].verrou);

            send(socket_client, "OK", 3, 0);
        } else if (strncmp(buffer, "LIBERATION_OUTIL", strlen("LIBERATION_OUTIL")) == 0) {
            int id1, id2;
            sscanf(buffer, "LIBERATION_OUTIL %d %d", &id1, &id2);
            printf("🔓 Libération de l’outil %d\n", id1);
            pthread_mutex_lock(&outils[id1].verrou);
            outils[id1].occupe = 0;
            pthread_mutex_unlock(&outils[id1].verrou);
            printf("🔓 Libération de l’outil %d\n", id2);
            pthread_mutex_lock(&outils[id2].verrou);
            outils[id2].occupe = 0;
            pthread_mutex_unlock(&outils[id2].verrou);
            fprintf(logs,"[%s] Outils %d et %d liberé par le client %s:%d (socket %d)\n",get_current_time(),
                    id1,id2,inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port),socket_client);
            fflush(logs);
        } else {
            printf("❓ Message non reconnu : %s\n", buffer);
        }
    }

    close(socket_client);
    return NULL;
}

int main() {
    signal(SIGINT, handle_sigint);
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
    fprintf(logs, "[%s] 🟢 Serveur démarré sur le port %d\n", get_current_time(), PORT);
    fflush(logs);
    printf("🟢 Serveur en attente de connexions sur le port %d...\n", PORT);

    while (!stop) {
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
        

        fprintf(logs, "[%s] 🔗 Connexion acceptée depuis %s:%d (socket %d)\n", get_current_time(),
               inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port), *client->socket_client);
        fflush(logs);
        printf("🔗 Connexion acceptée depuis %s:%d (socket %d)\n",
               inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port), *client->socket_client);

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, gerer_client, client) != 0) {
            perror("Erreur pthread_create");
            close(*socket_client);
            free(socket_client);
        }

        pthread_detach(thread_id);
    }

    for (int i = 0; i < NB_OUTILS; i++) {
        pthread_mutex_destroy(&outils[i].verrou);
    }

    printf("🛑 Arrêt du serveur.\n");
    fprintf(logs, "[%s] 🛑 Arrêt du serveur.\n------------------------------------------------\n", get_current_time());


    fclose(logs);



    return 0;
}
