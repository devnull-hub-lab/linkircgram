#include <json-c/json.h>

#include "header.h"

void send_instagram_message(stparsing_conf *CONFIG, char *uid_nick, char *channel_uid, char *msg) {
    char url[256];

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "libcurl init - error\n");
        return;
    }

    size_t len = strlen(uid_nick) + strlen(msg) + 3; //:\0
    char *fullmsg = malloc(len);
    snprintf(fullmsg, len, "%s: %s", uid_nick, msg);

    char *escaped_msg = curl_easy_escape(curl, fullmsg, 0); //escape bad chars
    if (!escaped_msg) {
        fprintf(stderr, "curl_easy_escape failed\n");
        free(fullmsg);
        curl_easy_cleanup(curl);
        return;
    }
    
    for (int index = 0; index < CONFIG->mapping_count; index++) {
        if (strncmp(CONFIG->mappings[index].irc_channel, channel_uid, 32) == 0) {
            snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/sendMessage?chat_id=%s&message_thread_id=%s&text=%s",
                     CONFIG->token, CONFIG->mappings[index].telegram_gid, CONFIG->mappings[index].telegram_topic, escaped_msg);
            break;
        }
    }
    


    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LinkBot/1.0");

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "Error on sending telegram message: %s\n", curl_easy_strerror(res));
    
    curl_free(escaped_msg);
    free(fullmsg);
    curl_easy_cleanup(curl);
}
//---------------------------------------------------------------------------------------
int fetch_telegram_updates_raw(stparsing_conf *CONFIG, struct string *response) {
    CURL *curl;
    CURLcode ret;
    char url[256];

    if (CONFIG == NULL || response == NULL)
        return -1;

    snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/getUpdates", CONFIG->token);
    
    //TODO:Implement last_update_id
    //if (*last_update_id > 0)
    //    snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/getUpdates?offset=%lld", CONFIG->token, (*last_update_id + 1));
    //else
    //snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/getUpdates", CONFIG->token);

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "libcurl init - error\n");
        return -1;
    }

    //init string
    response->len = 0;
    response->data = malloc(1);  //1 byte = \0
    response->data[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writecurl);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    ret = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return (ret == CURLE_OK) ? 0 : -1;
}
//---------------------------------------------------------------------------------------
short parse_telegram_updates(const char *json_str, telegram_message **out_msgs, long long *last_update_id)
{    
    struct json_object *parsed_json, *result_array;
    parsed_json = json_tokener_parse(json_str);

    if (!parsed_json)
        return -1;

    if (!json_object_object_get_ex(parsed_json, "result", &result_array)) {
        json_object_put(parsed_json);
        return -1;
    }

    short total_elements = json_object_array_length(result_array);
    if (total_elements == 0) {
        json_object_put(parsed_json);
        *out_msgs = NULL;
        return 0;
    }

    telegram_message *msg_array = calloc(total_elements, sizeof(telegram_message));
    if (!msg_array) {
        json_object_put(parsed_json);
        return -1;
    }

    short total_msgs = 0;
    for (int i = 0; i < total_elements; i++) {
        struct json_object *update = json_object_array_get_idx(result_array, i);
        struct json_object *update_id, *message, *msg_text, *chat, *chatid;
        struct json_object *from, *userid, *fname, *lname, *threadid;

        if (!json_object_object_get_ex(update, "update_id", &update_id))
            continue;

        if (!json_object_object_get_ex(update, "message", &message))
            json_object_object_get_ex(update, "edited_message", &message);

        if (message &&  json_object_object_get_ex(message, "text",       &msg_text) &&
                        json_object_object_get_ex(message, "chat",       &chat)     &&
                        json_object_object_get_ex(chat,    "id",         &chatid)   &&
                        json_object_object_get_ex(message, "from",       &from)     &&
                        json_object_object_get_ex(from,    "id",         &userid)   &&
                        json_object_object_get_ex(from,    "first_name", &fname))    {

            telegram_message *msg = &msg_array[total_msgs];
            msg->update_id  = json_object_get_int64(update_id);
            msg->chat_id    = json_object_get_int64(chatid);
            msg->user_id    = json_object_get_int64(userid);
            msg->first_name = strdup(json_object_get_string(fname));
            msg->text       = strdup(json_object_get_string(msg_text));

            if (json_object_object_get_ex(from, "last_name", &lname))
                msg->last_name = strdup(json_object_get_string(lname));
            else
                msg->last_name = strdup("");

            if (json_object_object_get_ex(message, "message_thread_id", &threadid))
                msg->thread_id = json_object_get_int64(threadid);
            else
                msg->thread_id = -1;

            //if (last_update_id)
            //    *last_update_id = msg->update_id;

            total_msgs++;
        }
    }

    json_object_put(parsed_json);

    *out_msgs = msg_array;
    
    return total_msgs;
}
//---------------------------------------------------------------------------------------
size_t writecurl(void *data, size_t size, size_t qt_items, struct string *s) {
    size_t new_len = s->len + size * qt_items;
    s->data = realloc(s->data, new_len + 1);
    if (s->data == NULL) {
        fprintf(stderr, "Error on realocating memory - write\n");
        exit(1);
    }
    memcpy(s->data + s->len, data, size * qt_items);
    s->data[new_len] = '\0';
    s->len = new_len;
    return size * qt_items;
}

