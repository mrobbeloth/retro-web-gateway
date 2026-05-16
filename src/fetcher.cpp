#include "fetcher.h"
#include <curl/curl.h>

static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* body = static_cast<std::string*>(userdata);
    body->append(ptr, size * nmemb);
    return size * nmemb;
}

FetchResponse fetch_url(const std::string& url) {
    FetchResponse result{};
    CURL* curl = curl_easy_init();
    if (!curl) {
        result.status_code = 500;
        result.body = "Failed to init curl";
        return result;
    }

    std::string header_data;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/4.0 (compatible; RetroProxy/1.0)");

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status_code);
        char* ct = nullptr;
        curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
        if (ct) result.content_type = ct;
    } else {
        result.status_code = 502;
        result.body = "Fetch failed: " + std::string(curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    return result;
}
