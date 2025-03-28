cmake_minimum_required(VERSION 3.7)
project(packetreplay)

set(lib_srcs src/lib/capture.cc src/lib/tcp_conversation.cc src/lib/udp_conversation.cc src/lib/conversation_factory.cc src/lib/util.cc)
set(http_srcs src/http_replay/http_replay.cc src/http_replay/http_response_processor.cc)
set(udp_srcs src/udp_replay/udp_replay.cc)

add_compile_options(-std=c++20)
include_directories(include)
include(FindPCAP.cmake)

# find_package(PythonLibs REQUIRED)

add_library(packet_replay STATIC ${lib_srcs})
add_executable(http_replay ${http_srcs})
add_executable(udp_replay ${udp_srcs})

# target_include_directories(udp_replay PRIVATE ${PYTHON_INCLUDE_DIRS})

target_link_libraries(http_replay packet_replay ${PCAP_LIBRARY})
# target_link_libraries(udp_replay packet_replay ${PCAP_LIBRARY} ${PYTHON_LIBRARIES})
target_link_libraries(udp_replay packet_replay ${PCAP_LIBRARY} ${PYTHON_LIBRARIES})
