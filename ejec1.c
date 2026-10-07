#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

pid_t pid_ejec;

// Manejador vacío para capturar señales y salir del pause()
void manejador_vacio(int sig) {}

// Manejador en A para ejecutar pstree al recibir la señal de Z
void manejador_A(int sig) {
    // A crea un hijo temporal para ejecutar pstree aislando el PID de la raíz
    if (fork() == 0) {
        char pid_str[15];
        sprintf(pid_str, "%d", pid_ejec);
        execlp("pstree", "pstree", "-c", pid_str, NULL);
        exit(1);
    }
    wait(NULL); // A espera a que el árbol se imprima por completo en pantalla
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <segundos>\n", argv[0]);
        exit(1);
    }
    
    int secs = atoi(argv[1]);
    pid_ejec = getpid();
    printf("Soy el proceso ejec: mi pid es %d\n", pid_ejec);

    pid_t pid_A = fork();

    if (pid_A == 0) { //PROCESO A
        pid_t me_A = getpid();
        printf("Soy el proceso A: mi pid es %d. Mi padre es %d\n", me_A, pid_ejec);

        // A se prepara para recibir la señal de Z
        signal(SIGUSR1, manejador_A);

        pid_t pid_B = fork();

        if (pid_B == 0) { //PROCESO B
            pid_t me_B = getpid();
            printf("Soy el proceso B: mi pid es %d. Mi padre es %d. Mi abuelo es %d\n", me_B, me_A, pid_ejec);

            // B se prepara para recibir la señal de A (para iniciar las muertes)
            signal(SIGUSR1, manejador_vacio);

            // Crea X
            pid_t pid_X = fork();
            if (pid_X == 0) {
                printf("Soy el proceso X: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n", getpid(), me_B, me_A, pid_ejec);
                signal(SIGUSR1, manejador_vacio);
                pause();
                printf("Soy X (%d) y muero\n", getpid());
                exit(0);
            }

            // Crea Y
            pid_t pid_Y = fork();
            if (pid_Y == 0) {
                printf("Soy el proceso Y: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n", getpid(), me_B, me_A, pid_ejec);
                signal(SIGUSR1, manejador_vacio);
                pause();
                printf("Soy Y (%d) y muero\n", getpid());
                exit(0);
            }

            // Crea Z
            pid_t pid_Z = fork();
            if (pid_Z == 0) {
                printf("Soy el proceso Z: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n", getpid(), me_B, me_A, pid_ejec);
                
                signal(SIGALRM, manejador_vacio);
                signal(SIGUSR1, manejador_vacio);
                
                alarm(secs);
                pause(); // Z espera a que suene la alarma planificada

                // Alarma cumplida: Z avisa a A enviando su señal
                kill(me_A, SIGUSR1);
                
                // Z espera permiso de B para morir
                pause(); 
                
                printf("Soy Z (%d) y muero\n", getpid());
                exit(0);
            }

            // B espera en silencio hasta que A termine de imprimir el árbol y le avise
            pause(); 

            // Destrucción ordenada. Despierta y espera a sus hijos secuencialmente
            kill(pid_X, SIGUSR1);
            waitpid(pid_X, NULL, 0);

            kill(pid_Y, SIGUSR1);
            waitpid(pid_Y, NULL, 0);

            kill(pid_Z, SIGUSR1);
            waitpid(pid_Z, NULL, 0);

            // B debe imprimir y morir. ¡No lo quites para no perder puntos en la práctica!
            printf("Soy B (%d) y muero\n", getpid());
            exit(0);
        }

        // A queda a la espera de la señal de Z. 
        // Al recibirla, salta a manejador_A, ejecuta pstree y vuelve aquí.
        pause(); 

        // El árbol ya está impreso. A avisa a B de que comience a limpiar a los hijos
        kill(pid_B, SIGUSR1);
        
        // A espera a que B muera
        waitpid(pid_B, NULL, 0);
        
        printf("Soy A (%d) y muero\n", getpid());
        exit(0);
    }

    // PROCESO RAIZ (ejec)
    // Ejec espera a A
    waitpid(pid_A, NULL, 0);
    printf("Soy ejec (%d) y muero\n", getpid());

    return 0;
}
