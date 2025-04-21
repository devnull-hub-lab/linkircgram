#include "header.h"

#include <sys/select.h> //TODO: Change later - Use kqueue for FreeBSD

int main()
{
    char buffer[BUFFER_SIZE];
    int sock, len;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    
    stuid_structure *UID = NULL;
    int count_uid_list = 0; //TODO: Should be < MAX_UID_LIST
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

    //TODO: Test compatibility with others ircds, at least: inspircd, ergo, solanum
    if (memcmp(CONFIG.protocol, "unrealircd", 10) == 0) {
        snprintf(buffer, sizeof(buffer), "PASS :%s\r\n", CONFIG.password);
        send(sock, buffer, strlen(buffer), 0);

        snprintf(buffer, sizeof(buffer), "PROTOCTL EAUTH=%s SID=%s\r\n", CONFIG.linkname, CONFIG.sid);
        send(sock, buffer, strlen(buffer), 0);

        snprintf(buffer, sizeof(buffer), "PROTOCTL SJOIN SJ3 CLK NOQUIT NICKv2 VL UMODE2 PROTOCTL NICKIP VHP ESVID EXTSWHOIS TKLEXT2 NEXTBANS\r\n");
        send(sock, buffer, strlen(buffer), 0);

        snprintf(buffer, sizeof(buffer), "SERVER %s 1 U6-linkircgram-%s :LinkIRCGram\r\n", CONFIG.linkname, CONFIG.sid);
        send(sock, buffer, strlen(buffer), 0);
    }
    
    sleep(1); /* Wait server buffer reply */
    
    curl_global_init(CURL_GLOBAL_DEFAULT);

    stuid_structure *tempalloc_UID = NULL;

    fd_set read_fds;
    struct timeval timeout;

    long long last_update_id = 0;

    while (1) {
        //TODO: Change later - Use kqueue for FreeBSD
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
    
        int ready = select(sock + 1, &read_fds, NULL, NULL, &timeout);
        memset(buffer, 0, sizeof(buffer));
        if (ready > 0 && FD_ISSET(sock, &read_fds)) {
            len = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (len <= 0) {
                printf("[!] Connection Lost! \n");
                break;
            }
            buffer[len] = '\0';
            if (strncmp(CONFIG.debug, "true", 4) == 0)
                printf("Received ON: %s\n", buffer);
        }

        if (strncmp(CONFIG.debug, "true", 4) == 0)
                printf("Received OFF: %s\n", buffer);
        
        if (strncmp(buffer, "PING", 4) == 0) {
            char pong_reply[256];
            char *ping_data = strchr(buffer, ':');
            if (ping_data) {
                snprintf(pong_reply, sizeof(pong_reply), "PONG %s\r\n", ping_data);
                send(sock, pong_reply, strlen(pong_reply), 0);
                if (strncmp(CONFIG.debug, "true", 4) == 0)
                    printf("[+] PONG sent: %s", pong_reply);
            }
        }

        /* Intercept telegram messages, to spawn and/or relay messages */
        struct string response;
        telegram_message *msg_array = NULL;

        if (fetch_telegram_updates_raw(&CONFIG, &response, &last_update_id) == 0)
        {
            short total_msgs = parse_telegram_updates(response.data, &msg_array);
            free(response.data);

            for (short index_json = 0; index_json < total_msgs; index_json++) 
            {
                if (strncmp(CONFIG.debug, "true", 4) == 0)
                    printf("Received %lld message ID from GID %lld thread %lld: %s\n", msg_array[index_json].update_id, msg_array[index_json].chat_id, msg_array[index_json].thread_id, msg_array[index_json].text);

                last_update_id = msg_array[index_json].update_id;
                
                int located_uid = -1;
            
                if (count_uid_list != 0)
                    search_user_on_irc(UID, count_uid_list, &located_uid, msg_array[index_json].user_id);
                
                if (located_uid >= 0) {
                    for (int index = 0; index < CONFIG.mapping_count; index++) {
                        if (strtoll(CONFIG.mappings[index].telegram_gid,   NULL, 10) == msg_array[index_json].chat_id &&
                            strtoll(CONFIG.mappings[index].telegram_topic, NULL, 10) == msg_array[index_json].thread_id) {
                                if (!user_already_in_channel(&UID[located_uid -1], CONFIG.mappings[index].irc_channel)) {
                                    time_t timestamp = time(NULL);
                                    len = snprintf(buffer, sizeof(buffer), "SJOIN %ld %s +nt :%s\r\n", (long)timestamp, CONFIG.mappings[index].irc_channel, UID[located_uid -1].uid);
                                    send(sock, buffer, len, 0);
                                    
                                    add_channel_to_user(&UID[located_uid -1], CONFIG.mappings[index].irc_channel);
                                    sleep(1); /* wait server response before sending messages */
                                }
                                remove_rn(msg_array[index_json].text);
                                snprintf(buffer, sizeof(buffer), "@relay=y :%s PRIVMSG %s :%s\r\n", UID[located_uid -1].nick, CONFIG.mappings[index].irc_channel, msg_array[index_json].text);
                                send(sock, buffer, strlen(buffer), 0);

                                if (strncmp(CONFIG.debug, "true", 4) == 0)
                                    printf("Sending %lld message ID from GID %lld thread %lld: %s\n", msg_array[index_json].update_id, msg_array[index_json].chat_id, msg_array[index_json].thread_id, msg_array[index_json].text);

                                break;
                        }
                    }
                }
                else /* Introduce new user on the server */
                {
                    tempalloc_UID = realloc(UID, (count_uid_list + 1) * sizeof(stuid_structure));
                    if (tempalloc_UID != NULL) {
                        UID = tempalloc_UID;

                        UID[count_uid_list].chanlist.channel_count = 0;
                        UID[count_uid_list].chanlist.channels = NULL;
                    }
            
                    generate_uid(CONFIG.sid, uid);
                
                    char full_nick[NICKLEN];
                    snprintf(full_nick, sizeof(full_nick), "%s%s",  msg_array[index_json].first_name,  msg_array[index_json].last_name);
                    sanitize_text(full_nick);
                    if (isdigit(full_nick[0])) /* first UID digit must be numeric, so nick can't */
                      full_nick[0] = '_';
                    
                    UID[count_uid_list].timestamp = (long)time(NULL);
                    UID[count_uid_list].nick      = strdup(full_nick);
                    UID[count_uid_list].host      = strdup("services.link");
                    UID[count_uid_list].uid       = strdup(uid);
    
                    char user_id_str[32];
                    snprintf(user_id_str, sizeof(user_id_str), "%lld",  msg_array[index_json].user_id);
                    UID[count_uid_list].user = strdup(user_id_str);
            
                    len = strlen(UID[count_uid_list].nick);
                    char vhost_buff[len + 15];
                    snprintf(vhost_buff, sizeof(vhost_buff), "%s.over.telegram", UID[count_uid_list].nick);
                    UID[count_uid_list].vhost = strdup(vhost_buff);
    
                    /* Introducing UID */
                    spawn_user(sock, buffer, sizeof(buffer), UID, count_uid_list);
            
                    sleep(1); /* wait server response after UID */

                    for (int index = 0; index < CONFIG.mapping_count; index++) {
                        if (strtoll(CONFIG.mappings[index].telegram_gid,   NULL, 10) == msg_array[index_json].chat_id &&
                            strtoll(CONFIG.mappings[index].telegram_topic, NULL, 10) == msg_array[index_json].thread_id)
                        {
                            if (!user_already_in_channel(&UID[count_uid_list], CONFIG.mappings[index].irc_channel)) {
                                time_t timestamp = time(NULL);
                                len = snprintf(buffer, sizeof(buffer), "SJOIN %ld %s +nt :%s\r\n", (long)timestamp, CONFIG.mappings[index].irc_channel, UID[count_uid_list].uid);
                                send(sock, buffer, len, 0);
                                
                                add_channel_to_user(&UID[count_uid_list], CONFIG.mappings[index].irc_channel);
                                sleep(1); /* wait server response before sending messages */
                            }
                            break;
                        }
                    }

                    for (int index = 0; index < CONFIG.mapping_count; index++) {
                        if (strtoll(CONFIG.mappings[index].telegram_gid,   NULL, 10) == msg_array[index_json].chat_id &&
                            strtoll(CONFIG.mappings[index].telegram_topic, NULL, 10) == msg_array[index_json].thread_id) {
                                /* mtag relay=y, so this message won't be intercepted by this link */
                                remove_rn( msg_array[index_json].text);
                                snprintf(buffer, sizeof(buffer), "@relay=y :%s PRIVMSG %s :%s\r\n", UID[count_uid_list].nick, CONFIG.mappings[index].irc_channel,  msg_array[index_json].text);
                                send(sock, buffer, strlen(buffer), 0);

                                if (strncmp(CONFIG.debug, "true", 4) == 0)
                                    printf("Sending %lld message ID from GID %lld thread %lld: %s\n", msg_array[index_json].update_id, msg_array[index_json].chat_id, msg_array[index_json].thread_id, msg_array[index_json].text);

                                break;
                        }
                    }

                    count_uid_list++;
                }
                free(msg_array[index_json].first_name);
                free(msg_array[index_json].last_name);
                free(msg_array[index_json].text);
            }
            if (msg_array)
                free(msg_array);
        } //fetch_telegram_updates_raw() end

        /* Intercept IRC messages, unless mtag relay=y is set */
        //TODO: Check if other mtag is set
        if (strstr(buffer, "PRIVMSG") != NULL && strstr(buffer, "@") == NULL) {
            char *privmsg_data = strchr(buffer, ':');
            if (privmsg_data) {
                    char *uid_nick, *channel_uid, *msg;

                    short ret = parse_privmsg_buf(buffer, &uid_nick, &channel_uid, &msg);

                    if (strncmp(CONFIG.debug, "true", 4) == 0) {
                        printf("uid-nick: %s\n", uid_nick);
                        printf("channel: %s\n", channel_uid);
                        printf("msg: %s\n", msg);
                    }
                    
                    if (!ret)
                        send_instagram_message(&CONFIG, uid_nick, channel_uid, msg);
                    
                    if (uid_nick)    free(uid_nick);
                    if (channel_uid) free(channel_uid);
                    if (msg)         free(msg);
            }
        }
    } //while_conn

    curl_global_cleanup();

    close(sock);

    for (unsigned int count_uid = 0; count_uid < count_uid_list; count_uid++)
    {
        for (int index = 0; index < UID[count_uid].chanlist.channel_count; index++)
        {
            free(UID[count_uid].chanlist.channels[index]);
            UID[count_uid].chanlist.channels[index] = NULL;
        }
        
        if (UID[count_uid].chanlist.channel_count > 0)
            if (UID[count_uid].chanlist.channels != NULL)
                free(UID[count_uid].chanlist.channels);

        free(UID[count_uid].nick);
        free(UID[count_uid].user);
        free(UID[count_uid].host);
        free(UID[count_uid].vhost);
        free(UID[count_uid].uid);
    }
    if (UID){
        free(UID);
        UID = NULL;
    }

    if (CONFIG.mappings != NULL) {
        free(CONFIG.mappings);
        CONFIG.mappings = NULL;
    }

    return 0;
}
//---------------------------------------------------------------------------------------
/*
 * Introducing new user on the server (without channel join)
 * S2S protocol commands used: UID, MD
 */
