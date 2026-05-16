#include "image_processor.h"
#include <Magick++.h>

std::string process_image(const std::string& image_data, const std::string& content_type) {
    try {
        Magick::Blob input(image_data.data(), image_data.size());
        Magick::Image img(input);

        if (img.columns() > 320) {
            img.resize(Magick::Geometry("320x"));
        }

        img.quantizeColors(256);
        img.quantize();
        img.magick("GIF");

        Magick::Blob output;
        img.write(&output);
        return std::string(static_cast<const char*>(output.data()), output.length());
    } catch (...) {
        // If conversion fails, return original data as-is
        return image_data;
    }
}
