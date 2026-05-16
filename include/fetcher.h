#pragma once
#include <string>

struct FetchResponse {
    long status_code;
    std::string content_type;
    std::string body;
};

FetchResponse fetch_url(const std::string& url);
