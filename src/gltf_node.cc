#define TINYGLTF_IMPLEMENTATION
#include "gltf_node.h"

#include "program_cache.h"
#include "scene_graph.h"

#include <iostream>


#include <cstdlib>

#define USE_MIPMAPS 1

using namespace tinygltf;

#include <endian.h>
#include <arpa/inet.h>
#include <byteswap.h>
template<class T> T view_little(const std::vector<uint8_t>& v, const size_t& byte) {
  /*
#if __BYTE_ORDER != __LITTLE_ENDIAN
  if (std::is_same<std::make_unsigned<T>, std::int16_t>::value) {
    return static_cast<T>(bswap_16(*reinterpret_cast<typename std::add_pointer<std::make_unsigned<T>>::type>(((char*)v.data())+byte)));
  }
  if (std::is_same<std::make_unsigned<T>, std::int32_t>::value) {
    return static_cast<T>(bswap_32(*reinterpret_cast<typename std::add_pointer<std::make_unsigned<T>>::type>(((char*)v.data())+byte)));
  }
  if (std::is_same<std::make_unsigned<T>, std::int64_t>::value) {
    return static_cast<T>(bswap_64(*reinterpret_cast<typename std::add_pointer<std::make_unsigned<T>>::type>(((char*)v.data())+byte)));
  }
#endif
  return *reinterpret_cast<T*>(((char*)v.data())+byte);
  */
#if __BYTE_ORDER != __LITTLE_ENDIAN
  if (std::is_same<T, std::int32_t>::value or std::is_same<T, std::uint32_t>::value) {
    uint32_t z = *reinterpret_cast<uint32_t*> ( ((char*)v.data())+byte );
    return bswap_32(z);
  }
  if (std::is_same<T, std::int16_t>::value or std::is_same<T, std::uint16_t>::value) {
    uint16_t z = *reinterpret_cast<uint16_t*> ( ((char*)v.data())+byte );
    return bswap_16(z);
  }
#endif
  T z = *reinterpret_cast<T*> ( ((char*)v.data())+byte );
}

static int filter_without_mipmap(int f) {
  if (f == GL_NEAREST_MIPMAP_NEAREST or f == GL_NEAREST_MIPMAP_LINEAR) return GL_NEAREST;
  if (f == GL_LINEAR_MIPMAP_NEAREST or f == GL_LINEAR_MIPMAP_LINEAR) return GL_LINEAR;
  return f;
}
static bool filter_uses_mipmap(int f) {
  return f == GL_NEAREST_MIPMAP_NEAREST or GL_LINEAR_MIPMAP_NEAREST or GL_NEAREST_MIPMAP_LINEAR or GL_LINEAR_MIPMAP_LINEAR;
}

