#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

int main(int argc, char ** argv) {
  int fd;
  char buf[256];
  // Open the Port. We want read/write, no "controlling tty" status, and open it no matter what state DCD is in
  fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1) {
    perror("open_port: Unable to open /dev/ttyS0 - ");
    return(-1);
  }

  // Turn off blocking for reads, use (fd, F_SETFL, FNDELAY) if you want that
  fcntl(fd, F_SETFL, 0);

  // Write to the port
  int n = write(fd,"SEND\r\n", 6);
  if (n < 0) {
    perror("Write failed - ");
    return -1;
  }

  n = read(fd, (void*)buf, 255);
  if (n < 0) {
    perror("Read failed - ");
    return -1;
  }
  else if (n == 0)
    printf("No data on port\n");
  else {
    printf("%i bytes read : %s", n, buf);
    
  }
  if(!strcmp(buf,"OK\n")){
      n = write(fd,"COMMAND 1\r\n", 12);
      if (n < 0) {
        perror("Write failed - ");
        return -1;
      }
    }
  // Don't forget to clean up
  close(fd);
  return 0;
}
