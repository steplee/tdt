#pragma once

#include "scene_graph.h"

#include <Eigen/Core>

#include <GL/glew.h>

namespace tinygltf { struct Model; }

class GltfNode : public SceneNode {
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    GltfNode(const std::string& path);

    virtual void render(SceneGraphTraversal& sgt) override;

  private:
    void setup(tinygltf::Model& model);

    GLuint vbo;
    GLuint n_verts;
};
