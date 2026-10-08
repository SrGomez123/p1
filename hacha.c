#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

// --- Funciones Auxiliares ---

// Módulo para calcular la cantidad de archivos resultantes (hijos a crear)
int calcular_num_hijos(const char *archivo, long tam_trozo) {
    struct stat st;
    if (stat(archivo, &st) == -1) {
        perror("Error al obtener información del archivo");
        exit(1);
    }

    int num_hijos = st.st_size / tam_trozo;
    if (st.st_size % tam_trozo != 0) {
        num_hijos++;
    }
    
    return num_hijos;
}

// --- Funciones de Comportamiento de Procesos ---

// Módulo con la lógica del proceso hijo (Recibir datos y escribir a archivo)
void proceso_hijo(int fd_pipe_lectura, const char *archivo_origen, int indice) {
    char nombre_out[256];
    // Construimos el nombre del archivo con formato .h00, .h01...
    sprintf(nombre_out, "%s.h%02d", archivo_origen, indice);

    // El hijo crea el archivo de destino y lo abre para escritura
    int fd_out = creat(nombre_out, 0666);
    if (fd_out < 0) {
        perror("Error al crear el archivo de destino");
        exit(1);
    }

    char buffer_hijo[4096];
    int leidos_hijo;

    // Lee de la tubería hasta que el padre cierra su extremo (devuelve 0)
    while ((leidos_hijo = read(fd_pipe_lectura, buffer_hijo, sizeof(buffer_hijo))) > 0) {
        write(fd_out, buffer_hijo, leidos_hijo);
    }

    close(fd_out);
    close(fd_pipe_lectura);
    
    exit(0); // El hijo termina su trabajo
}

// Módulo con la lógica del proceso padre (Leer del archivo original y enviar por tubería)
void enviar_trozo_por_tuberia(int fd_in, int fd_pipe_escritura, long tam_trozo) {
    char buffer_padre[4096];
    long bytes_enviados = 0;
    int leidos_padre;

    // Lee del archivo original solo el tamaño correspondiente a un trozo
    while (bytes_enviados < tam_trozo) {
        long a_leer = tam_trozo - bytes_enviados;
        if (a_leer > sizeof(buffer_padre)) {
            a_leer = sizeof(buffer_padre);
        }

        leidos_padre = read(fd_in, buffer_padre, a_leer);
        if (leidos_padre <= 0) {
            break; // Fin del archivo original
        }

        // Envía lo leído al hijo a través de la tubería
        write(fd_pipe_escritura, buffer_padre, leidos_padre);
        bytes_enviados += leidos_padre;
    }

    // Cerrar la tubería es fundamental para que el hijo detecte el final
    close(fd_pipe_escritura);
}

// --- Función Principal de Orquestación ---

// Módulo que orquesta la creación de tuberías y bifurcaciones
void dividir_archivo(const char *archivo_origen, long tam_trozo, int num_hijos) {
    int fd_in = open(archivo_origen, O_RDONLY);
    if (fd_in < 0) {
        perror("Error al abrir el archivo de origen");
        exit(1);
    }

    // Bucle para crear los hijos y comunicarles la información
    for (int i = 0; i < num_hijos; i++) {
        int p[2];
        
        if (pipe(p) < 0) {
            perror("Error al crear la tubería");
            exit(1);
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("Error en fork");
            exit(1);
        }

        if (pid == 0) {
            // DENTRO DEL PROCESO HIJO
            close(p[1]); // Cierra extremo de escritura
            proceso_hijo(p[0], archivo_origen, i);
        } else {
            // DENTRO DEL PROCESO PADRE
            close(p[0]); // Cierra extremo de lectura
            enviar_trozo_por_tuberia(fd_in, p[1], tam_trozo);
            
            // Lanzamiento secuencial: Esperamos a que termine este hijo antes de crear el siguiente
            wait(NULL);
        }
    }

    close(fd_in);
}

// --- MAIN ---

int main(int argc, char *argv[]) {
    // Validamos el número de argumentos usando la llamada al sistema write
    if (argc != 3) {
        char msg[] = "Uso: ./hacha <archivo> <tamaño>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(1);
    }

    char *archivo_origen = argv[1];
    long tam_trozo = atol(argv[2]);

    // 1. Calculamos los fragmentos necesarios
    int num_hijos = calcular_num_hijos(archivo_origen, tam_trozo);

    // 2. Ejecutamos la división mediante tuberías
    dividir_archivo(archivo_origen, tam_trozo, num_hijos);

    return 0;
}
