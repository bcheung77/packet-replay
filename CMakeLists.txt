cmake_minimum_required(VERSION 3.7)
project(packetreplay)

set(lib_srcs src/lib/capture.cc src/lib/tcp_conversation.cc src/lib/tcp_conversation_store.cc)
set(http_srcs src/http_replay/http_replay.cc src/http_replay/http_response_processor.cc)

add_compile_options(-std=c++20)
include_directories(include)
include(FindPCAP.cmake)

add_library(packet_replay STATIC ${lib_srcs})
add_executable(http_replay  ${http_srcs})
target_link_libraries(http_replay packet_replay ${PCAP_LIBRARY})
