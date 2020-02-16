#include <Eigen/Core>
#include <memory>


/*
 *
 * We need to build a stack of transformations as we traverse down the scene graph.
 * This could be done using global variables like glPushMatrix etc., but
 * I will opt to pass the state explicitly as a parameter.
 * I do not build the stack however, I will just use recursive calls w/ the
 * stack from the function calls.
 *
 * I may support other spatial data-structures later on, so I am trying to keep the
 * SceneGraph class storage independent, which makes the code considerably more complex
 * than just having it for only an octree.
 *
 */

template <class V>
class Octree {
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    template <class F, class Data>
    void traverse(F func, Data data);

    void add(V&& v, const Eigen::Vector3f& xyz);

  private:

    Octree* children[8] = {nullptr};

    // We can have either (or both) one item and several terminal items.
    // Sibiling terminal items DO NOT compose / see each other.
    // (e.g. they have independent stacks, given parent)
    // If you wanted that, have a chain of nested Octree with 'this_item's.
    V this_item;
    std::vector<V> terminal_items;

};

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
  virtual void render(SceneGraphTraversal& sgt) {};
};
struct TdtSceneNode : public SceneNode {
  virtual void render(SceneGraphTraversal& sgt) override;
};
using SceneNodeUPtr = std::unique_ptr<SceneNode>;



class SceneGraph {
  public:

    SceneGraph();

    void render(const RenderState& rs);
    void render(const RenderState& rs, const SceneGraphTraversal& sgt);

  private:

    Octree<SceneNodeUPtr> tree;

};
