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

Outil outils[NB_OUTILS];

#endif