#include "tdt_node.h"

TdtNode::TdtNode(tinygltf::TinyGLTF& tgltf, tinytdt::TileSpec& ts) : ts(ts) {
  if (ts.content.model) {
    //ts.content.model->model.render(sgt);
    tinygltf::Model the_model;
    ts.content.model->make_model(tgltf, &the_model);
    model = GltfModel(the_model);
  }

}

void TdtNode::render(SceneGraphTraversal& sgt) {


  model.renderScene(sgt, 0);
}



void TdtNode::print(std::ostream& os, int depth) const {
  using namespace tinytdt;
  tab_it(os, depth) << "- TdtNode:\n";
  tab_it(os, depth) << "-   asset version: " << model.model.asset.version << "\n";
  tab_it(os, depth) << "-   asset gen    : " << model.model.asset.generator << "\n";
  tab_it(os, depth) << "-   n_scenes  : " << model.model.scenes.size() << "\n";
  tab_it(os, depth) << "-   n_meshes  : " << model.model.meshes.size() << "\n";
  tab_it(os, depth) << "-   n_nodes   : " << model.model.nodes.size()  << "\n";
  tab_it(os, depth) << "-   n_buffers : " << model.model.buffers.size()  << "\n";
  tab_it(os, depth) << "-   n_bufferViews : " << model.model.bufferViews.size()  << "\n";
}

