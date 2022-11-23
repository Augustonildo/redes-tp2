#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

int main(int argc, char *argv[])
{
  struct sockaddr_storage storage;
  addrparse(argv[1], argv[2], &storage);

  int s = socket(storage.ss_family, SOCK_STREAM, 0);
  if (s == -1)
  {
    logexit("socket");
  }

  struct sockaddr *addr = (struct sockaddr *)(&storage);
  if (0 != connect(s, addr, sizeof(storage)))
  {
    logexit("connect");
  }

  char addrstr[BUFSZ];
  addrtostr(addr, addrstr, BUFSZ);

  char buf[BUFSZ];
  memset(buf, 0, BUFSZ);
  size_t count = 0;
  while (1)
  {
    fgets(buf, BUFSZ - 1, stdin);
    count = send(s, buf, strlen(buf) + 1, 0);
    if (count != strlen(buf) + 1)
    {
      logexit("send");
    }

    memset(buf, 0, BUFSZ);
    count = recv(s, buf, BUFSZ, 0);
    if (count == 0 || strcmp(buf, "\0") == 0)
    {
      break; // Connection terminated
    }
    printf("%s", buf);
    count = 0;
  }

  close(s);
  exit(EXIT_SUCCESS);
}