int spawn_user(int sock, char *buffer, size_t bufferlen, stuid_structure *UID, int index)
{
    time_t timestamp = time(NULL);

    /* UnrealIRCD
    nickname - hop - timestamp - username - hostname - uid - servicetimestamp - umodes - vhost - cloakedhost - ip - gecos */
    int len = snprintf(buffer, bufferlen,
                       "UID %s 1 %ld %s %s %s 0 +iwx %s Clk-%s.link * :%s on Telegram\r\n",
                       UID[index].nick, (long)timestamp, UID[index].user, UID[index].host, UID[index].uid, UID[index].vhost, UID[index].uid, UID[index].nick);

    send(sock, buffer, len, 0);

    len = snprintf(buffer, bufferlen, "MD client %s creationtime :%ld\r\n", UID[index].uid, (long)timestamp);
    send(sock, buffer, len, 0);

    return 0;
}
/*
 * Generate uid (SID _ _ _ _ _ _)
 * The first 3 digits = SID
 * Last 6 digits are random hex chars
 */
//TODO: Check if exists in the hash table
void generate_uid(char *sid, char *uid)
{
    const char hex_chars[] = "0123456789ABCDEF";

    strncpy(uid, sid, 3);
    
    for (short count = 3; count < UIDLEN - 1; count++) {
        short mod = rand() % 16;
        uid[count] = hex_chars[mod];
    }
    uid[UIDLEN - 1] = '\0';
}
/*
 * Parse privmsg buffer to vars
 */
