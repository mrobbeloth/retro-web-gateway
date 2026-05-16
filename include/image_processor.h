#pragma once
#include <string>

// Downsamples image data to retro-friendly format (GIF87a or low-quality JPEG)
std::string process_image(const std::string& image_data, const std::string& content_type);
