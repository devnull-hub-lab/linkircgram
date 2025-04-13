#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CONFFILE "link.json"

#define BUFFER_SIZE 2048
#define	HOSTLEN	63 + 1
#define CHANLEN 32 + 1
#define	NICKLEN 30 + 1
#define	USERLEN 10 + 1
#define UIDLEN   9 + 1
#define PORTLEN  5 + 1
#define SIDLEN   3 + 1

#define MAX_UID_LIST 6777216 //16^6 = SID + 6 hex chars

typedef struct {
	char *nick;
    long timestamp;
    char *user;
    char *host;
    char *vhost;
    char *uid;
    //servicestamp
    //umodes
    //virthost
    //cloakedhost
    //ip
    //gecos
} stuid_structure;

typedef struct {
    char sid[SIDLEN];
    char linkname[50]; //TODO: check length later
    char host[HOSTLEN];
    char port[PORTLEN];
    char password[50]; //TODO: check length later
    char protocol[15];
    char channel[CHANLEN];
    char bot[10]; //TODO: check length later
    char gid[25]; //TODO: check length later
    char topic[10]; //TODO: check length later
    char debug[6];
} stparsing_conf;

int  spawn_user   (int sock, char *buffer, size_t bufferlen, stuid_structure *UID, int count_uid_list);
void generate_uid (char *, char *uid);
int  parsing_conf (stparsing_conf *CONFIG);

#endif

