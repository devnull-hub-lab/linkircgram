#include "header.h"

int main() {
    int sock, len;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    stuid_structure *UID = NULL;
    int count_uid_list = 0; //Should be < MAX_UID_LIST
    char uid[UIDLEN];

    stparsing_conf CONFIG;
    parsing_conf(&CONFIG);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket-error");
        return 1;
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(atoi(CONFIG.port));
    server_addr.sin_addr.s_addr = inet_addr(CONFIG.host);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect-error");
        return 1;
    }

    snprintf(buffer, sizeof(buffer), "PASS :%s\r\n", CONFIG.password);
    send(sock, buffer, strlen(buffer), 0);

    snprintf(buffer, sizeof(buffer), "PROTOCTL EAUTH=%s SID=%s\r\n", CONFIG.linkname, CONFIG.sid);
    send(sock, buffer, strlen(buffer), 0);

    snprintf(buffer, sizeof(buffer), "PROTOCTL SJOIN SJ3 CLK NOQUIT NICKv2 VL UMODE2 PROTOCTL NICKIP VHP ESVID EXTSWHOIS TKLEXT2 NEXTBANS\r\n");
    send(sock, buffer, strlen(buffer), 0);

    snprintf(buffer, sizeof(buffer), "SERVER %s 1 U6-linkircgram-%s :LinkIRCGram\r\n", CONFIG.linkname, CONFIG.sid);
    send(sock, buffer, strlen(buffer), 0);

    sleep(2); //Wait server buffer response

    stuid_structure *tempalloc_UID = realloc(UID, count_uid_list + 1 * sizeof(stuid_structure));
    if (tempalloc_UID != NULL)
        UID = tempalloc_UID;

    generate_uid(CONFIG.sid, uid);
    
    //TODO: Just fake user to test.
    //Data will be filled after fetch each telegram user id
    UID[count_uid_list].timestamp = (long)time(NULL);
    UID[count_uid_list].nick      = strdup("nick1");
    UID[count_uid_list].user      = strdup("user1");
    UID[count_uid_list].host      = strdup("services.host");
    UID[count_uid_list].uid       = strdup(uid);

    len = strlen(UID[count_uid_list].nick);
    char vhost_buff[len + 15];
    snprintf(vhost_buff, sizeof(vhost_buff), "%s.over.telegram", UID[count_uid_list].nick);
    UID[count_uid_list].vhost = strdup(vhost_buff);

    //UID ...
    spawn_user(sock, buffer, sizeof(buffer), UID, count_uid_list);

    sleep(1);

    time_t timestamp = time(NULL);
    len = snprintf(buffer, sizeof(buffer), "SJOIN %ld %s +nt :%s\r\n", (long)timestamp, CONFIG.channel, UID[count_uid_list].uid);
    send(sock, buffer, len, 0);

    count_uid_list++;

    while (1) {
        len = recv(sock, buffer, sizeof(buffer) -1, 0);
        buffer[len] = '\0';
        if (len <= 0) {
            printf("[!] Connection Lost\n");
            break;
        }

        if (strncmp(CONFIG.debug, "true", 4) == 0)
            printf("[Server] %s\n", buffer);

        if (strncmp(buffer, "PING", 4) == 0) {
            char pong_reply[256];
            char *ping_data = strchr(buffer, ':');
            if (ping_data) {
                snprintf(pong_reply, sizeof(pong_reply), "PONG %s\r\n", ping_data);
                send(sock, pong_reply, strlen(pong_reply), 0);
                //printf("[+] PONG sent: %s", pong_reply);
            }
        }
        
        //TODO:.....
    
    } //while_conn
    close(sock);

    for (unsigned int count = 0; count < count_uid_list; count++) {
        free(UID[count].nick);
        free(UID[count].user);
        free(UID[count].host);
        free(UID[count].vhost);
        free(UID[count].uid);
    }
    free(UID);

    return 0;
}
//---------------------------------------------------------------------------------------
// Introducing new user on the server
//---------------------------------------------------------------------------------------

int spawn_user(int sock, char *buffer, size_t bufferlen, stuid_structure *UID, int index) {
    
    time_t timestamp = time(NULL);

    //UnrealIRCD
    //nickname - hop - timestamp - username - hostname - uid - servicetimestamp - umodes - vhost - cloakedhost - ip - gecos
    int len = snprintf(buffer, bufferlen, "UID %s 1 %ld %s %s %s 0 +iwx %s Clk-BA0161F4.host * :Nickname1 Registration Service\r\n",
                                 UID[index].nick, (long)timestamp, UID[index].user, UID[index].host, UID[index].uid, UID[index].vhost);

    send(sock, buffer, len, 0);

    len = snprintf(buffer, bufferlen, "MD client %s creationtime :%ld\r\n", UID[index].uid, (long)timestamp);
    send(sock, buffer, len, 0);

    return 0;
}
//---------------------------------------------------------------------------------------
// Generate uid (SID _ _ _ _ _ _)
//---------------------------------------------------------------------------------------
void generate_uid(char *sid, char *uid) {
    
    const char hex_chars[] = "0123456789ABCDEF";

    strncpy(uid, sid, 3);
    
    for (short count = 3; count < UIDLEN - 1; count++) {
        short mod = rand() % 16;
        uid[count] = hex_chars[mod];
    }
    uid[UIDLEN - 1] = '\0';
}
