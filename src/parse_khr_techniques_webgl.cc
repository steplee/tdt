#include "parse_khr_techniques_webgl.h"
#include <cassert>
#include <iostream>

bool linkShader(GLuint &prog, GLuint &vertShader, GLuint &fragShader) {
  GLint val = 0;

  assert(prog == 0);

  prog = glCreateProgram();

  glAttachShader(prog, vertShader);
  glAttachShader(prog, fragShader);
  glLinkProgram(prog);

  glGetProgramiv(prog, GL_LINK_STATUS, &val);
  assert(val == GL_TRUE && "failed to link shader");
  if (val != GL_TRUE)
    exit(1);

  return true;
}

static bool compileShaderFromBytes(GLenum shaderType, GLuint &shader, const char* buf, int len) {
  GLint val = 0;

  assert (shader == 0);

  shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &buf, &len);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &val);
  if (val != GL_TRUE) {
    char log[4096];
    GLsizei msglen;
    glGetShaderInfoLog(shader, 4096, &msglen, log);
    //spdlog::warn("Shader '{}' compilation failed!", shaderSourceFilename);
    //spdlog::info("Shader log: {}", log);
    std::cerr << " Shader failed to compile! " << std::endl;
    std::cerr << "    Log: \n" << log << std::endl;
    exit(1);
  }

  return true;
}

KhrTechniquesWebGL::KhrTechniquesWebGL() {
}
KhrTechniquesWebGL::~KhrTechniquesWebGL() {
  std::cout << "KhrTechniquesWebGL destructor called, erasing all shaders/programs.\n";
  for (auto shader  : shaders ) glDeleteShader(shader);
  for (auto program : programs) glDeleteProgram(program);
}

void KhrTechniquesWebGL::addShadersFrom(tinygltf::Model& model) {
  if (std::find(model.extensionsUsed.begin(), model.extensionsUsed.end(),
        "KHR_techniques_webgl") != model.extensionsUsed.end()) {

    auto ktw = model.extensions["KHR_techniques_webgl"];

    // 1. Compile each shader.
    // 2. Link each program.
    // 3. Fill in technique fields.
    //
    // Unforunately tinygltf does not use nlohmann::json to store extensions,
    // but this ugly and inefficient 'Value' class with a clunky api.

    // (1)
    int n_shaders = ktw.Get("shaders").Size();
    for (int i=0; i<n_shaders; i++) {
      auto &shader_spec = ktw.Get("shaders").Get(i);
      GLuint shader_id;
      const char* buf;
      int len;
      if (shader_spec.Has("bufferView")) {
        auto &bufView = model.bufferViews[shader_spec.Get("bufferView").GetNumberAsInt()];
        auto &buffer = model.buffers[bufView.buffer];
        buf = ((char*)buffer.data.data()) + bufView.byteOffset;
        len = bufView.byteLength;
      } else if (shader_spec.Has("uri")) {
        // TODO load file, get bytes and length.
        assert(false);
      }
      bool stat = compileShaderFromBytes(shader_spec.Get("type").GetNumberAsInt(), shader_id, buf, len);
      assert(stat);
      shaders.push_back(shader_id);
    }

    // (2)
    int n_programs = ktw.Get("programs").Size();
    for (int i=0; i<n_programs; i++) {
      auto &program_spec = ktw.Get("programs").Get(i);
      GLuint fs = program_spec.Get("fragmentShader").GetNumberAsInt();
      GLuint vs = program_spec.Get("vertexShader").GetNumberAsInt();
      GLuint program_id;
      bool stat = linkShader(program_id, vs, fs);
      assert(stat);
      programs.push_back(program_id);
    }

    // (3)
    int n_techniques = ktw.Get("techniques").Size();
    for (int i=0; i<n_techniques; i++) {
      auto &ts = ktw.Get("programs").Get(i);
      KTWTechnique tech;
      tech.program = ts.Get("program").GetNumberAsInt();

      int n_attrib = ts.Get("attributes").Size();
      auto attr_keys = ts.Get("attributes").Keys();
      for (int j=0; j<n_attrib; j++) {
        std::string name = attr_keys[j];
        std::string semantic = ts.Get("attributes").Get(j).Get("semantic").Get<std::string>();
        tech.attributes[name] = KTWTechnique::Attribute { semantic };
      }

      int n_uniforms = ts.Get("uniforms").Size();
      auto uni_keys = ts.Get("uniforms").Keys();
      for (int j=0; j<n_uniforms; j++) {
        KTWTechnique::Uniform uni;
        std::string name = uni_keys[j];
        auto uni_spec = ts.Get("uniforms").Get(j);
        uni.type = uni_spec.Has("type") ? uni_spec.Get("type").GetNumberAsInt() : 0;
        uni.semantic = uni_spec.Has("semantic") ? uni_spec.Get("semantic").Get<std::string>() : "";
        if (uni_spec.Has("value")) {
          if (uni_spec.Get("value").IsObject()) {
            uni.value_index = uni_spec.Get("value").Get("index").GetNumberAsInt();
          } else {
            auto v = uni_spec.Get("value");
            int nv = v.Size();
            for (int k=0;k<nv;k++) uni.value_default.push_back(v.Get(k).Get<double>());
          }
        }
        tech.uniforms[name] = uni;
      }

      techniques.push_back(tech);
    }

  }
}

std::string KTWTechnique::toString() {
  std::string out;
  out += "Technique '"+name+"':\n";
  out += "  - Attributes (" + std::to_string(attributes.size()) + "):\n";
  for (const auto& kv : attributes) {
  out += "       - " + kv.first + ": " + kv.second.semantic;
  }
  out += "  - Uniforms (" + std::to_string(uniforms.size()) + "):\n";
  for (const auto& kv : uniforms) {
  out += "       - " + kv.first + ": " + kv.second.semantic + " (" + std::to_string(kv.second.type) + ")\n";
  }
  out += "\n";
  return out;
}
