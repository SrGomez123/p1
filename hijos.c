#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

//Funciones de Gestión de Memoria Compartida

pid_t* inicializar_memoria(int x, int y, int *shmid) {
    int size = (x + y) * sizeof(pid_t);
    
    if ((*shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0666)) == -1) {
        perror("Error al crear memoria compartida");
        exit(1);
    }
    
    return (pid_t *)shmat(*shmid, 0, 0);
}

void limpiar_memoria(int shmid, pid_t *memoria) {
    shmdt((char *)memoria);
    shmctl(shmid, IPC_RMID, 0);
}

//Funciones de Gestión de Procesos

int generar_cadena_vertical(int x, pid_t *padres) {
    int nivel = 0;
    padres[0] = getpid(); // El superpadre guarda su PID

    for (int i = 1; i < x; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Error en fork");
            exit(1);
        }
        
        if (pid == 0) {
            nivel = i;
            padres[nivel] = getpid();
        } else {
            wait(NULL); // Espera al siguiente eslabón
            
            if (nivel == 0) {
                return 0; // El superpadre retorna al main para esperar a la rama y finalizar
            } else {
                exit(0); // Los padres intermedios mueren al terminar su hijo
            }
        }
    }
    return nivel;
}

void generar_subhijos_horizontales(int x, int y, pid_t *padres, pid_t *hijos_finales) {
    for (int j = 0; j < y; j++) {
        pid_t pid_hoja = fork();
        
        if (pid_hoja == 0) {
            hijos_finales[j] = getpid();
            
            printf("Soy el subhijo %d, mi padres son: ", getpid());
            for (int k = 0; k < x; k++) {
                printf("%d", padres[k]);
                if (k < x - 1) printf(", ");
            }
            printf("\n");
            
            exit(0);
        }
    }

    // Esperar a que todos los subhijos mueran
    for (int j = 0; j < y; j++) {
        wait(NULL);
    }
}

void imprimir_resultados_superpadre(int y, pid_t *hijos_finales) {
    printf("Soy el superpadre (%d) : mis hijos finales son: ", getpid());
    for (int j = 0; j < y; j++) {
        printf("%d", hijos_finales[j]);
        if (j < y - 1) printf(", ");
    }
    printf("\n");
}

// --- Función Principal ---

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s x y\n", argv[0]);
        exit(1);
    }

    int x = atoi(argv[1]);
    int y = atoi(argv[2]);

    if (x < 1 || y < 1) {
        printf("x e y deben ser mayores que 0\n");
        exit(1);
    }

    int shmid;
    pid_t *memoria = inicializar_memoria(x, y, &shmid);
    
    // Punteros lógicos para las dos partes de la memoria
    pid_t *padres = memoria;               
    pid_t *hijos_finales = memoria + x;    

    // 1. Generar la cadena vertical de 'x' niveles
    int nivel = generar_cadena_vertical(x, padres);

    // 2. El último nodo de la cadena crea la rama horizontal
    if (nivel == x - 1) {
        generar_subhijos_horizontales(x, y, padres, hijos_finales);
        
        // Si no es el superpadre (x > 1), muere tras esperar a las ramas
        if (nivel != 0) {
            exit(0);
        }
    }

    // 3. Imprimir el mensaje final y limpiar el IPC (sólo el superpadre)
    if (nivel == 0) {
        imprimir_resultados_superpadre(y, hijos_finales);
        limpiar_memoria(shmid, memoria);
    }

    return 0;
}
