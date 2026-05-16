#pragma once
#include <string>

// Strips JS, modern CSS, converts HTML to ~3.2-compatible markup
// Rewrites links to route through the proxy
std::string simplify_html(const std::string& html, const std::string& base_url);