short parse_privmsg_buf(char *buffer, char **uid_nick, char **channel_uid, char **msg)
{
    /* UID or NICK
    Somehow, privmmsg to channel uses nick, but privmsg to someone uses UID */
    if (buffer[0] == '@') { //TODO: I should check other mtag
        char *space_after_tag = strchr(buffer, ' ');
        if (!space_after_tag)
            return 1;
        
        buffer = space_after_tag + 1;
    }

    char *space = strchr(buffer, ' ');
    if (!space || buffer[0] != ':')
        return 1;

    size_t len = space - buffer - 1;
    *uid_nick = strndup(buffer + 1, len);

    space += 1;
    if (strncmp(space, "PRIVMSG ", 8) != 0)
        return 1;

    char *secondparam = space + 8;
    space = strchr(secondparam, ' ');
    if (!space)
        return 1;

    len = space - secondparam;
    *channel_uid = strndup(secondparam, len);

    char *colon = strchr(space, ':');
    if (!colon)
        return 1;

    *msg = strdup(colon + 1);

    return 0;
}
/*
 * Search if a user wass spawned on the Server
 */
// TODO: Use Hash table
void search_user_on_irc(stuid_structure *UID, int count_uid_list, int *located_uid, long long user_id)
{    
    char username[32];
    snprintf(username, sizeof(username), "%lld", user_id);

    for (int index = 0; index < count_uid_list; index++) {
        if (strcmp(UID[index].user, username) == 0)
            *located_uid = index + 1;
    }
}
/*
 * Add the channels where this user will be
 * This step is necessary, since the user introduced in the server can send messages
 * to multiple channels, if so defined in the .json.
 */
