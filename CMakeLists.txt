cmake_minimum_required(VERSION 3.10)

project(tdt)

include_directories(${PROJECT_SOURCE_DIR}/3rdparty)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR})

find_package(glfw3 REQUIRED)
find_package(Eigen3 REQUIRED)


add_executable(tdt_main
  src/main.cc
  src/window.cc
  src/program_cache.cc
  src/scene_graph.cc
  )

target_link_libraries(tdt_main
  m pthread
  #gflw
  ${GLFW3_LIBRARY}
  Eigen3::Eigen
  GL GLU GLEW
  )
target_compile_features(tdt_main PUBLIC cxx_std_14)
