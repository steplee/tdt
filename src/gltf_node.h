#pragma once

#include <Eigen/StdVector>
#include <Eigen/Core>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

#include "scene_graph.h"


#include <GL/glew.h>

//namespace tinygltf { struct Model; struct Node; struct Primitive; }



class GltfNode : public SceneNode {
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    GltfNode(const std::string& path);

    virtual void render(SceneGraphTraversal& sgt) override;

  private:
    void setup(tinygltf::Model& model);

    std::vector<GLuint> vbos;

    tinygltf::Model model;
};
