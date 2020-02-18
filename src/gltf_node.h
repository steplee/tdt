#pragma once

#include <Eigen/StdVector>
#include <Eigen/Core>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

#include "scene_graph.h"


#include <GL/glew.h>

// My shaders will use this fixed numbering.
#define MY_GL_COLOR_TEXTURE GL_TEXTURE0
#define MY_GL_SPECULAR_TEXTURE GL_TEXTURE1
#define MY_GL_ROUGHNESS_TEXTURE GL_TEXTURE2

//namespace tinygltf { struct Model; struct Node; struct Primitive; }



class GltfNode : public SceneNode {
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    GltfNode(const std::string& path);
    ~GltfNode();

    virtual void render(SceneGraphTraversal& sgt) override;

  private:
    void setup(tinygltf::Model& model);

    // numbered as Model::buffers/textures
    std::vector<GLuint> vbos;
    std::vector<GLuint> textures;

    tinygltf::Model model;
};
