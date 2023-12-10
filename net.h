#ifndef NET_H
#define NET_H

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

extern int tcp_listen(char *host, char *service, int qsize);

#endif
