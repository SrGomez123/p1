#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    // Validamos el número de argumentos
    if (argc != 3) {
        char msg[] = "Uso: ./hacha <archivo> <tamaño>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(1);
    }

    char *archivo_origen = argv[1];
    long tam_trozo = atol(argv[2]);

    // Obtenemos el tamaño total del archivo para saber cuántos hijos necesitamos crear
    struct stat st;
    if (stat(archivo_origen, &st) == -1) {
        perror("Error al obtener información del archivo");
        exit(1);
    }

    // Calculamos el número de fragmentos (y por ende, de hijos)
    int num_hijos = st.st_size / tam_trozo;
    if (st.st_size % tam_trozo != 0) {
        num_hijos++;
    }

    // Abrimos el archivo original para lectura
    int fd_in = open(archivo_origen, O_RDONLY);
    if (fd_in < 0) {
        perror("Error al abrir el archivo de origen");
        exit(1);
    }

    // Bucle para crear los hijos y enviarles la información
    for (int i = 0; i < num_hijos; i++) {
        int p[2];
        
        // Creamos la tubería antes del fork
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
            //PROCESO HIJO
            close(p[1]); // El hijo no va a escribir en la tubería, cierra ese extremo

            char nombre_out[256];
            // Construimos el nombre del archivo con formato .h00, .h01...
            // sprintf se usa aquí solo para formatear cadenas en memoria, no hace E/S a disco
            sprintf(nombre_out, "%s.h%02d", archivo_origen, i);

            // El hijo crea el archivo de destino y lo abre para escritura
            int fd_out = creat(nombre_out, 0666);
            if (fd_out < 0) {
                perror("Error al crear el archivo de destino");
                exit(1);
            }

            char buffer_hijo[4096];
            int leidos_hijo;

            // El hijo lee de la tubería hasta que el padre cierra su extremo de escritura (devuelve 0)
            while ((leidos_hijo = read(p[0], buffer_hijo, sizeof(buffer_hijo))) > 0) {
                // Escribe en su archivo de destino lo recibido por la tubería
                write(fd_out, buffer_hijo, leidos_hijo);
            }

            close(fd_out);
            close(p[0]); // Cierra la tubería de lectura
            
            exit(0); // El hijo termina su trabajo
        } else {
            //PROCESO PADRE
            close(p[0]); // El padre no va a leer de la tubería, cierra ese extremo

            char buffer_padre[4096];
            long bytes_enviados = 0;
            int leidos_padre;

            // El padre lee del archivo original solo el tamaño correspondiente a un trozo
            while (bytes_enviados < tam_trozo) {
                long a_leer = tam_trozo - bytes_enviados;
                if (a_leer > sizeof(buffer_padre)) {
                    a_leer = sizeof(buffer_padre);
                }

                // Lee del archivo usando llamadas al sistema
                leidos_padre = read(fd_in, buffer_padre, a_leer);
                if (leidos_padre <= 0) {
                    break; // Fin del archivo
                }

                // Envía lo leído al hijo a través de la tubería
                write(p[1], buffer_padre, leidos_padre);
                bytes_enviados += leidos_padre;
            }

            // CERRAR LA TUBERÍA es fundamental para que el hijo reciba el EOF en su bucle de lectura
            close(p[1]); 

            // Decisión de implementación: Esperamos a que termine este hijo antes de crear el siguiente (lanzamiento secuencial)
            wait(NULL);
        }
    }

    close(fd_in);
    return 0;
}
