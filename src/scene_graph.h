#include <Eigen/Core>
#include <memory>


template <class V>
class Octree {};

struct RenderState {
  Eigen::Matrix4f view_proj; // = proj . view
  double engineTime;
};

struct SceneGraphTraversal {
  Eigen::Matrix4f model;
  uint16_t depth;

  inline SceneGraphTraversal() : model(Eigen::Matrix4f::Identity()), depth(0) {};
};

struct SceneNode {
  virtual void render() =0;
};
using SceneNodeUPtr = std::unique_ptr<SceneNode>;



class SceneGraph {
  public:

    SceneGraph();

    void render(const RenderState& rs, const SceneGraphTraversal& sgt);

  private:

    Octree<SceneNode> tree;

};