int add_channel_to_user(stuid_structure *user, const char *channel) 
{
    if (user_already_in_channel(user, channel)) {
        return 0;
    }
    
    char **new_channels = realloc(user->chanlist.channels, (user->chanlist.channel_count + 1) * sizeof(char *));
    if (!new_channels)
        return -1;

    user->chanlist.channels = new_channels;
    user->chanlist.channels[user->chanlist.channel_count] = strdup(channel);
    user->chanlist.channel_count++;

    return 1;
}
/*
 * Remove user from channel he is in.
 * Required when a user leaves telegram or the time spent on the channel expires
 */
int remove_user_from_channel(stuid_structure *user, const char *channel)
{    
    int found = 0;

    for (int index = 0; index < user->chanlist.channel_count; index++) {
        if (strcmp(user->chanlist.channels[index], channel) == 0) {
            free(user->chanlist.channels[index]); // Libera a string
            found = 1;

            // Move os elementos seguintes para preencher o buraco
            for (int count = index; count < user->chanlist.channel_count - 1; count++) {
                user->chanlist.channels[count] = user->chanlist.channels[count + 1];
            }

            user->chanlist.channel_count--;

            // Realloc para ajustar o tamanho (opcional)
            if (user->chanlist.channel_count > 0) {
                char **new_channels = realloc(user->chanlist.channels, 
                    sizeof(char *) * user->chanlist.channel_count);
                if (new_channels) {
                    user->chanlist.channels = new_channels;
                }
            } else {
                free(user->chanlist.channels);
                user->chanlist.channels = NULL;
            }

            break;
        }
    }
    return found;
}
/*
 * Check if a user is inside a channel.
 * The user can only send messages to the channels he is in.
 */
int user_already_in_channel(const stuid_structure *user, const char *channel)
{
    for (int index = 0; index < user->chanlist.channel_count; index++) {
        if (strcmp(user->chanlist.channels[index], channel) == 0) {
            return 1;
        }
    }
    return 0;
}
/*
 * Sanitize Nickname. Nicks must contain 0-9, a-z, A-z
 * Unknown chars will be replaced by _
 */
void sanitize_text(char *text)
{
    for (char *c = text; *c; ++c) {
        if (!( (*c >= '0' && *c <= '9') ||
               (*c >= 'A' && *c <= 'Z') ||
               (*c >= 'a' && *c <= 'z') )) {
            *c = '_';
        }
    }
}
/*
 * To avoid message cut after received \r\n from telegram
 */
void remove_rn(char *text)
{
    for (char *c = text; *c != '\0'; ++c) {
        if (*c == '\r' || *c == '\n')
            *c = '_';
    }
}
