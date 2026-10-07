#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(int argc, char *argv[]) {
    // Validar parámetros
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

    // --- 1. CONFIGURACIÓN DE MEMORIA COMPARTIDA ---
    int shmid;
    pid_t *memoria;
    int size = (x + y) * sizeof(pid_t);

    // Obtención del segmento IPC_PRIVATE con permisos de lectura/escritura
    if ((shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0666)) == -1) {
        perror("Error al crear memoria compartida");
        exit(1);
    }

    // Vinculación del segmento al proceso
    memoria = (pid_t *)shmat(shmid, 0, 0);
    
    // Organizamos el array único en dos bloques lógicos:
    pid_t *padres = memoria;               // Primeros 'x' huecos para la cadena vertical
    pid_t *hijos_finales = memoria + x;    // Siguientes 'y' huecos para los subhijos

    // --- 2. CREACIÓN DEL ÁRBOL VERTICAL (x niveles) ---
    int nivel = 0;
    padres[0] = getpid(); // El superpadre guarda su PID en la raíz

    for (int i = 1; i < x; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Error en fork");
            exit(1);
        }
        
        if (pid == 0) {
            // PROCESO HIJO: Actualiza su nivel, registra su PID en la memoria y avanza en el bucle
            nivel = i;
            padres[nivel] = getpid();
        } else {
            // PROCESO PADRE: Espera a que su hijo (el siguiente eslabón) termine por completo
            wait(NULL);
            
            if (nivel == 0) {
                // Si es el superpadre, rompe el bucle para ir a la sección final (imprimir resultados y limpiar)
                break;
            } else {
                // Si es un padre intermedio de la cadena vertical, ya ha cumplido su ciclo y muere
                exit(0);
            }
        }
    }

    // --- 3. CREACIÓN DE LA RAMA HORIZONTAL (sólo la ejecuta el proceso del nivel x-1) ---
    if (nivel == x - 1) {
        for (int j = 0; j < y; j++) {
            pid_t pid_hoja = fork();
            
            if (pid_hoja == 0) {
                // DENTRO DEL SUBHIJO (Hoja del árbol)
                hijos_finales[j] = getpid(); // Registra su PID para que el superpadre lo pueda leer
                
                // Imprime su propio mensaje requerido
                printf("Soy el subhijo %d, mi padres son: ", getpid());
                for (int k = 0; k < x; k++) {
                    printf("%d", padres[k]);
                    if (k < x - 1) printf(", ");
                }
                printf("\n");
                
                exit(0); // El subhijo termina su ejecución
            }
        }

        // El padre del final de la cadena vertical espera a que todos sus 'y' subhijos terminen
        for (int j = 0; j < y; j++) {
            wait(NULL);
        }

        // Si x > 1, este eslabón intermedio muere aquí. 
        // (Si x == 1, este proceso coincide con el superpadre, por tanto NO debe morir aún).
        if (nivel != 0) {
            exit(0);
        }
    }

    // --- 4. IMPRESIÓN DEL SUPERPADRE Y LIMPIEZA ---
    if (nivel == 0) {
        // Al llegar aquí, el wait(NULL) del superpadre se ha desbloqueado, lo que garantiza 
        // que todos los hijos y subhijos han ejecutado su lógica y rellenado la memoria compartida.
        
        printf("Soy el superpadre (%d) : mis hijos finales son: ", getpid());
        for (int j = 0; j < y; j++) {
            printf("%d", hijos_finales[j]);
            if (j < y - 1) printf(", ");
        }
        printf("\n");

        // Desvincular el puntero y eliminar el segmento de memoria compartida
        shmdt((char *)memoria);
        shmctl(shmid, IPC_RMID, 0);
    }

    return 0;
}
