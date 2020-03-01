#pragma once

#include "tinygltf/tiny_gltf.h"
#include "tinygltf/json.hpp"

#include "program_cache.h"

/*
 * Caching:
 * I will be loading in many glTF/b3dm files from the same tileset (or multiple)
 * They will probably share similar shaders. They will probably all store them independently,
 * but we want to avoid re-compilation/linking of the same shaders!
 *
 * So I will make the assumption that if a shader/program has a name it is the same as another
 * with the same name from a different file.
 * I can't imagine a glTF/b3dm generator that would violate this.
 *
 * The tricky part will be if a shader/program does not have a name, but is the exact same as another.
 * In that case, I could try hashing the source code and using that as a name.
 *
 *
 *
 *
 * TODO Support some common cesium 'automatic uniforms':
 * https://github.com/CesiumGS/cesium/blob/b20/Source/Renderer/ShaderProgram.js
 * Also note that:
 *        ... Uniforms are program object-specific state. They retain their values once loaded,
 *            and their values are restored whenever a program object is used,
 *            as long as the program object has not been re-linked. ...
 * So if no object ever changes a uniform, we need only set it once.
 *
 */

// TODO.
// Rethink how I bind the current program + attribs/uniforms.
// I might want a higher-level abstraction that an OpenGL program.
// Also keep in mind all standard glTF (non-b3dm) will probably use the same PBR shader.
struct KTWMaterial {

};


struct KTWTechnique {
  struct Attribute {
    std::string semantic;
  };
  struct Uniform {
    int type;
    std::string semantic;
    int value_index=0;                // In case of sampler2D
    std::vector<float> value_default; // In case specified (and not sampler2D)
  };

  std::map<std::string, Attribute> attributes;
  std::map<std::string, Uniform> uniforms;
  std::string name;
  int program;

  std::string toString();
};

struct KhrTechniquesWebGL {
  KhrTechniquesWebGL();
  ~KhrTechniquesWebGL();

  void addShadersFrom(tinygltf::Model& model);

  //std::map<std::string, std::shared_ptr<ProgramCache::Program>> programs;

  std::vector<GLuint> shaders;
  std::vector<GLuint> programs;
  std::vector<KTWTechnique> techniques;
};
