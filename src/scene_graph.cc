#include "scene_graph.h"
#include <iostream>

using namespace Eigen;

template <class V>
template <class F, class Data>
void Octree<V>::traverse(F func, Data data0) {
  Data data = func(this_item, data0);

  for (int i=0; i<8; i++) if (children[i]) {
    children[i]->traverse(func, data);
  }

  for (auto& item : terminal_items)
    func(item, data);
}

template <class V>
void Octree<V>::add(V&& v, const Vector3f& xyz) {
  this_item = std::move(v);
}

void TdtSceneNode::render(SceneGraphTraversal& sgt) {
  std::cout << " - Hello.\n";
}

SceneGraph::SceneGraph() {
  //TdtSceneNode node0;
  auto node0 = std::make_unique<TdtSceneNode>();
  tree.add(std::move(node0), Vector3f::Zero());
}

void SceneGraph::render(const RenderState& rs) {
  SceneGraphTraversal sgt;
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
