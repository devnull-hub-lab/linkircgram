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

    json_object_object_get_ex(parsed_json, "irc-sid",        &jsid);
    json_object_object_get_ex(parsed_json, "irc-linkname",   &jlinkname);
    json_object_object_get_ex(parsed_json, "irc-host",       &jhost);
    json_object_object_get_ex(parsed_json, "irc-port",       &jport);
    json_object_object_get_ex(parsed_json, "irc-password",   &jpassword);
    json_object_object_get_ex(parsed_json, "irc-protocol",   &jprotocol);
    json_object_object_get_ex(parsed_json, "telegram-token", &jtoken);
    json_object_object_get_ex(parsed_json, "debug",          &jdebug);

    const char *isnotnull;

    isnotnull = json_object_get_string(jsid);
    if (isnotnull) strncpy(CONFIG->sid, json_object_get_string(jsid), sizeof(CONFIG->sid) - 1);
    
    isnotnull = json_object_get_string(jlinkname);
    if (isnotnull) strncpy(CONFIG->linkname, json_object_get_string(jlinkname), sizeof(CONFIG->linkname) - 1);
    
    isnotnull = json_object_get_string(jhost);
    if (isnotnull) strncpy(CONFIG->host, json_object_get_string(jhost), sizeof(CONFIG->host) - 1);
    
    isnotnull = json_object_get_string(jport);
    if (isnotnull) strncpy(CONFIG->port, json_object_get_string(jport), sizeof(CONFIG->port) - 1);
    
    isnotnull = json_object_get_string(jpassword);
    if (isnotnull) strncpy(CONFIG->password, json_object_get_string(jpassword), sizeof(CONFIG->password) - 1);
    
    isnotnull = json_object_get_string(jprotocol);
    if (isnotnull) strncpy(CONFIG->protocol, json_object_get_string(jprotocol), sizeof(CONFIG->protocol) - 1);

    isnotnull = json_object_get_string(jtoken);
    if (isnotnull) strncpy(CONFIG->token, json_object_get_string(jtoken), sizeof(CONFIG->token) - 1);

    isnotnull = json_object_get_string(jdebug);
    if (isnotnull) strncpy(CONFIG->debug, json_object_get_string(jdebug), sizeof(CONFIG->debug) - 1);

    //To avoid buffer overflow in case link.json overflow lengh values
    CONFIG->sid     [sizeof(CONFIG->sid)      - 1] = '\0';
    CONFIG->linkname[sizeof(CONFIG->linkname) - 1] = '\0';
    CONFIG->host    [sizeof(CONFIG->host)     - 1] = '\0';
    CONFIG->port    [sizeof(CONFIG->port)     - 1] = '\0';
    CONFIG->password[sizeof(CONFIG->password) - 1] = '\0';
    CONFIG->protocol[sizeof(CONFIG->protocol) - 1] = '\0';
    CONFIG->token   [sizeof(CONFIG->token)    - 1] = '\0';
    CONFIG->debug   [sizeof(CONFIG->debug)    - 1] = '\0';

    struct json_object *jmap_array;
    if (json_object_object_get_ex(parsed_json, "channel-mappings", &jmap_array) &&
        json_object_is_type (jmap_array, json_type_array))
    {
        int array_len = json_object_array_length(jmap_array);
        
        CONFIG->mappings = malloc(array_len * sizeof(stchannel_mapping));
        if (!CONFIG->mappings) {
            perror("Error allocating memory for mappings");
            json_object_put(parsed_json);
            return 1;
        }

        CONFIG->mapping_count = array_len;

        for (int index = 0; index < array_len; ++index) {
            struct json_object *item = json_object_array_get_idx(jmap_array, index);

            struct json_object *jgid, *jchannel, *jtopic;
            json_object_object_get_ex(item, "telegram-gid",   &jgid);
            json_object_object_get_ex(item, "irc-channel",    &jchannel);
            json_object_object_get_ex(item, "telegram-topic", &jtopic);

            if (jgid && jchannel && jtopic) {
                strncpy(CONFIG->mappings[index].telegram_gid,   json_object_get_string(jgid),     sizeof(CONFIG->mappings[index].telegram_gid)   - 1);
                strncpy(CONFIG->mappings[index].irc_channel,    json_object_get_string(jchannel), sizeof(CONFIG->mappings[index].irc_channel)    - 1);
                strncpy(CONFIG->mappings[index].telegram_topic, json_object_get_string(jtopic),   sizeof(CONFIG->mappings[index].telegram_topic) - 1);

                CONFIG->mappings[index].telegram_gid  [sizeof(CONFIG->mappings[index].telegram_gid)   - 1] = '\0';
                CONFIG->mappings[index].irc_channel   [sizeof(CONFIG->mappings[index].irc_channel)    - 1] = '\0';
                CONFIG->mappings[index].telegram_topic[sizeof(CONFIG->mappings[index].telegram_topic) - 1] = '\0';
            }
        }
    }
    else
    {
        CONFIG->mappings = NULL;
        CONFIG->mapping_count = 0;
    }

    json_object_put(parsed_json); //free parsed_json

    return 0;
}

