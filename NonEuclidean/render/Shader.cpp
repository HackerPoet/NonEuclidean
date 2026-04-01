#include "Shader.h"
#include <fstream>
#include <sstream>

Shader::Shader(const char* name) {
  //Get the file paths
  const std::string vert = "Shaders/" + std::string(name) + ".vert";
  const std::string frag = "Shaders/" + std::string(name) + ".frag";

  //Load the shaders from disk
  vertId = LoadShader(vert.c_str(), GL_VERTEX_SHADER);
  fragId = LoadShader(frag.c_str(), GL_FRAGMENT_SHADER);

  //Create the program
  progId = glCreateProgram();
  glAttachShader(progId, vertId);
  glAttachShader(progId, fragId);

  //Bind variables
  for (size_t i = 0; i < attribs.size(); ++i) {
    glBindAttribLocation(progId, (GLuint)i, attribs[i].c_str());
  }

  //Link the program
  glLinkProgram(progId);

  //Check for linking errors
  GLint isLinked;
  glGetProgramiv(progId, GL_LINK_STATUS, &isLinked);
  if (!isLinked) {
    GLint logLength;
    glGetProgramiv(progId, GL_INFO_LOG_LENGTH, &logLength);

    std::vector<GLchar> log;
    log.resize(logLength);
    glGetProgramInfoLog(progId, logLength, &logLength, log.data());

    std::ofstream fout(std::string(vert) + ".link.log");
    fout.write(log.data(), logLength);

    progId = 0;
    return;
  }

  //Get global variable locations
  mvpId = glGetUniformLocation(progId, "mvp");
  mvId = glGetUniformLocation(progId, "mv");
}

Shader::~Shader() {
  glDetachShader(progId, vertId);
  glDetachShader(progId, fragId);
  glDeleteProgram(progId);
  glDeleteShader(vertId);
  glDeleteShader(fragId);
}

void Shader::Use() {
  glUseProgram(progId);
}

GLuint Shader::LoadShader(const char* fname, GLenum type) {
  //Read shader source from disk
  std::ifstream fin(fname);
  std::stringstream buff;
  buff << fin.rdbuf();
  const std::string str = buff.str();
  const char* source = str.c_str();

  //Create and compile shader
  const GLuint id = glCreateShader(type);
  glShaderSource(id, 1, (const GLchar**)&source, 0);
  glCompileShader(id);

  //Check to make sure there were no errors
  GLint isCompiled = 0;
  glGetShaderiv(id, GL_COMPILE_STATUS, &isCompiled);
  if (!isCompiled) {
    GLint logLength;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);

    std::vector<GLchar> log;
    log.resize(logLength);
    glGetShaderInfoLog(id, logLength, &logLength, log.data());

    std::ofstream fout(std::string(fname) + ".log");
    fout.write(log.data(), logLength);
    return 0;
  }

  //Save variable bindings
  if (type == GL_VERTEX_SHADER) {
    size_t ix = 0;
    while (true) {
      ix = str.find("\nin ", ix);
      if (ix == std::string::npos) {
        break;
      }
      ix = str.find(";", ix);
      size_t start_ix = ix;
      while (str[--start_ix] != ' ');
      attribs.push_back(str.substr(start_ix + 1, ix - start_ix - 1));
    }
  }

  //Return the shader id
  return id;
}

void Shader::SetMVP(const float* mvp, const float* mv) {
  if (mvp) glUniformMatrix4fv(mvpId, 1, GL_TRUE, mvp);
  if (mv) glUniformMatrix4fv(mvId, 1, GL_TRUE, mv);
}