static std::string int_to_glfw_type(int t) {
 if (t ==  (2)) return "TINYGLTF_TYPE_VEC2";
 if (t ==  (3)) return "TINYGLTF_TYPE_VEC3";
 if (t ==  (4)) return "TINYGLTF_TYPE_VEC4";
 if (t ==  32+ 2) return "TINYGLTF_TYPE_MAT2";
 if (t ==  32+ 3) return "TINYGLTF_TYPE_MAT3";
 if (t ==  32+ 4) return "TINYGLTF_TYPE_MAT4";
 if (t ==  64+ 1) return "TINYGLTF_TYPE_SCALAR";
 if (t ==  64+ 4) return "TINYGLTF_TYPE_VECTOR";
 if (t ==  64+ 16) return "TINYGLTF_TYPE_MATRIX";
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

GltfModel::GltfModel() {}
GltfModel::GltfModel(tinygltf::Model& model) : model(model) {
  setup();
}
GltfModel::GltfModel(const std::string& path)
{
  TinyGLTF loader;
  std::string err, warn;
  bool good = false;
  if (path.find("glb") == std::string::npos)
    good = loader.LoadASCIIFromFile(&model, &err, &warn, path);
  else
    good = loader.LoadBinaryFromFile(&model, &err, &warn, path);
  if (err.length()) std::cout << " - GltfNode load err : " << err << "\n";
  if (warn.length()) std::cout << " - GltfNode load warn: " << warn << "\n";

  if (good) {
    setup();
  } else {
    std::cerr << " - Failed to load Gltf: " << path << "\n";
    exit(1);
  }
}

GltfModel::~GltfModel() {
  // TODO: make it move only so this is valid.
  //glDeleteBuffers(vbos.size(), vbos.data());
  //glDeleteTextures(textures.size(), textures.data());
  //vbos.clear();
  //textures.clear();
}


void GltfModel::setup(/*tinygltf::Model& model*/) {
  program_to_use = ProgramCache::getShader("simple_textured", "./shaders/simple_textured.vert", "./shaders/simple_textured.frag");
  //program_to_use = ProgramCache::getShader("simplest", "./shaders/simplest.vert", "./shaders/simplest.frag");

  // Create VBOs.
  vbos.resize(model.buffers.size());
  glGenBuffers(model.buffers.size(), vbos.data());
  for (int i=0; i<model.buffers.size(); i++) {
    std::cout << "  - Creating Buffer " << i << ": " << model.buffers[i].name << "\n";
    std::cout << "       - size: " << model.buffers[i].data.size() << "\n";

    glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
    glBufferData(GL_ARRAY_BUFFER, model.buffers[i].data.size(), model.buffers[i].data.data(),
        GL_STATIC_DRAW);
    //model.buffers[i].data.clear();
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // BufferViews.
  //vbos.resize(model.bufferViews.size());
  //glGenBuffers(model.bufferViews.size(), vbos.data());
  for (int i=0; i<model.bufferViews.size(); i++) {
    std::cout << "  - BufferView " << i << ": " << model.bufferViews[i].name << "\n";
    std::cout << "       - parent: " << model.bufferViews[i].buffer << "\n";
    std::cout << "       - offset: " << model.bufferViews[i].byteOffset << "\n";
    std::cout << "       - length: " << model.bufferViews[i].byteLength << "\n";
    std::cout << "       - stride: " << model.bufferViews[i].byteStride << "\n";
    std::cout << std::hex;
    std::cout << "       - target: " << (model.bufferViews[i].target) << "\n";
    std::cout << std::dec;
    std::cout << "       - sample:\n";
  }
  // Accessors.
  for (int i=0; i<model.accessors.size(); i++) {
    std::cout << "  - Accessor " << i << ": " << model.accessors[i].name << "\n";
    std::cout << "       - parent  : " << model.accessors[i].bufferView << "\n";
    std::cout << "       - count   : " << model.accessors[i].count << "\n";
    std::cout << "       - offset  : " << model.accessors[i].byteOffset << "\n";
    std::cout << std::hex;
    std::cout << "       - compType: " << (model.accessors[i].componentType) << "\n";
    std::cout << "       - type    : " << (model.accessors[i].type) << "\n";
    std::cout << std::dec;
    std::cout << "       - looks like:\n";
    for (int j=0; j<4; j++) {
      std::cout << "            - ";
      int len=0;
      for (int l=0; l<len; l++) {
        //if (model.accessors[i].componentType < 0x1400) 
          //std::cout << view_little<uint8_t>(
      }
      std::cout << "\n";

    }
  }

  // Create Textures.
  textures.resize(model.textures.size());
  glGenTextures(textures.size(), textures.data());
  for (int i=0; i<model.textures.size(); i++) {
    int src = model.textures[i].source;
    auto& img = model.images[src];
    auto& sampler = model.samplers[model.textures[i].sampler];
    std::cout << "  - Creating Texture " << i << ": " << model.textures[i].name << "\n";
    std::cout << "       - source: " << src << "\n";
    std::cout << "            + size: " << img.width << " " << img.height << "\n";
    std::cout << "            + uri: " << img.uri << " bufferView: " << img.bufferView << "\n";

    if (img.image.empty()) {
      std::cout << " - We don't allow image buffers, it must be URI right now." << std::endl;
      exit(1);
    }

    glActiveTexture(GL_TEXTURE0); // Note: when binding for renders this will change!
    glBindTexture(GL_TEXTURE_2D, textures[i]);

    int min_filter = sampler.minFilter == -1 ? GL_NEAREST_MIPMAP_LINEAR : sampler.minFilter;
    int mag_filter = sampler.magFilter == -1 ? GL_LINEAR : sampler.magFilter;
    if (not USE_MIPMAPS) min_filter = filter_without_mipmap(min_filter);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // I've seen a (contested) statement that glTexStorage+glTexSubImage allows optimizations over glTexImage.
    std::cout << "image component: " << (img.component) << " type: " << int_to_glfw_type(img.pixel_type) << "\n";
    int format   = img.component == 1 ? GL_LUMINANCE  : img.component == 3 ? GL_RGB  : img.component == 4 ? GL_RGBA  : -1;
    int informat = img.component == 1 ? GL_LUMINANCE8 : img.component == 3 ? GL_RGB8 : img.component == 4 ? GL_RGBA8 : -1;
    if (format == -1) { std::cerr << " - Invalid texture format." << std::endl; exit(1); }
    int lvls = log2(std::min(img.width,img.height));
    //glTexStorage2D(GL_TEXTURE_2D, lvls, informat, img.width, img.height);
    //glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, img.width, img.height, format, GL_UNSIGNED_BYTE, img.image.data());
    glTexImage2D(GL_TEXTURE_2D,  0, format, img.width, img.height, 0, format, GL_UNSIGNED_BYTE, img.image.data());
    if (filter_uses_mipmap(min_filter)) glGenerateMipmap(GL_TEXTURE_2D);
    // TODO mip-map.
  }
  glBindTexture(GL_TEXTURE_2D, 0);

  //exit(0);

  /*
  std::cout << "some floats.\n";
  for (int i=0; i<model.buffers[0].data.size(); i++) {
    std::cout << view_little<float>(model.buffers[0].data, i*4);
    if (i % 10 == 0) std::cout << "\n";
  }
  */
  //exit(0);

}


Eigen::Matrix4f GltfModel::get_node_xform(int node_id) {
  assert(node_id < model.nodes.size());
  auto& node = model.nodes[node_id];
  Eigen::Matrix4f out(Eigen::Matrix4f::Identity());
  return out;

  if (node.matrix.size()) {
    for (int i=0; i<16; i++) out(i) = node.matrix[i];
  } else {
    if (node.scale.size())
      for (int i=0; i<3; i++) out(i,i) = node.scale[i];
    if (node.rotation.size()) {
      Eigen::Quaternionf q(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
      for (int i=0; i<3; i++) out.topLeftCorner<3,3>() = q.toRotationMatrix() * out.topLeftCorner<3,3>();
    }
    if (node.translation.size()) {
      out.topRightCorner<3,1>() = out.topLeftCorner<3,3>() *
        Eigen::Vector3f { node.translation[0] , node.translation[1] , node.translation[2] };
    }
  }


  std::cout << "Node " << node_id << " xform:\n" << out << "\n";

  return out;
}

void GltfModel::renderScene(const SceneGraphTraversal& sgt, int scene) {
  for (int i=0; i<model.scenes[scene].nodes.size(); i++) {
    renderNode(sgt, model.scenes[scene].nodes[i]);
  }
}

// This impl uses an OpenGL 2.0+ shader pipeline
// First render this node, then it's children.
void GltfModel::renderNode(const SceneGraphTraversal& sgt, int node) {

  glColor4f(1,1,1,1);

  assert(program_to_use != nullptr);
  glUseProgram(program_to_use->id);

  Eigen::Matrix4f node_model_xform = get_node_xform(node);
  Eigen::Matrix4f new_mvp = sgt.mvp * node_model_xform;

  // Bind global uniforms.
  if (program_to_use->uniformMVP < 0) {
    std::cerr << " - MVP uniform was not found!\n";
  }
  glUniformMatrix4fv(program_to_use->uniformMVP, 1, false, new_mvp.data());

  if (model.nodes[node].mesh >= 0) {
    auto &mesh = model.meshes[model.nodes[node].mesh];
    for (const auto& prim : mesh.primitives) {
      //auto bufView = acc.
      int draw_count = 0;
      for (const auto key_accid : prim.attributes) {
        auto &acc = model.accessors[key_accid.second];
        auto &bv = model.bufferViews[acc.bufferView];

        int size = 1;
        if (acc.type == TINYGLTF_TYPE_SCALAR) size = 1;
        else if (acc.type == TINYGLTF_TYPE_VEC2) size = 2;
        else if (acc.type == TINYGLTF_TYPE_VEC3) size = 3;
        else if (acc.type == TINYGLTF_TYPE_VEC4) size = 4;
        else assert(0);

        int target = -2;

        if (key_accid.first == "POSITION")
          target = program_to_use->attribPos;
        if (key_accid.first == "TEXCOORD_0")
          target = program_to_use->attribTexCoord;
        if (key_accid.first == "NORMAL")
          target = program_to_use->attribNormal;

        std::cerr << "  vertex attrib: " << key_accid.first <<  " : " << target << "\n";
        if (target == -2) {
          std::cerr << " Unknown vertex attrib: " << key_accid.first << "\n";
        } else if (target == -1) {
          std::cerr << " Vertex attrib was not bound: " << key_accid.first << "\n";
        } else {
          //auto stride = model.accessors[key_accid.second].ByteStride(bv);
          auto stride = bv.byteStride;
          //glBindBuffer(GL_ARRAY_BUFFER, acc.bufferView);
          //glVertexAttribPointer(target, size, acc.componentType, acc.normalized, stride, (void*) (acc.byteOffset));
          glBindBuffer(GL_ARRAY_BUFFER, vbos[bv.buffer]);
          glVertexAttribPointer(target, size, acc.componentType, acc.normalized, stride, (void*) (bv.byteOffset+acc.byteOffset));
          glEnableVertexAttribArray(target);
        }

        if (key_accid.first == "TANGENT") {
          std::cout << " - We don't support Tangents yet.\n";
        }
      }

      if (prim.material >= 0) {
        auto& mat = model.materials[prim.material];
        auto& pbr = mat.pbrMetallicRoughness;


        if (pbr.baseColorTexture.index >= 0) {
          assert(pbr.baseColorTexture.texCoord == 0);
          auto& color_tex_id = pbr.baseColorTexture.index;
          auto& color_tex = model.textures[color_tex_id];
          std::cout << " - SETTING TEXTURE " << textures[color_tex_id] << "\n";
          glEnable(GL_TEXTURE_2D);
          glActiveTexture(MY_GL_COLOR_TEXTURE);
          glBindTexture(GL_TEXTURE_2D, textures[color_tex_id]);
        }
      }

      glDisable(GL_TEXTURE_2D);

      if (prim.indices >= 0) {
        auto &ind_acc = model.accessors[prim.indices];
        auto &ind_bv = model.bufferViews[ind_acc.bufferView];
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ind_acc.bufferView);
        //glIndexPointer(ind_acc.componentType, ind_bv.byteStride, (void*) (ind_acc.byteOffset));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[ind_bv.buffer]);
        std::cout << " - Drawing " << ind_acc.count << " indices of type " << ind_acc.componentType << " in mode " << prim.mode << "\n";
        //glDrawElements(prim.mode, ind_acc.count, ind_acc.componentType, 0);
        glDrawElements(prim.mode, ind_acc.count, ind_acc.componentType, (void*) (ind_bv.byteOffset+ind_acc.byteOffset));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      } else {
        std::cout << " - Drawing " << draw_count << " arrays.\n";
        glDrawArrays(prim.mode, 0, draw_count);
      }

      glDisableVertexAttribArray(program_to_use->attribPos);
      glDisableVertexAttribArray(program_to_use->attribTexCoord);
      glDisableVertexAttribArray(program_to_use->attribNormal);
      glDisable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }

  // Now recurse on children.
  SceneGraphTraversal lower_sgt(new_mvp, sgt.depth+1);
  for (int ci=0; ci<model.nodes[node].children.size(); ci++) {
    renderNode(lower_sgt, model.nodes[node].children[ci]);
  }


  glUseProgram(0);
}
