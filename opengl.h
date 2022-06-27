#ifndef _opengl_h_
#define _opengl_h_

#include <GLES3/gl3.h>

void glCheckError();

GLuint createProgram(const char* _vertex_shader_source, const char* _fragment_shader_source);

GLuint loadTexture(const char* _filename);

GLuint createTexture(int _width, int _height, const unsigned char* _data, GLint _internal_format = GL_RGBA8, GLenum _format = GL_RGBA, GLenum _type = GL_UNSIGNED_BYTE);

#endif
