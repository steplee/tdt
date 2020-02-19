#include "src/program_cache.h"
#include <cassert>
#include <iostream>
#include <vector>
//#include <spdlog/spdlog.h>

namespace ProgramCache {

std::unordered_map<std::string, std::shared_ptr<Program>> programCache;

Program::~Program() {
  glDeleteProgram(id);
  id = 0;
}

std::shared_ptr<Program> getShader(std::string name, std::string vertPath, std::string fragPath) {
  if (programCache.find(name) == programCache.end()) {
    //spdlog::info(" Creating shader '{}' from {} & {}.", name, vertPath, fragPath);
    auto program = std::make_shared<Program>();

    GLuint vertId = 0, fragId = 0;
    LoadShader(GL_VERTEX_SHADER, vertId, vertPath.c_str());
    LoadShader(GL_FRAGMENT_SHADER, fragId, fragPath.c_str());
    LinkShader(program->id, vertId, fragId);

    getAttribs(*program);

    programCache[name] = program;
    return programCache[name];

  } else {
    //spdlog::trace("Cache hit for shader '{}'.", name);
    return programCache[name];
  }
}

std::shared_ptr<Program> getShader(std::string name) {
  if (programCache.find(name) == programCache.end()) {
    std::cerr << " Failed to retrieve program: " << name << std::endl;
    exit(1);
  }
  //spdlog::trace("Cache hit for shader '{}'.", name);
  return programCache[name];
}

bool getAttribs(Program& p) {
  p.attribPos = glGetAttribLocation(p.id, "in_pos");
  p.attribTexCoord = glGetAttribLocation(p.id, "in_uv");
  p.attribNormal = glGetAttribLocation(p.id, "in_normal");

  p.uniformMVP = glGetUniformLocation(p.id, "mvp");

  return true;
}

bool LinkShader(GLuint &prog, GLuint &vertShader, GLuint &fragShader) {
  GLint val = 0;

  if (prog != 0) {
    glDeleteProgram(prog);
  }

  prog = glCreateProgram();

  glAttachShader(prog, vertShader);
  glAttachShader(prog, fragShader);
  glLinkProgram(prog);

  glGetProgramiv(prog, GL_LINK_STATUS, &val);
  assert(val == GL_TRUE && "failed to link shader");
  if (val != GL_TRUE)
    exit(1);

  //spdlog::info("Link shader OK.");

  return true;
}

bool LoadShader(GLenum shaderType, GLuint &shader, const char *shaderSourceFilename) {
  GLint val = 0;

  // free old shader/program
  if (shader != 0) {
    glDeleteShader(shader);
  }

  std::vector<GLchar> srcbuf;
  FILE *fp = fopen(shaderSourceFilename, "rb");
  if (!fp) {
    std::cerr << " Failed to open shader file: " << shaderSourceFilename << std::endl;
    exit(1);
    return false;
  }
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  rewind(fp);
  srcbuf.resize(len + 1);
  len = fread(&srcbuf.at(0), 1, len, fp);
  srcbuf[len] = 0;
  fclose(fp);

  const GLchar *srcs[1];
  srcs[0] = &srcbuf.at(0);

  shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, srcs, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &val);
  if (val != GL_TRUE) {
    char log[4096];
    GLsizei msglen;
    glGetShaderInfoLog(shader, 4096, &msglen, log);
    //spdlog::warn("Shader '{}' compilation failed!", shaderSourceFilename);
    //spdlog::info("Shader log: {}", log);
    std::cerr << " Shader failed to compile: " << shaderSourceFilename << std::endl;
    exit(1);
  }

  //spdlog::trace("Load shader '{}' OK", shaderSourceFilename);
}

}
