#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

pid_t pid_ejec;

// --- Manejadores de Señales ---

// Manejador vacío para capturar señales y salir del pause()
void manejador_vacio(int sig) {}

// Manejador en A para ejecutar pstree al recibir la señal de Z
void manejador_A(int sig) {
    if (fork() == 0) {
        char pid_str[15];
        sprintf(pid_str, "%d", pid_ejec);
        execlp("pstree", "pstree", "-c", pid_str, NULL);
        exit(1);
    }
    wait(NULL); // A espera a que el árbol se imprima por completo en pantalla
}

// --- Funciones de Comportamiento por Proceso ---

// Lógica para los procesos hoja idénticos (X e Y)
void proceso_hoja(char nombre, pid_t pid_padre, pid_t pid_abuelo) {
    printf("Soy el proceso %c: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n", 
           nombre, getpid(), pid_padre, pid_abuelo, pid_ejec);
    
    signal(SIGUSR1, manejador_vacio);
    pause(); // Espera permiso de B para morir
    
    printf("Soy %c (%d) y muero\n", nombre, getpid());
    exit(0);
}

// Lógica específica para el proceso Z
void proceso_Z(pid_t pid_padre, pid_t pid_abuelo, int secs) {
    printf("Soy el proceso Z: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n", 
           getpid(), pid_padre, pid_abuelo, pid_ejec);
    
    signal(SIGALRM, manejador_vacio);
    signal(SIGUSR1, manejador_vacio);
    
    alarm(secs);
    pause(); // Z espera a que suene la alarma planificada

    // Alarma cumplida: Z avisa a A enviando su señal
    kill(pid_abuelo, SIGUSR1);
    
    pause(); // Z espera permiso de B para morir
    
    printf("Soy Z (%d) y muero\n", getpid());
    exit(0);
}

// Lógica para el proceso B (Padre de X, Y, Z)
void proceso_B(pid_t pid_padre, int secs) {
    pid_t me_B = getpid();
    printf("Soy el proceso B: mi pid es %d. Mi padre es %d. Mi abuelo es %d\n", me_B, pid_padre, pid_ejec);

    // B se prepara para recibir la señal de A (para iniciar las muertes)
    signal(SIGUSR1, manejador_vacio);

    // Creación de la rama de hijos
    pid_t pid_X = fork();
    if (pid_X == 0) proceso_hoja('X', me_B, pid_padre);

    pid_t pid_Y = fork();
    if (pid_Y == 0) proceso_hoja('Y', me_B, pid_padre);

    pid_t pid_Z = fork();
    if (pid_Z == 0) proceso_Z(me_B, pid_padre, secs);

    // B espera en silencio hasta que A termine de imprimir el árbol y le avise
    pause(); 

    // Destrucción ordenada: despierta y espera a sus hijos secuencialmente
    kill(pid_X, SIGUSR1); waitpid(pid_X, NULL, 0);
    kill(pid_Y, SIGUSR1); waitpid(pid_Y, NULL, 0);
    kill(pid_Z, SIGUSR1); waitpid(pid_Z, NULL, 0);

    printf("Soy B (%d) y muero\n", getpid());
    exit(0);
}

// Lógica para el proceso A
void proceso_A(int secs) {
    pid_t me_A = getpid();
    printf("Soy el proceso A: mi pid es %d. Mi padre es %d\n", me_A, pid_ejec);

    // A se prepara para recibir la señal de Z
    signal(SIGUSR1, manejador_A);

    pid_t pid_B = fork();
    if (pid_B == 0) proceso_B(me_A, secs);

    // A queda a la espera de la señal de Z. Al recibirla, ejecuta pstree y vuelve aquí.
    pause(); 

    // El árbol ya está impreso. A avisa a B de que comience a limpiar a los hijos
    kill(pid_B, SIGUSR1);
    waitpid(pid_B, NULL, 0);
    
    printf("Soy A (%d) y muero\n", getpid());
    exit(0);
}

// --- Función Principal ---

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <segundos>\n", argv[0]);
        exit(1);
    }
    
    int secs = atoi(argv[1]);
    pid_ejec = getpid();
    printf("Soy el proceso ejec: mi pid es %d\n", pid_ejec);

    pid_t pid_A = fork();

    if (pid_A == 0) {
        proceso_A(secs); // Deriva toda la cascada del árbol
    }

    // Ejec espera a A
    waitpid(pid_A, NULL, 0);
    printf("Soy ejec (%d) y muero\n", getpid());

    return 0;
}
