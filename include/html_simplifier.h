#pragma once
#include <string>

// Strips JS, modern CSS, and converts HTML to ~3.2-compatible markup
std::string simplify_html(const std::string& html);
