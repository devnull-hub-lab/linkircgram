#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <curl/curl.h>

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
    char **channels;
    int channel_count;
} stchannel_list;

typedef struct {
	char *nick;
    long timestamp;
    char *user;
    char *host;
    char *vhost;
    char *uid;
    stchannel_list chanlist;
} stuid_structure;

typedef struct {
    char telegram_gid[32];
    char irc_channel[CHANLEN];
    char telegram_topic[32];
} stchannel_mapping;

typedef struct {
    char sid[SIDLEN];
    char linkname[25]; //TODO: check length later
    char host[HOSTLEN];
    char port[PORTLEN];
    char password[50]; //TODO: check length later
    char protocol[15];
    char token[50];
    char debug[6];

    stchannel_mapping *mappings;
    int mapping_count;
} stparsing_conf;

struct string {
    char *data;
    size_t len;
};

typedef struct {
    long long update_id;
    long long user_id;
    long long chat_id;
    long long thread_id;
    char *first_name;
    char *last_name;
    char *text;
} telegram_message;

/* IRC */
int  spawn_user (int sock, char *buffer, size_t bufferlen, stuid_structure *UID, int count_uid_list);
void generate_uid (char *, char *uid);
int  parsing_conf (stparsing_conf *CONFIG);
void search_user_on_irc(stuid_structure *UID, int count_uid_list, int *located_uid, long long user_id);
int  add_channel_to_user(stuid_structure *user, const char *channel);
int  remove_user_from_channel(stuid_structure *user, const char *channel);
int  user_already_in_channel(const stuid_structure *user, const char *channel);
void sanitize_text(char *text);
void remove_rn(char *text);
void join_linkserv_channels();

/* Telegram */
void   send_telegram_message(stparsing_conf *CONFIG, char *uid_nick, char *channel_uid, char *msg);
short  parse_privmsg_buf(char *buffer, char **uid_nick, char **channel_uid, char **msg);
void   parse_updates(const char *json_str);
int    fetch_telegram_updates_raw(stparsing_conf *CONFIG, struct string *response, long long *offset);
short  parse_telegram_updates(const char *json_str, telegram_message **out_msgs);
size_t writecurl(void *data, size_t size, size_t qt_items, struct string *s);

#endif

