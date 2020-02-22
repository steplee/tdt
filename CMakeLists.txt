cmake_minimum_required(VERSION 3.10)

project(tdt)

include_directories(${PROJECT_SOURCE_DIR}/3rdparty)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR})

find_package(glfw3 REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Sophus REQUIRED)


add_executable(tdt_main
  src/main.cc
  src/twindow.cc
  src/program_cache.cc
  src/scene_graph.cc
  src/camera.cc

  src/gltf_node.cc
  )

set(ALL_LIBS
  m pthread
  ${GLFW3_LIBRARY}
  GL GLU GLEW

  Eigen3::Eigen
  Sophus::Sophus)

target_link_libraries(tdt_main ${ALL_LIBS})
target_compile_features(tdt_main PUBLIC cxx_std_14)

add_executable(test_tiny_tdt src/test_tiny_tdt.cc)
target_link_libraries(test_tiny_tdt ${ALL_LIBS})
target_compile_features(test_tiny_tdt PUBLIC cxx_std_14)
