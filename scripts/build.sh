#!/bin/bash
# Build script for the retro-web-gateway Lambda
# Requires: docker
set -e

echo "Building retro-web-gateway in Docker (Amazon Linux 2023)..."

docker run --rm -v "$(pwd)":/src -w /src amazonlinux:2023 bash -c '
    set -e
    dnf install -y cmake gcc-c++ libcurl-devel ImageMagick-c++-devel git zip make autoconf automake libtool pkgconfig libjpeg-turbo-devel libpng-devel

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

    # Add ImageMagick delegates (JPEG/PNG) that are loaded at runtime
    cd /src/build
    mkdir -p tmp && cd tmp && unzip ../retro-web-gateway.zip
    cp /usr/lib64/libjpeg.so.62 lib/ 2>/dev/null || true
    cp /usr/lib64/libpng16.so.16 lib/ 2>/dev/null || true
    cp /usr/lib64/libturbojpeg.so.0 lib/ 2>/dev/null || true
    # Copy ImageMagick coders (delegates)
    mkdir -p lib/ImageMagick
    cp -r /usr/lib64/ImageMagick-6.9.13/modules-Q16/coders lib/ImageMagick/ 2>/dev/null || true
    zip -r ../retro-web-gateway.zip .
    cd /src/build && rm -rf tmp
'

echo "Build complete: build/retro-web-gateway.zip"
