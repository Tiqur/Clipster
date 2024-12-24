#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include <string>
#include <GL/glew.h>

GLuint createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);

#endif

