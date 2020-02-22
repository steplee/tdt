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
namespace ProgramCache { class Program; };

class SceneGraph;
struct GltfNode;

class MultiGltfContainer {
  public:
    MultiGltfContainer();

    using ModelId = int32_t;

    std::shared_ptr<tinygltf::Model> loadNew(const std::string& path);
    void loadScene(std::vector<tinygltf::Model>& model, int scene);

  private:

    std::vector<tinygltf::Model> models;

};

/*
 * Update:
 * I will change to start this as simple as possible.
 * Externally, you will render a scene at a time.
 * Internally, it will recursively visit all nodes and compose
 * all transformations each render call.
 */

/*
 * TODO
 * Right now when parsing the gltf, we create all buffers and textures.
 * We should do lazy-loading on accessing e.g. vbos[1], by having a dependency graph for nodes.
 * Have a getter getVbo(idx) that checks if VBO idx was created yet.
 *
 * NOTE
 * By having every GltfNode contain a shared_ptr to its tinygltf::Model, we needn't worry about deallocation:
 * it is handled in the destructor when the last GltfNode is removed.
 *
 */
struct GltfModel {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    GltfModel();
    GltfModel(const std::string& path);
    GltfModel(tinygltf::Model& model);
    ~GltfModel();

    void setup(/*tinygltf::Model& model*/);

    tinygltf::Model model;

    // numbered as Model::buffers/textures
    std::vector<GLuint> vbos;
    std::vector<GLuint> textures;

    std::shared_ptr<ProgramCache::Program> program_to_use = nullptr;

    Eigen::Matrix4f transform;

    Eigen::Matrix4f get_node_xform(int node);

    //void recursiveAddNodes(SceneGraph* graph, int scene, int node);

    void renderScene(const SceneGraphTraversal& sgt, int scene);

  private:
    void renderNode(const SceneGraphTraversal& sgt, int node);

    //friend GltfNode;
};

