#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include "../include/commun.h"

int sock;
pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_outils_dispos = PTHREAD_COND_INITIALIZER;
int outils_ok = 0;

void *thread_idle(void *arg) {
    int sleep_time = rand() % 5 + 1;
    printf("🕓 [Idle] Le bras attend %d secondes.\n", sleep_time);
    sleep(sleep_time);
    printf("✅ [Idle] Fin de l’attente.\n");
    return NULL;
}

void *thread_comm(void *arg) {
    

    char reponse[TAILLE_BUFFER];
    char message[TAILLE_BUFFER];

    int *ids = (int *)arg;
    
    snprintf(message, TAILLE_BUFFER, "DEMANDE_DEUX_OUTILS %d %d", ids[0], ids[1]);
    while (1) {
        send(sock, message, strlen(message), 0);
        printf("📤 [Comm] Message envoyé : %s\n", message);


        int lu = recv(sock, reponse, TAILLE_BUFFER - 1, 0);
        if (lu <= 0) {
            printf("❌ [Comm] Connexion perdue ou serveur fermé.\n");
            break;
        }
        reponse[lu] = '\0';
        printf("📥 [Comm] Réponse du serveur : %s\n", reponse);

        if (strncmp(reponse, "OK", 2) == 0) {
            pthread_mutex_lock(&verrou);
            outils_ok = 1;
            pthread_cond_signal(&cond_outils_dispos);
            pthread_mutex_unlock(&verrou);
            break;
        } else if (strncmp(reponse, "OCCUPE", 6) == 0) {
            printf("⏳ [Comm] En attente des outils...\n");
            sleep(3);

        } else {
            printf("⚠️ [Comm] Réponse inattendue : %s\n", reponse);
            break;
        }
    }

    return NULL;
}

void *thread_assemble(void *arg) {
    int *ids = (int *)arg;
    printf("🔧 [Assemblage] Attente des outils %d et %d...\n", ids[0], ids[1]);

    pthread_mutex_lock(&verrou);
    while (!outils_ok)
        pthread_cond_wait(&cond_outils_dispos, &verrou);
    pthread_mutex_unlock(&verrou);

    printf("🔧 [Assemblage] Outils reçus, début du travail...\n");
    sleep(3);
    printf("✅ [Assemblage] Tâche terminée.\n");

    char message[TAILLE_BUFFER];
    snprintf(message, TAILLE_BUFFER, "LIBERATION_OUTIL %d %d", ids[0], ids[1]);
    send(sock, message, strlen(message), 0);
    printf("📤 [Assemblage] Outils libérés.\n");

    return NULL;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <nombre_de_taches>\n", argv[0]);
        exit(1);
    }

    srand(time(NULL));

    struct sockaddr_in serveur_addr;
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

    printf("🤖 [Client] Connecté au serveur.\n");
    int id1,id2,i=0,t=atoi(argv[1]);
    while(i<t){
        id1 = rand() % NB_OUTILS;
        id2 = rand() % NB_OUTILS;
        while (id1 == id2) {
            id2 = rand() % NB_OUTILS;
        }
        int arg[2]={id1,id2};
        pthread_t t_idle, t_comm, t_assemble;
        pthread_create(&t_idle, NULL, thread_idle, NULL);
        pthread_join(t_idle, NULL);
        pthread_create(&t_comm, NULL, thread_comm, (void*) arg);
        pthread_create(&t_assemble, NULL, thread_assemble, (void*) arg);

        
        pthread_join(t_comm, NULL);
        pthread_join(t_assemble, NULL);
        i++;
    }

    close(sock);
    pthread_mutex_destroy(&verrou);
    pthread_cond_destroy(&cond_outils_dispos);
    printf("👋 [Client] Déconnexion.\n");
    return 0;
}

