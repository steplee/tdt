#pragma once

#include "tiny_tdt.h"
#include "gltf_node.h"


struct TdtNode {
  TdtNode(tinygltf::TinyGLTF& tgltf, tinytdt::TileSpec& ts);

  tinytdt::TileSpec& ts;
  GltfModel model;

  void render(SceneGraphTraversal& sgt);

  void print(std::ostream& os, int depth) const;

};

