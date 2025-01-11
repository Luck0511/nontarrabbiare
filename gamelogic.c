#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define GRID_SIZE 20
#define NUM_PLAYERS 4
#define MAX_PEDINE 4

void stampaGriglia(int posizioni[NUM_PLAYERS][MAX_PEDINE], int pedine_attive[NUM_PLAYERS], int fine) {
    char simboli[NUM_PLAYERS] = {'R', 'Y', 'G', 'B'};
    for (int i = 0; i < fine; i++) {
        for (int j = 0; j < NUM_PLAYERS; j++) {
            int occupato = 0;
            for (int k = 0; k < pedine_attive[j]; k++) {
                if (posizioni[j][k] == i) {
                    printf("[%c%d]", simboli[j], k + 1);
                    occupato = 1;
                    break;
                }
            }
            if (!occupato) {
                printf("[ ]");
            }
        }
        printf("\n");
    }
    printf("\n");
}

int scegliPedina(int pedine_attive) {
    int scelta;
    printf("Scegli quale pedina muovere (1-%d): ", pedine_attive);
    scanf("%d", &scelta);
    return scelta - 1; // Indice della pedina
}

int main() {
    char simboli[NUM_PLAYERS] = {'R', 'Y', 'G', 'B'};
    int posizioni[NUM_PLAYERS][MAX_PEDINE] = {{0}};
    int pedine_attive[NUM_PLAYERS] = {1, 1, 1, 1};
    int pedine_arrivate[NUM_PLAYERS] = {0};
    int fine = GRID_SIZE;
    int turno = 0;

    srand(time(NULL));

    printf("Inizio del gioco!\n\n");

    while (1) {
        printf("\nTurno del giocatore %c:\n", simboli[turno]);

        // Lancia il dado
        int dado = rand() % 6 + 1;
        printf("Il dado mostra: %d\n", dado);

        if (dado == 6 && pedine_attive[turno] < MAX_PEDINE) {
            printf("Vuoi aggiungere una nuova pedina (1) o muovere una pedina esistente (2)? ");
            int scelta;
            scanf("%d", &scelta);

            if (scelta == 1) {
                posizioni[turno][pedine_attive[turno]] = 0;
                pedine_attive[turno]++;
                printf("Nuova pedina aggiunta per il giocatore %c.\n", simboli[turno]);
                stampaGriglia(posizioni, pedine_attive, fine);
                continue;
            }
        }

        // Scegli una pedina da muovere
        int pedina_da_muovere = scegliPedina(pedine_attive[turno]);

        // Calcola la nuova posizione
        int nuovaPosizione = posizioni[turno][pedina_da_muovere] + dado;
        if (nuovaPosizione >= fine) {
            printf("La pedina %d del giocatore %c ha raggiunto il traguardo!\n", pedina_da_muovere + 1, simboli[turno]);
            pedine_arrivate[turno]++;
            posizioni[turno][pedina_da_muovere] = fine - 1; // Rimane sul traguardo

            if (pedine_arrivate[turno] == MAX_PEDINE) {
                printf("Il giocatore %c ha vinto portando tutte le sue pedine al traguardo!\n", simboli[turno]);
                break;
            }

            stampaGriglia(posizioni, pedine_attive, fine);
            turno = (turno + 1) % NUM_PLAYERS; // Passa al turno successivo
            continue;
        }

        // Controlla se un'altra pedina è già nella nuova posizione
        int collisione = 0;
        for (int i = 0; i < NUM_PLAYERS; i++) {
            for (int k = 0; k < pedine_attive[i]; k++) {
                if (i != turno || k != pedina_da_muovere) {
                    if (posizioni[i][k] == nuovaPosizione) {
                        collisione = 1;
                        break;
                    }
                }
            }
            if (collisione) break;
        }

        if (collisione) {
            printf("Collisione! La pedina %d del giocatore %c torna all'inizio.\n", pedina_da_muovere + 1, simboli[turno]);
            posizioni[turno][pedina_da_muovere] = 0;
        } else {
            posizioni[turno][pedina_da_muovere] = nuovaPosizione;
            printf("La pedina %d del giocatore %c si sposta alla posizione %d.\n", pedina_da_muovere + 1, simboli[turno], nuovaPosizione);
        }

        stampaGriglia(posizioni, pedine_attive, fine);

        printf("Attendere 5 secondi per il prossimo turno...\n");
        for (int t = 5; t > 0; t--) {
            fflush(stdout);
            sleep(1);
        }
        printf("\n");

        turno = (turno + 1) % NUM_PLAYERS;
    }

    return 0;
}
