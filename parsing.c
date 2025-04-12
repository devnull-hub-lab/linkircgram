#include <stdio.h>
#include <stdlib.h>
#include <json-c/json.h>
#include <string.h>

#include "header.h"

int parsing_conf(stparsing_conf *CONFIG) {
    FILE *file = fopen(CONFFILE, "r");
    if (!file) {
        perror("Erro ao abrir link.json");
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char *data = malloc(length + 1);
    if (!data) {
        perror("Erro ao alocar memória");
        fclose(file);
        return 1;
    }

    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    struct json_object *parsed_json = json_tokener_parse(data);
    
    struct json_object *jsid, *jlinkname, *jhost, *jport, *jpassword, *jprotocol;

    free(data);

    if (!parsed_json) {
        fprintf(stderr, "Erro ao fazer parse do JSON.\n");
        return 1;
    }

    json_object_object_get_ex(parsed_json, "sid",      &jsid);
    json_object_object_get_ex(parsed_json, "linkname", &jlinkname);
    json_object_object_get_ex(parsed_json, "host",     &jhost);
    json_object_object_get_ex(parsed_json, "port",     &jport);
    json_object_object_get_ex(parsed_json, "password", &jpassword);
    json_object_object_get_ex(parsed_json, "protocol", &jprotocol);
    
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

    //To avoid buffer overflow in case link.json overflow lengh values
    CONFIG->sid     [sizeof(CONFIG->sid)      - 1] = '\0';
    CONFIG->linkname[sizeof(CONFIG->linkname) - 1] = '\0';
    CONFIG->host    [sizeof(CONFIG->host)     - 1] = '\0';
    CONFIG->port    [sizeof(CONFIG->port)     - 1] = '\0';
    CONFIG->password[sizeof(CONFIG->password) - 1] = '\0';
    CONFIG->protocol[sizeof(CONFIG->protocol) - 1] = '\0';

    json_object_put(parsed_json); //free parsed_json

    /*
    printf("SID: %s\n", CONFIG->sid);
    printf("Link: %s\n", CONFIG->linkname);
    printf("Host: %s\n", CONFIG->host);
    printf("Port: %s\n", CONFIG->port);
    printf("Pass: %s\n", CONFIG->password);
    printf("Proto: %s\n", CONFIG->protocol);
    */
    return 0;
}
