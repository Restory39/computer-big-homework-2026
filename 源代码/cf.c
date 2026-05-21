#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "cJSON.h"
#include <time.h>

#define BUF_SIZE (1024 * 1024)
char buf1[BUF_SIZE], buf2[BUF_SIZE], buf3[BUF_SIZE];

size_t cb1(void *data, size_t size, size_t nmemb, char *userp) {
    size_t len = size * nmemb;
    if (strlen(buf1) + len < BUF_SIZE - 1) {
        strncat(buf1, (char*)data, len);
    }
    return len;
}

size_t cb2(void *data, size_t size, size_t nmemb, char *userp) {
    size_t len = size * nmemb;
    if (strlen(buf2) + len < BUF_SIZE - 1) {
        strncat(buf2, (char*)data, len);
    }
    return len;
}

size_t cb3(void *data, size_t size, size_t nmemb, char *userp) {
    size_t len = size * nmemb;
    if (strlen(buf3) + len < BUF_SIZE - 1) {
        strncat(buf3, (char*)data, len);
    }
    return len;
}

char* safe_str(cJSON *obj, const char *key, char *def) {
    if (!obj) return def;
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (item && cJSON_IsString(item)) {
        return item->valuestring;
    }
    return def;
}

long long safe_ll(cJSON *obj, const char *key) {
    if (!obj) return 0;
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (item && cJSON_IsNumber(item)) {
        return (long long)item->valuedouble;
    }
    return 0;
}

int main() {
    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));
    memset(buf3, 0, sizeof(buf3));
    curl_global_init(CURL_GLOBAL_ALL);

    const char *handle = "tourist";

    CURL *c1 = curl_easy_init();
    char url1[256];
    sprintf(url1, "https://codeforces.com/api/user.info?handles=%s", handle);
    curl_easy_setopt(c1, CURLOPT_URL, url1);
    curl_easy_setopt(c1, CURLOPT_WRITEFUNCTION, cb1);
    curl_easy_setopt(c1, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(c1, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_perform(c1);
    curl_easy_cleanup(c1);

    cJSON *root1 = cJSON_Parse(buf1);
    cJSON *result1 = cJSON_GetObjectItem(root1, "result");
    cJSON *user = cJSON_GetArrayItem(result1, 0);

    char *rank = safe_str(user, "rank", "无段位");
    int rating = (int)safe_ll(user, "rating");
    char *maxRank = safe_str(user, "maxRank", "无段位");
    int maxRating = (int)safe_ll(user, "maxRating");
    char *avatar = safe_str(user, "titlePhoto", "");

    CURL *c2 = curl_easy_init();
    char url2[256];
    sprintf(url2, "https://codeforces.com/api/user.rating?handle=%s", handle);
    curl_easy_setopt(c2, CURLOPT_URL, url2);
    curl_easy_setopt(c2, CURLOPT_WRITEFUNCTION, cb2);
    curl_easy_setopt(c2, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(c2, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_perform(c2);
    curl_easy_cleanup(c2);

    cJSON *root2 = cJSON_Parse(buf2);
    cJSON *contests = cJSON_GetObjectItem(root2, "result");
    int contestCount = cJSON_GetArraySize(contests);

    int recent180Count = 0;
    time_t now = time(NULL);
    const long long day180Sec = 15552000LL;
    for (int i = 0; i < contestCount; i++) {
        cJSON *one = cJSON_GetArrayItem(contests, i);
        long long t = safe_ll(one, "ratingUpdateTimeSeconds");
        if ((now - t) <= day180Sec) {
            recent180Count++;
        }
    }

    cJSON *records = cJSON_CreateArray();
    for (int i = 0; i < contestCount; i++) {
        cJSON *one = cJSON_GetArrayItem(contests, i);
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", safe_str(one, "contestName", "未知比赛"));
        cJSON_AddNumberToObject(item, "time", safe_ll(one, "ratingUpdateTimeSeconds"));
        cJSON_AddNumberToObject(item, "oldRating", safe_ll(one, "oldRating"));
        cJSON_AddNumberToObject(item, "newRating", safe_ll(one, "newRating"));
        cJSON_AddNumberToObject(item, "rank", safe_ll(one, "rank"));
        cJSON_AddItemToArray(records, item);
    }

    CURL *c3 = curl_easy_init();
    char url3[256];
    sprintf(url3, "https://codeforces.com/api/user.status?handle=%s&from=1&count=1000", handle);
    curl_easy_setopt(c3, CURLOPT_URL, url3);
    curl_easy_setopt(c3, CURLOPT_WRITEFUNCTION, cb3);
    curl_easy_setopt(c3, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(c3, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_perform(c3);
    curl_easy_cleanup(c3);

    cJSON *root3 = cJSON_Parse(buf3);
    cJSON *submissions = cJSON_GetObjectItem(root3, "result");
    int subCount = cJSON_GetArraySize(submissions);

    cJSON *acProblems = cJSON_CreateArray();
    int acTotal = 0;
    for (int i = 0; i < subCount; i++) {
        cJSON *sub = cJSON_GetArrayItem(submissions, i);
        cJSON *verdict = cJSON_GetObjectItem(sub, "verdict");
        if (verdict && strcmp(verdict->valuestring, "OK") == 0) {
            acTotal++;
            cJSON *prob = cJSON_GetObjectItem(sub, "problem");
            cJSON *pItem = cJSON_CreateObject();
            cJSON_AddNumberToObject(pItem, "contestId", safe_ll(prob, "contestId"));
            cJSON_AddStringToObject(pItem, "index", safe_str(prob, "index", ""));
            cJSON_AddStringToObject(pItem, "name", safe_str(prob, "name", ""));
            cJSON_AddNumberToObject(pItem, "time", safe_ll(sub, "creationTimeSeconds"));
            cJSON_AddItemToArray(acProblems, pItem);
        }
    }

    cJSON *final = cJSON_CreateObject();
    cJSON_AddStringToObject(final, "handle", handle);
    cJSON_AddStringToObject(final, "currentRank", rank);
    cJSON_AddNumberToObject(final, "currentRating", rating);
    cJSON_AddStringToObject(final, "maxRank", maxRank);
    cJSON_AddNumberToObject(final, "maxRating", maxRating);
    cJSON_AddNumberToObject(final, "contestCount", contestCount);
    cJSON_AddStringToObject(final, "avatar", avatar);
    cJSON_AddNumberToObject(final, "recentContest", recent180Count);
    cJSON_AddItemToObject(final, "records", records);
    cJSON_AddNumberToObject(final, "acTotal", acTotal);
    cJSON_AddItemToObject(final, "acProblems", acProblems);

    FILE *fp = fopen("data.json", "w");
    char *jsonStr = cJSON_Print(final);
    fprintf(fp, "%s", jsonStr);
    fclose(fp);
    free(jsonStr);

    cJSON_Delete(final);
    cJSON_Delete(root1);
    cJSON_Delete(root2);
    cJSON_Delete(root3);
    curl_global_cleanup();

    return 0;
}