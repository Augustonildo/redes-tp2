#include "common.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct equipment
{
  int id;
  int installed;
  int socket;
} equipment;

int equipmentCount = 0;
equipment equipments[MAX_EQUIPMENT_NUMBER];

void initializeEquipments()
{
  for (int i = 0; i < MAX_EQUIPMENT_NUMBER; i++)
  {
    equipments[i].installed = 0;
    equipments[i].id = i + 1;
    equipments[i].socket = 0;
  }
}

typedef struct response
{
  char message[BUFSZ];
  int endConnection;
} response;

response exitHandler(char *message)
{
  response response;
  memset(response.message, 0, BUFSZ);
  response.endConnection = 1;
  return response;
}

response resolveHandler(char *message)
{
  response response;
  strcpy(response.message, message);
  response.endConnection = 0;
  return response;
}

response addNewEquipment(int clientSocket)
{
  int newId;
  for (int i = 0; i < MAX_EQUIPMENT_NUMBER; i++)
  {
    if (!equipments[i].installed)
    {
      newId = equipments[i].id;
      equipments[i].installed = 1;
      equipments[i].socket = clientSocket;
      equipmentCount++;
      break;
    }
  }

  printf("Equipment %02d added\n", newId);

  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  sprintf(msg, "%02d %02d\n", RES_ADD, newId);
  for (int i = 0; i < MAX_EQUIPMENT_NUMBER; i++)
  {
    if (equipments[i].installed)
    {
      sendMessage(equipments[i].socket, msg);
    }
  }

  usleep(1000); // Sleep between messages

  memset(msg, 0, BUFSZ);
  sprintf(msg, "%02d", RES_LIST);
  for (int i = 0; i < MAX_EQUIPMENT_NUMBER; i++)
  {
    if (equipments[i].installed)
    {
      char installedId[BUFSZ];
      sprintf(installedId, " %02d", equipments[i].id);
      strcat(msg, installedId);
    }
  }
  strcat(msg, "\n");
  sendMessage(equipments[newId - 1].socket, msg);

  return resolveHandler(msg);
}

response removeEquipment(int id)
{
  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  if (!equipments[id - 1].installed)
  {
    sprintf(msg, "%02d %02d", ERROR, EQUIPMENT_NOT_FOUND);
    sendMessage(equipments[id - 1].socket, msg);
    return exitHandler("Error");
  }

  equipments[id - 1].installed = 0;
  equipmentCount--;

  sprintf(msg, "%02d %02d %02d", OK, id, 1);
  sendMessage(equipments[id - 1].socket, msg);
  printf("Equipment %02d removed\n", id);

  for (int i = 0; i < MAX_EQUIPMENT_NUMBER; i++)
  {
    if (equipments[i].installed)
    {
      sprintf(msg, "%02d %02d", REQ_RM, id);
      sendMessage(equipments[i].socket, msg);
    }
  }

  return exitHandler("Success");
}

int isValidId(int id)
{
  if (id < 0)
    return 0;
  if (id > MAX_EQUIPMENT_NUMBER)
    return 0;
  return 1;
}

response requestInformation(int sourceId, int destineId)
{
  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  if (!isValidId(sourceId) || !equipments[sourceId - 1].installed)
  {
    printf("Equipment %02d not found\n", sourceId);
    sprintf(msg, "%02d %02d", ERROR, SOURCE_NOT_FOUND);
    sendMessage(equipments[sourceId - 1].socket, msg);
    return resolveHandler("Not found");
  }

  if (!isValidId(destineId) || !equipments[destineId - 1].installed)
  {
    printf("Equipment %02d not found\n", destineId);
    sprintf(msg, "%02d %02d", ERROR, TARGET_NOT_FOUND);
    sendMessage(equipments[sourceId - 1].socket, msg);
    return resolveHandler("Not found");
  }

  sprintf(msg, "%02d %02d %02d", REQ_INF, sourceId, destineId);
  sendMessage(equipments[destineId - 1].socket, msg);
  return resolveHandler("Success");
}

