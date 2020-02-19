#pragma once

#include <GL/glew.h>
//#define GLFW_INCLUDE_GLU
//#include <GLFW/glfw3.h>

#include <string>
#include <unordered_map>
#include <unordered_map>
#include <memory>

namespace ProgramCache {

  struct Program {
    GLuint id            = 0;

    GLint attribPos      = -1;
    GLint attribNormal   = -1;
    GLint attribTexCoord = -1;
    GLint attribExtra1   = -1;
    GLint attribExtra2   = -1;

    GLint uniformMVP     = -1;

    inline void bind() { glUseProgram(id); }
    inline void unbind() { glUseProgram(0); }

    ~Program();

  };

  std::shared_ptr<Program> getShader(std::string name, std::string vertPath, std::string fragPath);
  std::shared_ptr<Program> getShader(std::string name);

  bool LoadShader(GLenum shaderType, GLuint &shader, const char *shaderSourceFilename);
  bool LinkShader(GLuint &prog, GLuint &vertShader, GLuint &fragShader);
  bool getAttribs(Program& prog);

  extern std::unordered_map<std::string, std::shared_ptr<Program>> programCache;

}
