cmake_minimum_required(VERSION 3.22)
project(retro-web-gateway LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(ImageMagick COMPONENTS Magick++ REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUMBO REQUIRED gumbo)
find_package(aws-lambda-runtime REQUIRED)

add_executable(retro-web-gateway
    src/main.cpp
    src/fetcher.cpp
    src/html_simplifier.cpp
    src/image_processor.cpp
)

target_include_directories(retro-web-gateway PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${GUMBO_INCLUDE_DIRS}
    ${ImageMagick_INCLUDE_DIRS}
)

# Prefer static gumbo
find_library(GUMBO_STATIC_LIB libgumbo.a PATHS /usr/local/lib)
if(GUMBO_STATIC_LIB)
    set(GUMBO_LINK ${GUMBO_STATIC_LIB})
else()
    set(GUMBO_LINK ${GUMBO_LIBRARIES})
endif()

target_link_libraries(retro-web-gateway PRIVATE
    AWS::aws-lambda-runtime
    CURL::libcurl
    ${GUMBO_LINK}
    ${ImageMagick_LIBRARIES}
)

aws_lambda_package_target(retro-web-gateway)
