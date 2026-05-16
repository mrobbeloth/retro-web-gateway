#include "html_simplifier.h"
#include <gumbo.h>
#include <sstream>

static void walk(GumboNode* node, std::ostringstream& out) {
    if (node->type == GUMBO_NODE_TEXT) {
        out << node->v.text.text;
        return;
    }
    if (node->type != GUMBO_NODE_ELEMENT) return;

    GumboTag tag = node->v.element.tag;

    // Skip script, style, noscript, svg, canvas
    if (tag == GUMBO_TAG_SCRIPT || tag == GUMBO_TAG_STYLE ||
        tag == GUMBO_TAG_NOSCRIPT || tag == GUMBO_TAG_SVG ||
        tag == GUMBO_TAG_CANVAS) {
        return;
    }

    // Emit only retro-safe tags
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
        // Preserve href and src attributes only
        GumboVector* attrs = &node->v.element.attributes;
        for (unsigned i = 0; i < attrs->length; i++) {
            auto* attr = static_cast<GumboAttribute*>(attrs->data[i]);
            if (std::string(attr->name) == "href" || std::string(attr->name) == "src" ||
                std::string(attr->name) == "alt") {
                out << " " << attr->name << "=\"" << attr->value << "\"";
            }
        }
        out << ">";
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned i = 0; i < children->length; i++) {
        walk(static_cast<GumboNode*>(children->data[i]), out);
    }

    if (emit_tag && tag != GUMBO_TAG_BR && tag != GUMBO_TAG_HR && tag != GUMBO_TAG_IMG) {
        out << "</" << tagname << ">";
    }
}

std::string simplify_html(const std::string& html) {
    GumboOutput* output = gumbo_parse(html.c_str());
    std::ostringstream out;
    out << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n";
    walk(output->root, out);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return out.str();
}
