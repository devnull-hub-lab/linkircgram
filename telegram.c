#include <curl/curl.h>

#include "header.h"

void send_instagram_message(stparsing_conf *CONFIG, char *uid_nick, char *channel_uid, char *msg) {
    
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "libcurl init - error\n");
        return;
    }

    size_t len = strlen(uid_nick) + strlen(msg) + 3; //:\0
    char *fullmsg = malloc(len);
    snprintf(fullmsg, len, "%s: %s", uid_nick, msg);


    char *escaped_msg = curl_easy_escape(curl, fullmsg, 0); //scape bad chars

    char url[1024];
    snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/sendMessage?chat_id=%s&text=%s",
                                CONFIG->token, CONFIG->gid, escaped_msg);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    CURLcode res = curl_easy_perform(curl);

    if (strncmp(CONFIG->debug, "true", 4) == 0) {
        if (res != CURLE_OK)
            fprintf(stderr, "Error on sending telegram message: %s\n", curl_easy_strerror(res));
        //else
        //    printf("Telegram message sent\n");
    }

    curl_free(escaped_msg);
    free(fullmsg);
    curl_easy_cleanup(curl);
}
