// commun.h

#ifndef COMMUN_H
#define COMMUN_H

#define PORT 8080
#define TAILLE_BUFFER 1024

#define NB_OUTILS 5

typedef struct {
    int id;
    int occupe;
    pthread_mutex_t verrou;
} Outil;

typedef struct {
    int *socket_client;
    struct sockaddr_in addr;
} client_info;

Outil outils[NB_OUTILS];

FILE *logs;

#endif