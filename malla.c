#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Lógica para el último proceso de cada columna
void ultimo_proceso() {
    sleep(3);
    exit(0);
}

// Lógica para crear la profundidad (filas) de una columna concreta
void generar_cadena_vertical(int x) {
    for (int i = 1; i < x; i++) {
        pid_t pid_fila = fork();
        
        if (pid_fila < 0) {
            perror("Error en fork vertical");
            exit(1);
        }
        
        if (pid_fila > 0) {
            // PROCESO PADRE: Espera a que el descendiente muera y luego muere él
            wait(NULL);
            exit(0);
        }
    }
    // El último proceso en nacer sale del bucle y se convierte en la espera
    ultimo_proceso();
}

// Lógica para la expansión horizontal de la malla
void generar_columnas(int x, int y) {
    for (int j = 0; j < y; j++) {
        pid_t pid_columna = fork();
        
        if (pid_columna < 0) {
            perror("Error en fork horizontal");
            exit(1);
        }
        
        if (pid_columna == 0) {
            // Abandona la horizontal e inicia su propia cadena vertical
            generar_cadena_vertical(x);
        }
    }
}

// Lógica para la impresión del árbol
void ejecutar_pstree(pid_t pid_raiz) {
    sleep(1); 
    
    printf("\nÁrbol de procesos resultante:\n");
    if (fork() == 0) {
        char pid_str[20];
        sprintf(pid_str, "%d", pid_raiz);
        
        // Ejecutamos pstree aislando el PID de la raíz
        execlp("pstree", "pstree", "-c", pid_str, NULL);
        perror("Error al ejecutar pstree");
        exit(1);
    }
    
    // Espera a que pstree termine de dibujarse en consola
    wait(NULL); 
}

// Lógica de limpieza
void recolectar_columnas(int y) {
    for (int j = 0; j < y; j++) {
        wait(NULL);
    }
    printf("\nDestrucción del árbol completada con éxito.\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <filas_x> <columnas_y>\n", argv[0]);
        exit(1);
    }

    int x = atoi(argv[1]);
    int y = atoi(argv[2]);

    if (x < 1 || y < 1) {
        printf("Las filas (x) y columnas (y) deben ser mayores que 0.\n");
        exit(1);
    }

    pid_t pid_raiz = getpid();
    printf("Proceso raíz (malla) iniciado con PID: %d\n", pid_raiz);

    // 1.Creación de la malla
    generar_columnas(x, y);
    
    // 2.Arbol
    ejecutar_pstree(pid_raiz);
    
    // 3.Limpieza procesos
    destruccion_columnas(y);

    return 0;
}
