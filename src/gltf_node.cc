#define TINYGLTF_IMPLEMENTATION
#include "gltf_node.h"

#include <iostream>


#include <cstdlib>

using namespace tinygltf;

static std::string int_to_glfw_type(int t) {
 if (t ==  (0)) return "TINYGLTF_MODE_POINTS";
 if (t ==  (1)) return "TINYGLTF_MODE_LINE";
 if (t ==  (2)) return "TINYGLTF_MODE_LINE_LOOP";
 if (t ==  (3)) return "TINYGLTF_MODE_LINE_STRIP";
 if (t ==  (4)) return "TINYGLTF_MODE_TRIANGLES";
 if (t ==  (5)) return "TINYGLTF_MODE_TRIANGLE_STRIP";
 if (t ==  (6)) return "TINYGLTF_MODE_TRIANGLE_FAN";

 if (t ==  (5120)) return "TINYGLTF_COMPONENT_TYPE_BYTE";
 if (t ==  (5121)) return "TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE";
 if (t ==  (5122)) return "TINYGLTF_COMPONENT_TYPE_SHORT";
 if (t ==  (5123)) return "TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT";
 if (t ==  (5124)) return "TINYGLTF_COMPONENT_TYPE_INT";
 if (t ==  (5125)) return "TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT";
 if (t ==  (5126)) return "TINYGLTF_COMPONENT_TYPE_FLOAT";
 if (t ==  (5130)) return "TINYGLTF_COMPONENT_TYPE_DOUBLE";

 if (t ==  (9728)) return "TINYGLTF_TEXTURE_FILTER_NEAREST";
 if (t ==  (9729)) return "TINYGLTF_TEXTURE_FILTER_LINEAR";
 if (t ==  (9984)) return "TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST";
 if (t ==  (9985)) return "TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST";
 if (t ==  (9986)) return "TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR";
 if (t ==  (9987)) return "TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR";

 if (t ==  (10497)) return "TINYGLTF_TEXTURE_WRAP_REPEAT";
 if (t ==  (33071)) return "TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE";
 if (t ==  (33648)) return "TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT";

 if (t ==  (5120)) return "TINYGLTF_PARAMETER_TYPE_BYTE";
 if (t ==  (5121)) return "TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE";
 if (t ==  (5122)) return "TINYGLTF_PARAMETER_TYPE_SHORT";
 if (t ==  (5123)) return "TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT";
 if (t ==  (5124)) return "TINYGLTF_PARAMETER_TYPE_INT";
 if (t ==  (5125)) return "TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT";
 if (t ==  (5126)) return "TINYGLTF_PARAMETER_TYPE_FLOAT";

 if (t ==  (35664)) return "TINYGLTF_PARAMETER_TYPE_FLOAT_VEC2";
 if (t ==  (35665)) return "TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3";
 if (t ==  (35666)) return "TINYGLTF_PARAMETER_TYPE_FLOAT_VEC4";

 if (t ==  (35667)) return "TINYGLTF_PARAMETER_TYPE_INT_VEC2";
 if (t ==  (35668)) return "TINYGLTF_PARAMETER_TYPE_INT_VEC3";
 if (t ==  (35669)) return "TINYGLTF_PARAMETER_TYPE_INT_VEC4";

 if (t ==  (35670)) return "TINYGLTF_PARAMETER_TYPE_BOOL";
 if (t ==  (35671)) return "TINYGLTF_PARAMETER_TYPE_BOOL_VEC2";
 if (t ==  (35672)) return "TINYGLTF_PARAMETER_TYPE_BOOL_VEC3";
 if (t ==  (35673)) return "TINYGLTF_PARAMETER_TYPE_BOOL_VEC4";

 if (t ==  (35674)) return "TINYGLTF_PARAMETER_TYPE_FLOAT_MAT2";
 if (t ==  (35675)) return "TINYGLTF_PARAMETER_TYPE_FLOAT_MAT3";
 if (t ==  (35676)) return "TINYGLTF_PARAMETER_TYPE_FLOAT_MAT4";

 if (t ==  (35678)) return "TINYGLTF_PARAMETER_TYPE_SAMPLER_2D";

 if (t ==  (2)) return "TINYGLTF_TYPE_VEC2";
 if (t ==  (3)) return "TINYGLTF_TYPE_VEC3";
 if (t ==  (4)) return "TINYGLTF_TYPE_VEC4";
 if (t ==  32+ 2) return "TINYGLTF_TYPE_MAT2";
 if (t ==  32+ 3) return "TINYGLTF_TYPE_MAT3";
 if (t ==  32+ 4) return "TINYGLTF_TYPE_MAT4";
 if (t ==  64+ 1) return "TINYGLTF_TYPE_SCALAR";
 if (t ==  64+ 4) return "TINYGLTF_TYPE_VECTOR";
 if (t ==  64+ 16) return "TINYGLTF_TYPE_MATRIX";
 if (t ==  (0)) return "TINYGLTF_IMAGE_FORMAT_JPEG";
 if (t ==  (1)) return "TINYGLTF_IMAGE_FORMAT_PNG";
 if (t ==  (2)) return "TINYGLTF_IMAGE_FORMAT_BMP";
 if (t ==  (3)) return "TINYGLTF_IMAGE_FORMAT_GIF";

 if (t ==  (6406)) return "TINYGLTF_TEXTURE_FORMAT_ALPHA";
 if (t ==  (6407)) return "TINYGLTF_TEXTURE_FORMAT_RGB";
 if (t ==  (6408)) return "TINYGLTF_TEXTURE_FORMAT_RGBA";
 if (t ==  (6409)) return "TINYGLTF_TEXTURE_FORMAT_LUMINANCE";
 if (t ==  (6410)) return "TINYGLTF_TEXTURE_FORMAT_LUMINANCE_ALPHA";

 if (t ==  (3553)) return "TINYGLTF_TEXTURE_TARGET_TEXTURE2D";
 if (t ==  (5121)) return "TINYGLTF_TEXTURE_TYPE_UNSIGNED_BYTE";

 if (t ==  (34962)) return "TINYGLTF_TARGET_ARRAY_BUFFER";
 if (t ==  (34963)) return "TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER";

 if (t ==  (35633)) return "TINYGLTF_SHADER_TYPE_VERTEX_SHADER";
 if (t ==  (35632)) return "TINYGLTF_SHADER_TYPE_FRAGMENT_SHADER";

 return std::to_string(t);
}

