#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, const char * argv[]){
  if(argc != 3){
    printf("necesita 3 argumentos!\n");
    return 1;
  }

  int x = atoi(argv[1]);
  int y = atoi(argv[2]);

  for(int j = 0; j < y; j++){
    pid_t pid = fork();

    if(pid == 0){
      for(int i = 0; i < x; i++){
        pid_t pid_hijo = fork();

        if(pid_hijo > 0){
          wait(NULL);
          exit(0);
        }
        else if(pid_hijo < 0){
          printf("Se produjo un error creando al hijo");
          exit(1);
        }
      }
      sleep(30);
      exit(0);
    }
    else if(pid < 0){
      printf("Se profujo un error creando al hijo");
      exit(1);
    }
  }
  for(int j = 0; j < y; j++){
    wait(NULL);
  }
  return 0;
}
