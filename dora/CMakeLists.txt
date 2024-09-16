cmake_minimum_required(VERSION 3.0.2)
project(bridge)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})


include_directories(
  ${YAMLCPP_INCLUDE_DIRS}
  $ENV{HOME}/dora/apis/c/node #dora的头文件路径 node_api.h
  $ENV{HOME}/dora/apis/c/operator
  
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_compile_definitions(bridge PUBLIC HAVE_DRAND48)
endif ()

set(src_test main.cc)
add_executable(bridge ${src_test})
target_link_libraries(
  bridge
  $ENV{HOME}/dora/target/release/libdora_node_api_c.a
  pthread
  rt
  dl
  ${Boost_LIBS}
)
