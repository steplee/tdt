#include "scene_graph.h"
#include <iostream>
#include "gltf_node.h"

#define TINY_TDT_IMPLEMENTATION
#include "tiny_tdt.h"

using namespace Eigen;

SceneGraphTraversal::SceneGraphTraversal(const Eigen::Matrix4f& mvp, uint16_t depth)
  : mvp(mvp), depth(depth)
{ }



#if 0

static void test_quadrant_of() {
  AlignedBox<float,3> box { Vector3f{0,0,0} , Vector3f{2,2,2} };
  std::cout << " - original: " << box.min().transpose() << " => " << box.max().transpose() << "\n";

  AlignedBox<float,3> box000 = quadrant_of(box, 0);
  std::cout << " - quad 000: " << box000.min().transpose() << " => " << box000.max().transpose() << "\n";
  AlignedBox<float,3> box010 = quadrant_of(box, 2);
  std::cout << " - quad 010: " << box010.min().transpose() << " => " << box010.max().transpose() << "\n";
  AlignedBox<float,3> box111 = quadrant_of(box, 7);
  std::cout << " - quad 111: " << box111.min().transpose() << " => " << box111.max().transpose() << "\n";
}
static void test_find_quadrant() {
  AlignedBox<float,3> box { Vector3f{0,0,0} , Vector3f{2,2,2} };
  Eigen::Vector3f q { 0, 1.2, 1.2 };
  std::cout << " - quad of " << q.transpose() << " => " << static_cast<int>(find_quadrant(box, q)) << "\n";
}

template <class V>
Octree<V>::Octree(const Eigen::AlignedBox<float, 3>& extent)
  : extent(extent)
{
}

template <class V>
template <class F, class Data>
void Octree<V>::traverse(F func, Data data) {
  if (this_item) {
    data = func(this_item, data);
  }

  for (int i=0; i<8; i++) if (children[i]) {
    children[i]->traverse(func, data);
  }

  for (auto& item : terminal_items)
    func(item, data);
}

template <class V>
void Octree<V>::add(V&& v) {
  //this_item = std::move(v);
  auto bb = v->boundingBox;

#ifndef NDEBUG // redundant with standard c assert.
  assert(extent.contains(bb));
#endif

  // If min/max lie in same child bbox, we should propagate down to it.
  // Otherwise it lies in multiple children, and we should contain it ourself.
  uint8_t quad_bl = find_quadrant(extent, bb.min());
  uint8_t quad_tr = find_quadrant(extent, bb.max());
  if (quad_bl == quad_tr) {
    if (children[quad_bl] == nullptr)
      children[quad_bl] = std::make_unique<Octree>(quadrant_of(extent, quad_bl));

    children[quad_bl]->add(std::move(v));
  } else {
    terminal_items.push_back(std::move(v));
  }

}

void TdtSceneNode::render(SceneGraphTraversal& sgt) {
  std::cout << " - Hello.\n";
  test_quadrant_of();
  test_find_quadrant();
}






SceneGraph::SceneGraph()
  : tree(AlignedBox<float,3> ( Vector3f{-100,-100,-100} , Vector3f{100,100,100} ))
{
  //TdtSceneNode node0;
  auto node0 = std::make_unique<TdtSceneNode>();
  tree.add(std::move(node0));

  /*
  auto model = std::make_shared<GltfModel>("../3rdparty/tinygltf/models/Cube2/Cube.gltf");
  auto node1 = std::make_unique<GltfNode>(model, 0, 0);
  auto node2 = std::make_unique<GltfNode>(model, 0, 1);
  tree.add(std::move(node1));
  tree.add(std::move(node2));
  */
  auto model = std::make_shared<GltfModel>("../3rdparty/tinygltf/models/Cube/Cube.gltf");
  //model->recursiveAddNodes(&tree, 0, 0);

}

void SceneGraph::render(const RenderState& rs) {
  SceneGraphTraversal sgt(rs.view_proj, 0);
  render(rs, sgt);
}
void SceneGraph::render(const RenderState& rs, const SceneGraphTraversal& sgt) {
  auto func = [](SceneNodeUPtr& node, SceneGraphTraversal& sgt) -> SceneGraphTraversal {
      std::cout << "render.\n";
      node->render(sgt);
      return sgt;
  };
  tree.traverse<decltype(func),SceneGraphTraversal>(func, sgt);
}
#endif
