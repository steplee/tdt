
#include <Eigen/Core>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define TINY_TDT_IMPLEMENTATION
#include "tiny_tdt.h"

using namespace tinytdt;

int main() {

  Tileset ts("./assets/tileset.json");

  ts.print(std::cout);



  return 0;
}
