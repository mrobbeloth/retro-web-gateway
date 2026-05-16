#include <aws/lambda-runtime/runtime.h>
#include "fetcher.h"
#include "html_simplifier.h"
#include "image_processor.h"
#include <string>
#include <cstring>

using namespace aws::lambda_runtime;

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string url_decode(const std::string& src) {
    std::string out;
    for (size_t i = 0; i < src.size(); i++) {
        if (src[i] == '%' && i + 2 < src.size()) {
            int hi = src[i+1];
            int lo = src[i+2];
            auto hex = [](int c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                return -1;
            };
            int h = hex(hi), l = hex(lo);
            if (h >= 0 && l >= 0) {
                out += static_cast<char>(h * 16 + l);
                i += 2;
                continue;
            }
        } else if (src[i] == '+') {
            out += ' ';
            continue;
        }
        out += src[i];
    }
    return out;
}

static std::string base64_encode(const std::string& input) {
    std::string out;
    out.reserve(((input.size() + 2) / 3) * 4);
    const unsigned char* p = reinterpret_cast<const unsigned char*>(input.data());
    size_t len = input.size();
    for (size_t i = 0; i < len; i += 3) {
        unsigned int n = static_cast<unsigned int>(p[i]) << 16;
        if (i + 1 < len) n |= static_cast<unsigned int>(p[i + 1]) << 8;
        if (i + 2 < len) n |= static_cast<unsigned int>(p[i + 2]);
        out += B64[(n >> 18) & 0x3F];
        out += B64[(n >> 12) & 0x3F];
        out += (i + 1 < len) ? B64[(n >> 6) & 0x3F] : '=';
        out += (i + 2 < len) ? B64[n & 0x3F] : '=';
    }
    return out;
}

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

static std::string make_response(int status, const std::string& body, const std::string& content_type, bool is_binary) {
    std::string encoded_body;
    if (is_binary) {
        encoded_body = base64_encode(body);
    } else {
        // JSON-escape the body
        for (char c : body) {
            switch (c) {
                case '"': encoded_body += "\\\""; break;
                case '\\': encoded_body += "\\\\"; break;
                case '\n': encoded_body += "\\n"; break;
                case '\r': encoded_body += "\\r"; break;
                case '\t': encoded_body += "\\t"; break;
                default: encoded_body += c;
            }
        }
    }
    return "{\"statusCode\":" + std::to_string(status) +
        ",\"headers\":{\"Content-Type\":\"" + content_type + "\"" +
        ",\"Content-Disposition\":\"inline\"" +
        "}," +
        "\"isBase64Encoded\":" + (is_binary ? "true" : "false") +
        ",\"body\":\"" + encoded_body + "\"}";
}

static invocation_response handler(invocation_request const& request) {
    std::string path = json_get_string(request.payload, "rawPath");
    if (path.empty()) path = json_get_string(request.payload, "path");

    std::string target_url;

    // Mode 1: /proxy/http://example.com
    const std::string prefix = "/proxy/";
    auto ppos = path.find(prefix);
    if (ppos != std::string::npos) {
        target_url = path.substr(ppos + prefix.size());
        // Reconstruct query string if present (API Gateway strips it from path)
        std::string qs = json_get_string(request.payload, "rawQueryString");
        if (!qs.empty() && qs.find("url=") != 0 && qs.find("q=") != 0) {
            target_url += "?" + qs;
        }
        // Handle DuckDuckGo redirect URLs: extract actual destination from uddg param
        if (target_url.find("duckduckgo.com/l/") != std::string::npos) {
            auto uddg_pos = target_url.find("uddg=");
            if (uddg_pos != std::string::npos) {
                std::string encoded = target_url.substr(uddg_pos + 5);
                auto amp = encoded.find('&');
                if (amp != std::string::npos) encoded = encoded.substr(0, amp);
                target_url = url_decode(encoded);
            }
        }
    }

    // Mode 2: /proxy/?url=http://example.com
    if (target_url.empty()) {
        std::string qs = json_get_string(request.payload, "rawQueryString");
        const std::string qprefix = "url=";
        if (qs.find(qprefix) == 0) {
            target_url = url_decode(qs.substr(qprefix.size()));
        }
    }

    // Mode 3: Search query — route to DuckDuckGo HTML lite
    if (target_url.empty()) {
        std::string qs = json_get_string(request.payload, "rawQueryString");
        const std::string sprefix = "q=";
        if (qs.find(sprefix) == 0) {
            // Pass query as-is (already URL-encoded) to DuckDuckGo
            target_url = "https://html.duckduckgo.com/html/?" + qs;
        }
    }

    if (target_url.empty()) {
        std::string portal =
            "<html><head><title>Retro Web Gateway</title></head><body>"
            "<h1>Retro Web Gateway</h1>"
            "<form action=\"/proxy/\" method=\"get\">"
            "<p><b>Search the Web:</b></p>"
            "<input type=\"text\" name=\"q\" size=\"40\">"
            "<input type=\"submit\" value=\"Search\">"
            "</form>"
            "<hr>"
            "<form action=\"/proxy/\" method=\"get\">"
            "<p><b>Go to URL:</b></p>"
            "<input type=\"text\" name=\"url\" size=\"60\" value=\"http://\">"
            "<input type=\"submit\" value=\"Go\">"
            "</form>"
            "<hr><p><b>Bookmarks:</b></p><ul>"
            "<li><a href=\"/proxy/http://www.news.com\">News (CNET)</a></li>"
            "<li><a href=\"/proxy/http://www.wikipedia.org\">Wikipedia</a></li>"
            "<li><a href=\"/proxy/http://example.com\">Example.com</a></li>"
            "</ul></body></html>";
        return invocation_response::success(
            make_response(200, portal, "text/html", false),
            "application/json");
    }

    auto response = fetch_url(target_url);

    std::string body;
    std::string content_type = response.content_type;

    if (content_type.find("text/html") != std::string::npos) {
        body = simplify_html(response.body, target_url);
        content_type = "text/html; charset=iso-8859-1";
    } else if (content_type.find("image/svg+xml") != std::string::npos) {
        body = process_image(response.body, "image/svg+xml");
        content_type = "image/gif";
    } else if (content_type.find("image/") != std::string::npos) {
        body = process_image(response.body, content_type);
        content_type = "image/gif";
    } else {
        body = response.body;
    }

    bool is_binary = content_type.find("image/") != std::string::npos;
    return invocation_response::success(
        make_response(static_cast<int>(response.status_code), body, content_type, is_binary),
        "application/json");
}

int main() {
    run_handler(handler);
    return 0;
}
