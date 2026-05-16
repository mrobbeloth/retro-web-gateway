#!/bin/bash
# Build script for the retro-web-gateway Lambda
# Requires: docker
set -e

echo "Building retro-web-gateway in Docker (Amazon Linux 2023)..."

docker run --rm -v "$(pwd)":/src -w /src amazonlinux:2023 bash -c '
    set -e
    dnf install -y cmake gcc-c++ libcurl-devel ImageMagick-c++-devel git zip make autoconf automake libtool pkgconfig libjpeg-turbo-devel libpng-devel librsvg2-devel

    # Build gumbo-parser from source (static)
    git clone https://github.com/google/gumbo-parser.git /tmp/gumbo
    cd /tmp/gumbo
    ./autogen.sh && ./configure --prefix=/usr/local --enable-static --disable-shared
    make -j$(nproc) && make install
    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
    ldconfig

    # Build AWS Lambda C++ Runtime
    git clone https://github.com/awslabs/aws-lambda-cpp.git /tmp/aws-lambda-cpp
    cd /tmp/aws-lambda-cpp
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
    make -j$(nproc) && make install

    # Build the project
    cd /src
    rm -rf build && mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr/local
    make -j$(nproc)
    make aws-lambda-package-retro-web-gateway

    # Add ImageMagick delegates (JPEG/PNG/SVG) that are loaded at runtime
    cd /src/build
    mkdir -p tmp && cd tmp && unzip ../retro-web-gateway.zip
    cp /usr/lib64/libjpeg.so.62 lib/ 2>/dev/null || true
    cp /usr/lib64/libpng16.so.16 lib/ 2>/dev/null || true
    cp /usr/lib64/libturbojpeg.so.0 lib/ 2>/dev/null || true
    # SVG support via librsvg
    cp /usr/lib64/librsvg-2.so.2 lib/ 2>/dev/null || true
    cp /usr/lib64/libcairo.so.2 lib/ 2>/dev/null || true
    cp /usr/lib64/libgdk_pixbuf-2.0.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libgio-2.0.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libgobject-2.0.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libglib-2.0.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libpango-1.0.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libpangocairo-1.0.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libpangoft2-1.0.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libfribidi.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libharfbuzz.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libfontconfig.so.1 lib/ 2>/dev/null || true
    cp /usr/lib64/libfreetype.so.6 lib/ 2>/dev/null || true
    cp /usr/lib64/libpixman-1.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libffi.so.8 lib/ 2>/dev/null || true
    cp /usr/lib64/libcairo-gobject.so.2 lib/ 2>/dev/null || true
    # rsvg-convert binary (ImageMagick delegates SVG to this)
    dnf install -y librsvg2-tools 2>/dev/null | tail -1
    mkdir -p bin
    cp /usr/bin/rsvg-convert bin/ 2>/dev/null || true
    # Additional libs needed by rsvg-convert
    cp /usr/lib64/libdatrie.so.1 lib/ 2>/dev/null || true
    cp /usr/lib64/libthai.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libmount.so.1 lib/ 2>/dev/null || true
    cp /usr/lib64/libblkid.so.1 lib/ 2>/dev/null || true
    cp /usr/lib64/libgmodule-2.0.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libXrender.so.1 lib/ 2>/dev/null || true
    cp /usr/lib64/libX11.so.6 lib/ 2>/dev/null || true
    cp /usr/lib64/libXext.so.6 lib/ 2>/dev/null || true
    cp /usr/lib64/libxcb.so.1 lib/ 2>/dev/null || true
    cp /usr/lib64/libxcb-render.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libxcb-shm.so.0 lib/ 2>/dev/null || true
    cp /usr/lib64/libXau.so.6 lib/ 2>/dev/null || true
    # Copy ImageMagick coders (delegates)
    mkdir -p lib/ImageMagick
    cp -r /usr/lib64/ImageMagick-6.9.13/modules-Q16/coders lib/ImageMagick/ 2>/dev/null || true
    # Copy ImageMagick config (delegates.xml needed for SVG)
    mkdir -p lib/ImageMagick/config
    cp /usr/lib64/ImageMagick-6.9.13/config-Q16/*.xml lib/ImageMagick/config/ 2>/dev/null || true
    cp /etc/ImageMagick-6/*.xml lib/ImageMagick/config/ 2>/dev/null || true
    zip -r ../retro-web-gateway.zip .
    cd /src/build && rm -rf tmp
'

echo "Build complete: build/retro-web-gateway.zip"
