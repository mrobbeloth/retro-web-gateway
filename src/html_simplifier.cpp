#include "html_simplifier.h"
#include <gumbo.h>
#include <sstream>

static const std::string PROXY_PREFIX = "/proxy/";

// Resolve a possibly-relative URL against a base URL
static std::string resolve_url(const std::string& href, const std::string& base_url) {
    if (href.empty() || href[0] == '#') return href;
    if (href.find("http://") == 0 || href.find("https://") == 0) return href;
    if (href.size() > 2 && href[0] == '/' && href[1] == '/') return "http:" + href;

    // Extract scheme+host from base_url
    std::string base_origin;
    auto scheme_end = base_url.find("://");
    if (scheme_end != std::string::npos) {
        auto path_start = base_url.find('/', scheme_end + 3);
        base_origin = (path_start != std::string::npos)
            ? base_url.substr(0, path_start)
            : base_url;
    } else {
        base_origin = base_url;
    }

    if (href[0] == '/') return base_origin + href;

    // Relative path: append to base directory
    auto last_slash = base_url.rfind('/');
    if (last_slash != std::string::npos && last_slash > base_url.find("://") + 2)
        return base_url.substr(0, last_slash + 1) + href;
    return base_origin + "/" + href;
}

static void walk(GumboNode* node, std::ostringstream& out, const std::string& base_url) {
    if (node->type == GUMBO_NODE_TEXT) {
        out << node->v.text.text;
        return;
    }
    if (node->type != GUMBO_NODE_ELEMENT) return;

    GumboTag tag = node->v.element.tag;

    if (tag == GUMBO_TAG_SCRIPT || tag == GUMBO_TAG_STYLE ||
        tag == GUMBO_TAG_NOSCRIPT || tag == GUMBO_TAG_SVG ||
        tag == GUMBO_TAG_CANVAS) {
        return;
    }

    const char* tagname = gumbo_normalized_tagname(tag);
    bool emit_tag = (tag == GUMBO_TAG_HTML || tag == GUMBO_TAG_HEAD ||
                     tag == GUMBO_TAG_TITLE || tag == GUMBO_TAG_BODY ||
                     tag == GUMBO_TAG_H1 || tag == GUMBO_TAG_H2 ||
                     tag == GUMBO_TAG_H3 || tag == GUMBO_TAG_H4 ||
                     tag == GUMBO_TAG_P || tag == GUMBO_TAG_BR ||
                     tag == GUMBO_TAG_HR || tag == GUMBO_TAG_A ||
                     tag == GUMBO_TAG_IMG || tag == GUMBO_TAG_UL ||
                     tag == GUMBO_TAG_OL || tag == GUMBO_TAG_LI ||
                     tag == GUMBO_TAG_TABLE || tag == GUMBO_TAG_TR ||
                     tag == GUMBO_TAG_TD || tag == GUMBO_TAG_TH ||
                     tag == GUMBO_TAG_B || tag == GUMBO_TAG_I ||
                     tag == GUMBO_TAG_BLOCKQUOTE || tag == GUMBO_TAG_PRE);

    if (emit_tag) {
        out << "<" << tagname;
        GumboVector* attrs = &node->v.element.attributes;
        for (unsigned i = 0; i < attrs->length; i++) {
            auto* attr = static_cast<GumboAttribute*>(attrs->data[i]);
            std::string name = attr->name;
            std::string value = attr->value;

            if (name == "href" || name == "src") {
                // Resolve and rewrite through proxy
                std::string resolved = resolve_url(value, base_url);
                if (resolved.find("http") == 0) {
                    value = PROXY_PREFIX + resolved;
                }
                out << " " << name << "=\"" << value << "\"";
            } else if (name == "alt") {
                out << " " << name << "=\"" << value << "\"";
            }
        }
        out << ">";
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned i = 0; i < children->length; i++) {
        walk(static_cast<GumboNode*>(children->data[i]), out, base_url);
    }

    if (emit_tag && tag != GUMBO_TAG_BR && tag != GUMBO_TAG_HR && tag != GUMBO_TAG_IMG) {
        out << "</" << tagname << ">";
    }
}

std::string simplify_html(const std::string& html, const std::string& base_url) {
    GumboOutput* output = gumbo_parse(html.c_str());
    std::ostringstream out;
    out << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n";
    walk(output->root, out, base_url);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return out.str();
}
