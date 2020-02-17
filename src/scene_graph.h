#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>
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

// clang-format off
// Split the input box depending on the lowest 3 bits of quad.
inline Eigen::AlignedBox<float, 3> quadrant_of(const Eigen::AlignedBox<float,3>& b, uint8_t quad) {
  return Eigen::AlignedBox<float, 3> {
    Eigen::Vector3f {
        (quad&1) ? (b.min()(0) + b.max()(0)) / 2 : b.min()(0),
        (quad&2) ? (b.min()(1) + b.max()(1)) / 2 : b.min()(1),
        (quad&4) ? (b.min()(2) + b.max()(2)) / 2 : b.min()(2) },
     Eigen::Vector3f {
       !(quad&1) ? (b.min()(0) + b.max()(0)) / 2 : b.max()(0),
       !(quad&2) ? (b.min()(1) + b.max()(1)) / 2 : b.max()(1),
       !(quad&4) ? (b.min()(2) + b.max()(2)) / 2 : b.max()(2) }
  };
}
inline uint8_t find_quadrant(const Eigen::AlignedBox<float, 3>& b, const Eigen::Vector3f& pt) {
  return
    ((pt(0) > (b.min()(0) + b.max()(0))/2) << 0) |
    ((pt(1) > (b.min()(1) + b.max()(1))/2) << 1) |
    ((pt(2) > (b.min()(2) + b.max()(2))/2) << 2) ;
}
// clang-format on

// Constraint on V: it must have 'boundingBox' as a public member.
// Constraint on V: it must be a pointer-type (this can be fixed using SFINAE, problem
// occurs due to '->' vs. '.')
template <class V>
class Octree {
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    Octree(const Eigen::AlignedBox<float, 3>& extent);

    template <class F, class Data>
    void traverse(F func, Data data);

    void add(V&& v);

  private:

    Eigen::AlignedBox<float, 3> extent;

    std::unique_ptr<Octree> children[8] = {nullptr};

    // We can have either (or both) one item and several terminal items.
    // Sibiling terminal items DO NOT compose / see each other.
    // (e.g. they have independent stacks, given parent)
    // If you wanted that, have a chain of nested Octree with 'this_item's.
    // TODO: think more about this: it may make more sense to allow only a transform, and not
    // have a 'this_item' field and only a vector called 'items'
    V this_item;
    std::vector<V> terminal_items;

};

struct RenderState {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  Eigen::Matrix4f view_proj; // = proj . view
  double engineTime;
};

struct SceneGraphTraversal {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  Eigen::Matrix4f model;
  uint16_t depth;

  inline SceneGraphTraversal() : model(Eigen::Matrix4f::Identity()), depth(0) {};
};

struct SceneNode {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  virtual void render(SceneGraphTraversal& sgt) {};

  Eigen::AlignedBox<float, 3> boundingBox;
};
struct TdtSceneNode : public SceneNode {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
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
