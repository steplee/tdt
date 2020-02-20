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
    GltfModel(const std::string& path);
    ~GltfModel();

    void setup(tinygltf::Model& model);

    tinygltf::Model model;

    // numbered as Model::buffers/textures
    std::vector<GLuint> vbos;
    std::vector<GLuint> textures;

    std::shared_ptr<ProgramCache::Program> program_to_use = nullptr;

    Eigen::Matrix4f transform;

    void recursiveAddNodes(SceneGraph* graph, int scene, int node);

    friend GltfNode;
};


/*
 * NOTE
 * A GltfNode has no knowledge of its parents or children. The SceneGraph becomes the manager of such relations.
 * So when we load a model, we reorganize the glTF node hierarchy to our own SceneGraph hierarchy,
 * composing node transformations as needed.
 *
 * Now this conversion is actually ill-formed: glTF obviously does not require children to be contained within
 * parents' spatial extent. The important question is whether the 3dTile spec *does*?
 * I will deal with that, if needed, when I get to it.
 *
 */
class GltfNode : public SceneNode {
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  private:
    // NOTE: Private constructor: we can only create through GltfModel::recursiveAddNodes()
    GltfNode(std::shared_ptr<GltfModel>& model, int scene, int node,
        Eigen::Matrix4f prefixTransformation=Eigen::Matrix4f::Identity());

  public:

    virtual void render(SceneGraphTraversal& sgt) override;

  private:
    void setup();

    std::shared_ptr<GltfModel> model;
    int scene, node;

    Eigen::Matrix4f transform;

    // We could specify this, not really needed though.
    //std::shared_ptr<ProgramCache::Program> program_to_use = nullptr;

};