response informationResult(int sourceId, int destineId, float temperature)
{
  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  if (!isValidId(sourceId) || !equipments[sourceId - 1].installed)
  {
    printf("Equipment %02d not found\n", sourceId);
    sprintf(msg, "%02d %02d", ERROR, SOURCE_NOT_FOUND);
    sendMessage(equipments[sourceId - 1].socket, msg);
    return resolveHandler("Not found");
  }

  if (!isValidId(destineId) || !equipments[destineId - 1].installed)
  {
    printf("Equipment %02d not found\n", destineId);
    sprintf(msg, "%02d %02d", ERROR, TARGET_NOT_FOUND);
    sendMessage(equipments[sourceId - 1].socket, msg);
    return resolveHandler("Not found");
  }

  sprintf(msg, "%02d %02d %02d %.2f", RES_INF, sourceId, destineId, temperature);
  sendMessage(equipments[destineId - 1].socket, msg);
  return resolveHandler("Success");
}

response handleCommands(char *buf, int clientSocket)
{
  char *splittedCommand = strtok(buf, " ");
  int commandId = atoi(splittedCommand);
  int sourceId = 0, destineId = 0;

  switch (commandId)
  {
  case REQ_ADD:
    return addNewEquipment(clientSocket);
  case REQ_RM:
    splittedCommand = strtok(NULL, " ");
    return removeEquipment(atoi(splittedCommand));
  case REQ_INF:
    splittedCommand = strtok(NULL, " ");
    sourceId = atoi(splittedCommand);
    splittedCommand = strtok(NULL, " ");
    destineId = atoi(splittedCommand);
    return requestInformation(sourceId, destineId);
  case RES_INF:
    splittedCommand = strtok(NULL, " ");
    sourceId = atoi(splittedCommand);
    splittedCommand = strtok(NULL, " ");
    destineId = atoi(splittedCommand);
    splittedCommand = strtok(NULL, " ");
    float temperature = atof(splittedCommand);
    return informationResult(sourceId, destineId, temperature);
  default:
    break;
  }

  printf("Unknown message: %s\n", buf);
  return exitHandler("ERROR");
}

struct client_data
{
  int csock;
  struct sockaddr_storage storage;
};

void *client_thread(void *data)
{
  struct client_data *cdata = (struct client_data *)data;
  struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

  char caddrstr[BUFSZ];
  addrtostr(caddr, caddrstr, BUFSZ);

  if (equipmentCount >= MAX_EQUIPMENT_NUMBER)
  {
    char mensagemServidorCheio[BUFSZ];
    memset(mensagemServidorCheio, 0, BUFSZ);
    sprintf(mensagemServidorCheio, "%02d %02d", ERROR, LIMIT_EXCEEDED);
    sendMessage(cdata->csock, mensagemServidorCheio);
    close(cdata->csock);
    pthread_exit(EXIT_SUCCESS);
  }

  response response;
  memset(response.message, 0, BUFSZ);

  char buf[BUFSZ];
  while (1)
  {
    memset(buf, 0, BUFSZ);
    memset(response.message, 0, BUFSZ);

    recv(cdata->csock, buf, BUFSZ - 1, 0);
    buf[strcspn(buf, "\n")] = 0;

    response = handleCommands(buf, cdata->csock);
    if (response.endConnection)
    {
      close(cdata->csock);
      break;
    }
  }
  pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
  struct sockaddr_storage storage;
  server_sockaddr_init(argv[1], &storage);

  int s = socket(storage.ss_family, SOCK_STREAM, 0);
  if (s == -1)
  {
    logexit("socket");
  }

  struct sockaddr *addr = (struct sockaddr *)(&storage);
  if (0 != bind(s, addr, sizeof(storage)))
  {
    logexit("bind");
  }

  if (0 != listen(s, 10))
  {
    logexit("listen");
  }

  char addrstr[BUFSZ];
  addrtostr(addr, addrstr, BUFSZ);

  initializeEquipments();
  while (1)
  {
    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);

    int csock = accept(s, caddr, &caddrlen);
    if (csock == -1)
    {
      logexit("accept");
    }

    struct client_data *cdata = malloc(sizeof(*cdata));
    if (!cdata)
    {
      logexit("malloc");
    }
    cdata->csock = csock;
    memcpy(&(cdata->storage), &cstorage, sizeof(storage));

    pthread_t tid;
    pthread_create(&tid, NULL, client_thread, cdata);
  }
  exit(EXIT_SUCCESS);
}