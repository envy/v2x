cmake_minimum_required(VERSION 3.13)
project(v2x)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

find_package(SFML 2.5 COMPONENTS graphics window REQUIRED)

file(GLOB ASN_FILES ${PROJECT_SOURCE_DIR}/asn1-src/*.c)

set(APP_SOURCE_FILES
        factory.cpp
        main.cpp
        parser.cpp
        proxy.cpp
        MessageSink.cpp)

add_executable(app ${APP_SOURCE_FILES} ${ASN_FILES})

target_include_directories(app PUBLIC . asn1-src)
target_link_libraries(app sfml-graphics sfml-window pthread)
