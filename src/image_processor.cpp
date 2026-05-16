#include "image_processor.h"
#include <Magick++.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

std::string process_image(const std::string& image_data, const std::string& content_type) {
    try {
        Magick::Blob input;

        if (content_type.find("svg") != std::string::npos) {
            // Write SVG to temp file, convert with rsvg-convert, read PNG output
            std::string svg_path = "/tmp/input.svg";
            std::string png_path = "/tmp/output.png";
            {
                std::ofstream f(svg_path, std::ios::binary);
                f.write(image_data.data(), image_data.size());
            }
            std::string cmd = "LD_LIBRARY_PATH=/var/task/lib /var/task/bin/rsvg-convert -w 320 -o " + png_path + " " + svg_path + " 2>/dev/null";
            int ret = system(cmd.c_str());
            if (ret == 0) {
                std::ifstream f(png_path, std::ios::binary);
                std::ostringstream ss;
                ss << f.rdbuf();
                std::string png_data = ss.str();
                input = Magick::Blob(png_data.data(), png_data.size());
            } else {
                // rsvg-convert failed — return 1x1 placeholder
                static const char gif1x1[] = "GIF89a\x01\x00\x01\x00\x80\x00\x00\xff\xff\xff\x00\x00\x00!\xf9\x04\x00\x00\x00\x00\x00,\x00\x00\x00\x00\x01\x00\x01\x00\x00\x02\x02D\x01\x00;";
                return std::string(gif1x1, 43);
            }
            std::remove(svg_path.c_str());
            std::remove(png_path.c_str());
        } else {
            input = Magick::Blob(image_data.data(), image_data.size());
        }

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
        static const char gif1x1[] = "GIF89a\x01\x00\x01\x00\x80\x00\x00\xff\xff\xff\x00\x00\x00!\xf9\x04\x00\x00\x00\x00\x00,\x00\x00\x00\x00\x01\x00\x01\x00\x00\x02\x02D\x01\x00;";
        return std::string(gif1x1, 43);
    }
}