GltfNode::GltfNode(const std::string& path) {
  TinyGLTF loader;
  std::string err, warn;
  bool good = loader.LoadASCIIFromFile(&model, &err, &warn, path);
  if (err.length()) std::cout << " - GltfNode load err : " << err << "\n";
  if (warn.length()) std::cout << " - GltfNode load warn: " << warn << "\n";

  if (good) {
    setup(model);
  } else {
    std::cerr << " - Failed to load Gltf: " << path << "\n";
    exit(1);
  }
}


void GltfNode::setup(tinygltf::Model& model) {
  // Create VBOs.
  vbos.resize(model.buffers.size());
  glGenBuffers(model.buffers.size(), vbos.data());
  for (int i=0; i<model.buffers.size(); i++) {
    std::cout << "  - Creating Buffer " << i << ": " << model.buffers[i].name << "\n";
    std::cout << "       - size: " << model.buffers[i].data.size() << "\n";

    glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
    glBufferData(GL_ARRAY_BUFFER, model.buffers[i].data.size(), model.buffers[i].data.data(),
        GL_STATIC_DRAW);
    model.buffers[i].data.clear();
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // BufferViews.
  for (int i=0; i<model.bufferViews.size(); i++) {
    std::cout << "  - BufferView " << i << ": " << model.bufferViews[i].name << "\n";
    std::cout << "       - parent: " << model.bufferViews[i].buffer << "\n";
    std::cout << "       - offset: " << model.bufferViews[i].byteOffset << "\n";
    std::cout << "       - length: " << model.bufferViews[i].byteLength << "\n";
    std::cout << "       - stride: " << model.bufferViews[i].byteStride << "\n";
    std::cout << "       - target: " << int_to_glfw_type(model.bufferViews[i].target) << "\n";
  }
  // Accessors.
  for (int i=0; i<model.accessors.size(); i++) {
    std::cout << "  - Accessor " << i << ": " << model.accessors[i].name << "\n";
    std::cout << "       - parent  : " << model.accessors[i].bufferView << "\n";
    std::cout << "       - count   : " << model.accessors[i].count << "\n";
    std::cout << "       - offset  : " << model.accessors[i].byteOffset << "\n";
    std::cout << "       - compType: " << int_to_glfw_type(model.accessors[i].componentType) << "\n";
    std::cout << "       - type    : " << int_to_glfw_type(model.accessors[i].type) << "\n";
  }

  // Create Textures.
}



void GltfNode::render(SceneGraphTraversal& sgt) {
  std::cout << "rendering " << model.meshes.size() << " GlTF meshes.\n";

  for (const auto& mesh : model.meshes) {
    for (const auto& prim : mesh.primitives) {
      //auto bufView = acc.
      int draw_count = 0;
      for (const auto key_accid : prim.attributes) {
        auto acc = model.accessors[key_accid.second];
        auto bv = model.bufferViews[acc.bufferView];
        glBindBuffer(GL_ARRAY_BUFFER, vbos[bv.buffer]);

        if (key_accid.first == "POSITION") {
          glEnableClientState(GL_VERTEX_ARRAY);
          // Obviously do not use '3', nor type: should get from gltf.
          glVertexPointer(3, GL_FLOAT, bv.byteStride, (void*) (bv.byteOffset+acc.byteOffset));
          //glVertexPointer(acc.type, acc.componentType, bv.byteStride, (void*) (bv.byteOffset+acc.byteOffset));
          draw_count = acc.count;
          std::cout << " - Setting vertex pointer with cnt: " << acc.count << " off: " << bv.byteOffset+acc.byteOffset
                    << " stride: " << bv.byteStride << " ctype: " << acc.componentType
                    << " type: " <<acc.type << "\n";
        }
      }

      if (prim.indices >= 0) {
        auto ind_acc = model.accessors[prim.indices];
        auto ind_bv = model.bufferViews[ind_acc.bufferView];
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[ind_bv.buffer]);
        glIndexPointer(ind_acc.componentType, ind_bv.byteStride, (void*) (ind_bv.byteOffset+ind_acc.byteOffset));
        std::cout << " - Drawing " << ind_acc.count << " indices.\n";
        glDrawElements(prim.mode, ind_acc.count, ind_acc.componentType, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      } else {
        //glDrawArrays
        std::cout << " - Drawing " << draw_count << " arrays.\n";
      }

      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      glDisableClientState(GL_NORMAL_ARRAY);
      glDisableClientState(GL_COLOR_ARRAY);
    }
  }
}
