
#include <cstdio>
#include <SDL_image.h>

#include "opengl.h"

// --------------------------------

void glCheckError()
{
  GLenum err = glGetError();
  if(err != GL_NO_ERROR) { printf("Error: %d\n", err); }
}

// --------------------------------

GLuint loadShader(GLenum _type, const char* _source)
{
  GLuint shader_id = glCreateShader(_type);

  glShaderSource(shader_id, 1, &_source, nullptr);

  glCompileShader(shader_id);

  int ok;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &ok);

  if(ok != GL_TRUE)
  {
    int info_log_length = 0;
    int source_length = 0;

    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
    glGetShaderiv(shader_id, GL_SHADER_SOURCE_LENGTH, &source_length);

    if(info_log_length > 1)
    {
      char info_log[info_log_length];

      glGetShaderInfoLog(shader_id, info_log_length, nullptr, info_log);

      printf("GLSL Shader Info Log:\n%s\n", info_log);
    }

    if(source_length > 1)
    {
      char source[source_length];

      glGetShaderSource(shader_id, source_length, nullptr, source);

      printf("GLSL Shader Source:\n%s\n", source);
    }

    return 0;
  }

  return shader_id;
}

// --------------------------------

GLuint createProgram(const char* _vertex_shader_source, const char* _fragment_shader_source)
{
  GLuint vertex_shader_id = loadShader(GL_VERTEX_SHADER, _vertex_shader_source);
  if(!vertex_shader_id) { return 0; }

  GLuint fragment_shader_id = loadShader(GL_FRAGMENT_SHADER, _fragment_shader_source);
  if(!fragment_shader_id) { return 0; }

  GLuint program_id = glCreateProgram();

  glAttachShader(program_id, vertex_shader_id);
  glAttachShader(program_id, fragment_shader_id);

  glBindAttribLocation(program_id, VertexAttribute::POSITION, "position");
  glBindAttribLocation(program_id, VertexAttribute::UV, "uv");

  glLinkProgram(program_id);

  int ok;
  glGetProgramiv(program_id, GL_LINK_STATUS, &ok);

  if(ok != GL_TRUE)
  {
    int info_log_length = 0;
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);

    if(info_log_length > 1)
    {
      char info_log[info_log_length];

      glGetProgramInfoLog(program_id, info_log_length, nullptr, info_log);

      printf("GLSL Program Info Log:\n%s\n", info_log);
    }

    return 0;
  }

  return program_id;
}

// --------------------------------

GLuint loadTexture(const char* _filename)
{
  GLuint texture_id = 0;

  SDL_Surface *image = IMG_Load(_filename);

  if(!image)
  {
    // Create a fallback gray texture
    printf("Failed to load %s, due to %s\n", _filename, IMG_GetError());
    const int w = 128, h = 128, bitsPerPixel = 24;
    image = SDL_CreateRGBSurface(0, w, h, bitsPerPixel, 0, 0, 0, 0);
    if(image)
      memset(image->pixels, 0x42, image->w * image->h * bitsPerPixel / 8);
  }

  if(image)
  {
    int bitsPerPixel = image->format->BitsPerPixel;
    printf ("Image dimensions %dx%d, %d bits per pixel\n", image->w, image->h, bitsPerPixel);

    // Determine GL texture format
    GLint format = -1;
    if(bitsPerPixel == 24)
    {
      format = GL_RGB;
    }
    else if(bitsPerPixel == 32)
    {
      format = GL_RGBA;
    }
    else
    {
      SDL_Surface *output = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_RGB24, 0);
      SDL_FreeSurface(image);
      image = output;
      format = GL_RGB;
    }

    if(format != -1)
    {
      glGenTextures(1, &texture_id);

      glBindTexture(GL_TEXTURE_2D, texture_id);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glTexImage2D(GL_TEXTURE_2D, 0, format, image->w, image->h, 0, format, GL_UNSIGNED_BYTE, image->pixels);
    }

    SDL_FreeSurface (image);
  }                       

  return texture_id;
}

// --------------------------------

GLuint createTexture(int _width, int _height, const unsigned char* _data, GLint _internal_format, GLenum _format, GLenum _type)
{
  GLuint texture_id = 0;

  glGenTextures(1, &texture_id);

  glBindTexture(GL_TEXTURE_2D, texture_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, _internal_format, _width, _height, 0, _format, _type, _data);

  return texture_id;
}

// --------------------------------

