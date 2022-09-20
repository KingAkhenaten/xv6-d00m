#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "user/user.h"

#define LBSIZE 4096
#define FNSIZE 128

char linebuf[LBSIZE];
char filename[FNSIZE];


int
main(int argc, char *argv[])
{
  int fd;

  if (argc > 1){
    fd = open(argv[1], O_RDWR | O_CREATE);

    while(gets(linebuf, LBSIZE)){
      if (linebuf[0] != 'q' && strlen(linebuf) > 1){
        write(fd, linebuf, strlen(linebuf));
      }
      else if (linebuf[0] == 'q' && strlen(linebuf) == 2){
        printf("exiting..\n");
        close(fd);
        return 0;
      }
      /*fd[1] = open("tmp", O_RDRW | O_CREATE);
      write(fd[1], linebuf, strlen(linebuf));
      if (linebuf[0] == 'w' && strlen(linebuf) == 1){
        //write
        printf("Filename? ");
        gets(filename, LBSIZE);
        fd[0] = open(filename, O_RDRW | O_CREATE);
        while(strcpy
        write(fd[0], linebuf, strlen(linebuf));
        close(fd[0]);
      }
      if (linebuf[0] == 'q' && strlen(linebuf) == 1){
        //exit
        printf("Bye..\n");
        return 0;
      }*/
    }
    close(fd);
    return 0;
  }
  else{
    fprintf(2, "ERROR: Please specify an input file\n");
    return 1;
  }
}
