cmake_minimum_required(VERSION 3.7)
project(packetreplay)

set(lib_srcs src/lib/capture.cc src/lib/tcp_conversation.cc src/lib/udp_conversation.cc src/lib/conversation_factory.cc src/lib/util.cc src/lib/python_api.cc 
    src/lib/packet_validator.cc src/lib/conversation_serializer.cc src/lib/properties.cc)
set(http_srcs src/http_replay/http_replay.cc src/http_replay/http_response_processor.cc)
set(udp_srcs src/udp_replay/udp_replay.cc)

add_compile_options(-std=c++20)
include_directories(src/include)
include(FindPCAP.cmake)

find_package(Python REQUIRED Development)

add_library(packet_replay STATIC ${lib_srcs})
add_executable(http_replay ${http_srcs})
add_executable(udp_replay ${udp_srcs})

message(Python_INCLUDE_DIRS=${Python_INCLUDE_DIRS})

target_include_directories(packet_replay PRIVATE ${Python_INCLUDE_DIRS})
target_include_directories(http_replay PRIVATE src/include)
target_include_directories(udp_replay PRIVATE src/include)

target_link_libraries(http_replay packet_replay ${PCAP_LIBRARY})
target_link_libraries(udp_replay packet_replay ${PCAP_LIBRARY} ${Python_LIBRARIES})
