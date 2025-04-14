#include <json-c/json.h>

#include "header.h"

int parsing_conf(stparsing_conf *CONFIG) {
    FILE *file = fopen(CONFFILE, "r");
    if (!file) {
        perror("Error opening link.json");
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char *data = malloc(length + 1);
    if (!data) {
        perror("Error alocating memory - data");
        fclose(file);
        return 1;
    }

    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    struct json_object *parsed_json = json_tokener_parse(data);
    
    struct json_object *jsid, *jlinkname, *jhost, *jport, *jpassword, *jprotocol, *jchannel, *jdebug;
    struct json_object *jtoken, *jgid, *jtopic;

    free(data);

    if (!parsed_json) {
        fprintf(stderr, "Error JSON parsing\n");
        return 1;
    }

    json_object_object_get_ex(parsed_json, "irc-sid",      &jsid);
    json_object_object_get_ex(parsed_json, "irc-linkname", &jlinkname);
    json_object_object_get_ex(parsed_json, "irc-host",     &jhost);
    json_object_object_get_ex(parsed_json, "irc-port",     &jport);
    json_object_object_get_ex(parsed_json, "irc-password", &jpassword);
    json_object_object_get_ex(parsed_json, "irc-protocol", &jprotocol);
    json_object_object_get_ex(parsed_json, "irc-channel",  &jchannel);

    json_object_object_get_ex(parsed_json, "telegram-token", &jtoken);
    json_object_object_get_ex(parsed_json, "telegram-gid",   &jgid);
    json_object_object_get_ex(parsed_json, "telegram-topic", &jtopic);

    json_object_object_get_ex(parsed_json, "debug",    &jdebug);
    
    const char *isnotnull;

    isnotnull = json_object_get_string(jsid);
    if (isnotnull)
        strncpy(CONFIG->sid, json_object_get_string(jsid), sizeof(CONFIG->sid) - 1);
    
    isnotnull = json_object_get_string(jlinkname);
    if (isnotnull)
        strncpy(CONFIG->linkname, json_object_get_string(jlinkname), sizeof(CONFIG->linkname) - 1);
    
    isnotnull = json_object_get_string(jhost);
    if (isnotnull)
        strncpy(CONFIG->host, json_object_get_string(jhost), sizeof(CONFIG->host) - 1);
    
    isnotnull = json_object_get_string(jport);
    if (isnotnull)
        strncpy(CONFIG->port, json_object_get_string(jport), sizeof(CONFIG->port) - 1);
    
    isnotnull = json_object_get_string(jpassword);
    if (isnotnull)
        strncpy(CONFIG->password, json_object_get_string(jpassword), sizeof(CONFIG->password) - 1);
    
    isnotnull = json_object_get_string(jprotocol);
    if (isnotnull)
        strncpy(CONFIG->protocol, json_object_get_string(jprotocol), sizeof(CONFIG->protocol) - 1);

    isnotnull = json_object_get_string(jchannel);
    if (isnotnull)
        strncpy(CONFIG->channel, json_object_get_string(jchannel), sizeof(CONFIG->channel) - 1);

    isnotnull = json_object_get_string(jtoken);
    if (isnotnull)
        strncpy(CONFIG->token, json_object_get_string(jtoken), sizeof(CONFIG->token) - 1);

    isnotnull = json_object_get_string(jgid);
    if (isnotnull)
        strncpy(CONFIG->gid, json_object_get_string(jgid), sizeof(CONFIG->gid) - 1);

    isnotnull = json_object_get_string(jtopic);
    if (isnotnull)
        strncpy(CONFIG->topic, json_object_get_string(jtopic), sizeof(CONFIG->topic) - 1);

    isnotnull = json_object_get_string(jdebug);
    if (isnotnull)
        strncpy(CONFIG->debug, json_object_get_string(jdebug), sizeof(CONFIG->debug) - 1);

    //To avoid buffer overflow in case link.json overflow lengh values
    CONFIG->sid     [sizeof(CONFIG->sid)      - 1] = '\0';
    CONFIG->linkname[sizeof(CONFIG->linkname) - 1] = '\0';
    CONFIG->host    [sizeof(CONFIG->host)     - 1] = '\0';
    CONFIG->port    [sizeof(CONFIG->port)     - 1] = '\0';
    CONFIG->password[sizeof(CONFIG->password) - 1] = '\0';
    CONFIG->protocol[sizeof(CONFIG->protocol) - 1] = '\0';
    CONFIG->channel [sizeof(CONFIG->channel)  - 1] = '\0';
    CONFIG->token   [sizeof(CONFIG->token)    - 1] = '\0';
    CONFIG->gid     [sizeof(CONFIG->gid)      - 1] = '\0';
    CONFIG->topic   [sizeof(CONFIG->topic)    - 1] = '\0';
    CONFIG->debug   [sizeof(CONFIG->debug)    - 1] = '\0';

    json_object_put(parsed_json); //free parsed_json

    return 0;
}
