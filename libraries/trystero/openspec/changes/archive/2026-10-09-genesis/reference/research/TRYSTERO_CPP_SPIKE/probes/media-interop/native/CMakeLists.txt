cmake_minimum_required(VERSION 3.20)
project(media_interop_native CXX)

set(LIBDATACHANNEL ${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/libdatachannel)

add_executable(peer peer.cpp)
target_compile_features(peer PRIVATE cxx_std_20)
target_include_directories(peer PRIVATE ${LIBDATACHANNEL}/include ${LIBDATACHANNEL}/deps/json/include)
target_link_libraries(peer PRIVATE ${LIBDATACHANNEL}/build/libdatachannel.dylib)
set_target_properties(peer PROPERTIES BUILD_RPATH ${LIBDATACHANNEL}/build)
