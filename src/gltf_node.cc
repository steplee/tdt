#define TINYGLTF_IMPLEMENTATION
#include "gltf_node.h"

#include "program_cache.h"

#include <iostream>


#include <cstdlib>

#define USE_MIPMAPS 1

using namespace tinygltf;

static int filter_without_mipmap(int f) {
  if (f == GL_NEAREST_MIPMAP_NEAREST or f == GL_NEAREST_MIPMAP_LINEAR) return GL_NEAREST;
  if (f == GL_LINEAR_MIPMAP_NEAREST or f == GL_LINEAR_MIPMAP_LINEAR) return GL_LINEAR;
  return f;
}
static bool filter_uses_mipmap(int f) {
  return f == GL_NEAREST_MIPMAP_NEAREST or GL_LINEAR_MIPMAP_NEAREST or GL_NEAREST_MIPMAP_LINEAR or GL_LINEAR_MIPMAP_LINEAR;
}

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

  program_to_use = ProgramCache::getShader("simple_textured", "../shaders/simple_textured.vert", "../shaders/simple_textured.frag");

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

GltfNode::~GltfNode() {
  glDeleteBuffers(vbos.size(), vbos.data());
  glDeleteTextures(textures.size(), textures.data());
  vbos.clear();
  textures.clear();
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

}



/*
 * This function is for fixed pipeline OpenGL, and I didn't even apply sgt.mvp.
 *
void GltfNode::render(SceneGraphTraversal& sgt) {
  std::cout << "rendering " << model.meshes.size() << " GlTF meshes.\n";
  glColor4f(1,1,1,1);

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
          glVertexPointer(acc.type, acc.componentType, bv.byteStride, (void*) (bv.byteOffset+acc.byteOffset));
          draw_count = acc.count;
          std::cout << " - Setting vertex pointer with cnt: " << acc.count << " off: " << bv.byteOffset+acc.byteOffset
                    << " stride: " << bv.byteStride << " ctype: " << acc.componentType
                    << " type: " <<acc.type << "\n";
        }

        if (key_accid.first == "TEXCOORD_0") {
          glEnableClientState(GL_TEXTURE_COORD_ARRAY);
          glTexCoordPointer(acc.type, acc.componentType, bv.byteStride, (void*) (bv.byteOffset+acc.byteOffset));
          std::cout << " - Setting texcoord pointer with cnt: " << acc.count << " off: " << bv.byteOffset+acc.byteOffset
                    << " stride: " << bv.byteStride << " ctype: " << acc.componentType
                    << " type: " <<acc.type << "\n";
        }

        if (key_accid.first == "NORMAL") {
          glEnableClientState(GL_NORMAL_ARRAY);
          glNormalPointer(acc.componentType, bv.byteStride, (void*) (bv.byteOffset+acc.byteOffset));
          std::cout << " - Setting normal pointer with cnt: " << acc.count << " off: " << bv.byteOffset+acc.byteOffset
                    << " stride: " << bv.byteStride << " ctype: " << acc.componentType
                    << " type: " <<acc.type << "\n";
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
      } else glUseProgram(0);

      if (prim.indices >= 0) {
        auto ind_acc = model.accessors[prim.indices];
        auto ind_bv = model.bufferViews[ind_acc.bufferView];
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[ind_bv.buffer]);
        glIndexPointer(ind_acc.componentType, ind_bv.byteStride, (void*) (ind_bv.byteOffset+ind_acc.byteOffset));
        std::cout << " - Drawing " << ind_acc.count << " indices.\n";
        glDrawElements(prim.mode, ind_acc.count, ind_acc.componentType, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      } else {
        std::cout << " - Drawing " << draw_count << " arrays.\n";
        glDrawArrays(prim.mode, 0, draw_count);
      }

      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      glDisableClientState(GL_NORMAL_ARRAY);
      glDisableClientState(GL_COLOR_ARRAY);
      glDisable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }
}
*/

// This impl uses an OpenGL 2.0+ shader pipeline
void GltfNode::render(SceneGraphTraversal& sgt) {
  std::cout << "rendering " << model.meshes.size() << " GlTF meshes.\n";
  glColor4f(1,1,1,1);

  assert(program_to_use != nullptr);
  glUseProgram(program_to_use->id);

  // Bind global uniforms.
  auto mvp = sgt.mvp.transpose();
  if (program_to_use->uniformMVP < 0) {
    std::cerr << " - MVP uniform was not found!\n";
  }
  glUniformMatrix4fv(program_to_use->uniformMVP, 1, false, mvp.data());

  for (const auto& mesh : model.meshes) {
    for (const auto& prim : mesh.primitives) {
      //auto bufView = acc.
      int draw_count = 0;
      for (const auto key_accid : prim.attributes) {
        auto acc = model.accessors[key_accid.second];
        auto bv = model.bufferViews[acc.bufferView];
        glBindBuffer(GL_ARRAY_BUFFER, vbos[bv.buffer]);

        int target = -2;

        if (key_accid.first == "POSITION")
          target = program_to_use->attribPos;
        if (key_accid.first == "TEXCOORD_0")
          target = program_to_use->attribTexCoord;
        if (key_accid.first == "NORMAL")
          target = program_to_use->attribNormal;

        if (target == -2) {
          std::cerr << " Unknown vertex attrib: " << key_accid.first << "\n";
        } else if (target == -1) {
          std::cerr << " Vertex attrib was not bound: " << key_accid.first << "\n";
        } else {
          glEnableVertexAttribArray(target);
          glVertexAttribPointer(target, acc.type, acc.componentType, acc.normalized, bv.byteStride, (void*) (bv.byteOffset+acc.byteOffset));
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

      if (prim.indices >= 0) {
        auto ind_acc = model.accessors[prim.indices];
        auto ind_bv = model.bufferViews[ind_acc.bufferView];
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[ind_bv.buffer]);
        glIndexPointer(ind_acc.componentType, ind_bv.byteStride, (void*) (ind_bv.byteOffset+ind_acc.byteOffset));
        std::cout << " - Drawing " << ind_acc.count << " indices.\n";
        glDrawElements(prim.mode, ind_acc.count, ind_acc.componentType, 0);
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

  glUseProgram(0);
}
