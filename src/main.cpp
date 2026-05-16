#include <aws/lambda-runtime/runtime.h>
#include "fetcher.h"
#include "html_simplifier.h"
#include "image_processor.h"
#include <string>
#include <cstring>

using namespace aws::lambda_runtime;

// Minimal JSON string extraction (avoids AWS SDK dependency)
static std::string json_get_string(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

static std::string make_response(int status, const std::string& body, const std::string& content_type, bool base64) {
    // Build API Gateway proxy response JSON
    std::string resp = "{\"statusCode\":" + std::to_string(status) +
        ",\"headers\":{\"Content-Type\":\"" + content_type + "\"}," +
        "\"isBase64Encoded\":" + (base64 ? "true" : "false") +
        ",\"body\":\"";
    // Escape body for JSON
    for (char c : body) {
        switch (c) {
            case '"': resp += "\\\""; break;
            case '\\': resp += "\\\\"; break;
            case '\n': resp += "\\n"; break;
            case '\r': resp += "\\r"; break;
            case '\t': resp += "\\t"; break;
            default: resp += c;
        }
    }
    resp += "\"}";
    return resp;
}

static invocation_response handler(invocation_request const& request) {
    std::string path = json_get_string(request.payload, "rawPath");
    if (path.empty()) path = json_get_string(request.payload, "path");

    const std::string prefix = "/proxy/";
    std::string target_url;
    auto ppos = path.find(prefix);
    if (ppos != std::string::npos) {
        target_url = path.substr(ppos + prefix.size());
    }

    if (target_url.empty()) {
        return invocation_response::success(
            make_response(400, "<html><body><h1>Bad Request</h1><p>Use /proxy/URL</p></body></html>", "text/html", false),
            "application/json");
    }

    auto response = fetch_url(target_url);

    std::string body;
    std::string content_type = response.content_type;

    if (content_type.find("text/html") != std::string::npos) {
        body = simplify_html(response.body);
        content_type = "text/html";
    } else if (content_type.find("image/") != std::string::npos) {
        body = process_image(response.body, content_type);
        content_type = "image/gif";
    } else {
        body = response.body;
    }

    bool is_image = content_type.find("image/") != std::string::npos;
    return invocation_response::success(
        make_response(static_cast<int>(response.status_code), body, content_type, is_image),
        "application/json");
}

int main() {
    run_handler(handler);
    return 0;
}
