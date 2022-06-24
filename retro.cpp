
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengles2.h>
#include <GLES3/gl3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "font.h"
#include "opengl.h"
#include "shaders.h"

// --------------------------------

bool running = true;

struct Window
{
  SDL_Window* sdl_window = nullptr;
  Uint32 id = 0;

  int    width  = 640;
  int    height = 480;
  float  aspect = 640.0f / 480.0f;
};

Window window;

struct Screen
{
  int   width  = 320;
  int   height = 240;
  float aspect = 320.0f / 240.0f;
  GLuint program = 0;
  GLuint vbo = 0;
  GLuint tex = 0;
};

Screen screen;

// --------------------------------

void updateScreenVBO()
{
  GLfloat vertices[16];

  const GLfloat screen_size = 1.0f;

  if(window.aspect < screen.aspect)
  {
    vertices[0]  =  screen_size;
    vertices[1]  =  screen_size * window.aspect / screen.aspect;
    vertices[2]  =  screen.width;
    vertices[3]  =  0.0f;

    vertices[4]  = -screen_size;
    vertices[5]  =  screen_size * window.aspect / screen.aspect;
    vertices[6]  =  0.0f;
    vertices[7]  =  0.0f;

    vertices[8]  =  screen_size;
    vertices[9]  = -screen_size * window.aspect / screen.aspect;
    vertices[10] =  screen.width;
    vertices[11] =  screen.height;

    vertices[12] = -screen_size;
    vertices[13] = -screen_size * window.aspect / screen.aspect;
    vertices[14] =  0.0f;
    vertices[15] =  screen.height;
  }
  else
  {
    vertices[0]  =  screen_size * screen.aspect / window.aspect;
    vertices[1]  =  screen_size;
    vertices[2]  =  screen.width;
    vertices[3]  =  0.0f;

    vertices[4]  = -screen_size * screen.aspect / window.aspect;
    vertices[5]  =  screen_size;
    vertices[6]  =  0.0f;
    vertices[7]  =  0.0f;

    vertices[8]  =  screen_size * screen.aspect / window.aspect;
    vertices[9]  = -screen_size;
    vertices[10] =  screen.width;
    vertices[11] =  screen.height;

    vertices[12] = -screen_size * screen.aspect / window.aspect;
    vertices[13] = -screen_size;
    vertices[14] =  0.0f;
    vertices[15] =  screen.height;
  }

  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}

// --------------------------------

void resizeWindow(int width, int height)
{
  glViewport(0, 0, width, height);
  window.width = width;
  window.height = height;
  window.aspect = (float)width / (float)height;

  updateScreenVBO();

  printf("Window: %4d x %4d\n", width, height);
}

// --------------------------------

void resizeScreen(int width, int height)
{
  screen.width = width;
  screen.height = height;
  screen.aspect = (float)width / (float)height;

  updateScreenVBO();

  printf("Screen: %4d x %4d\n", width, height);
}

// --------------------------------

bool initSDL(void)
{
  if(SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("Failed to initialize SDL:  %s\n", SDL_GetError());
    return false;
  }

  window.sdl_window = SDL_CreateWindow("Retro", 
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      window.width, window.height, 
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

  if(!window.sdl_window)
  {
    printf("Failed to create SDL window\n");
    return false;
  }

  window.id = SDL_GetWindowID(window.sdl_window);

  return true;
}

// --------------------------------

bool initOpenGL(void)
{
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetSwapInterval(1);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  SDL_GLContext context = SDL_GL_CreateContext(window.sdl_window);

  if(!context)
  {
    printf("Failed to create the OpenGL context\n");
    return false;
  }

  glClearColor(0.53f, 0.48f, 0.87f, 1.0f);

  printf("%s\n",glGetString(GL_VERSION));
  printf("%s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));

  screen.program = createProgram(vertex_shader_source, fragment_shader_source);

  if(!screen.program) { return false; }

  glUseProgram(screen.program);

  GLint texSamplerUniformLoc = glGetUniformLocation(screen.program, "texSampler");
  glUniform1i(texSamplerUniformLoc, 0);

  unsigned char font_image[128 * 48];
  const unsigned int* font_bitmap_ptr = font_bitmap;

  for(Uint32 tile_counter = 0; tile_counter < 16 * 6; ++tile_counter)
  {
    const Uint32 tile_x = tile_counter & 0xF;
    const Uint32 tile_y = tile_counter >> 4;

    unsigned int font_bits = *font_bitmap_ptr++;

    for(Uint32 i = 0; i < 32; ++i)
    {
      const Uint32 px = i & 0x7;
      const Uint32 py = i >> 3;

      if(font_bits & (1 << i))
      {
        font_image[128 * (tile_y * 8 + py) + (tile_x * 8 + px)] = 0xFF;
      }
      else
      {
        font_image[128 * (tile_y * 8 + py) + (tile_x * 8 + px)] = 0x00;
      }
    }

    font_bits = *font_bitmap_ptr++;

    for(Uint32 i = 0; i < 32; ++i)
    {
      const Uint32 px = i & 0x7;
      const Uint32 py = (i >> 3) + 4;

      if(font_bits & (1 << i))
      {
        font_image[128 * (tile_y * 8 + py) + (tile_x * 8 + px)] = 0xFF;
      }
      else
      {
        font_image[128 * (tile_y * 8 + py) + (tile_x * 8 + px)] = 0x00;
      }
    }
  }

  screen.tex = createTexture(128, 48, font_image, GL_R8, GL_RED, GL_UNSIGNED_BYTE);

  if(!screen.tex) { return false; }

  glGenBuffers(1, &screen.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, screen.vbo);
  glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(GLfloat), nullptr, GL_STATIC_DRAW);

  glEnableVertexAttribArray(VertexAttribute::POSITION);
  glVertexAttribPointer(VertexAttribute::POSITION, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

  glEnableVertexAttribArray(VertexAttribute::UV);
  glVertexAttribPointer(VertexAttribute::UV, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void*)(2 * sizeof(GLfloat)));

  resizeWindow(window.width, window.height);

  return true;
}

// --------------------------------

bool startup(void)
{
  if(!initSDL()) { return false; }
  if(!initOpenGL()) { return false; }

  return true;
}

// --------------------------------

void render(void)
{
  glClear(GL_COLOR_BUFFER_BIT);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  SDL_GL_SwapWindow(window.sdl_window);
}

// --------------------------------

void update(void)
{
  SDL_Event event;

  while(SDL_PollEvent(&event))
  {
    switch (event.type)
    {
      case SDL_QUIT:
        running = false;
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        break;

      case SDL_WINDOWEVENT:
        {
          if (event.window.windowID == window.id
              && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
          {
            int width = event.window.data1;
            int height = event.window.data2;
            resizeWindow(width, height);
          }
          break;
        }
    }
  }

  render();
}

// --------------------------------

void shutdown(void)
{
  glDeleteProgram(screen.program);
  glDeleteBuffers(1, &screen.vbo);
  glDeleteTextures(1, &screen.tex);

  SDL_Quit();
}

// --------------------------------

int main(int argc, char** argv)
{
  if(!startup()) { return 1; }

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(update, 0, true);
#else
  while(running) {
    update();
  }
#endif

  shutdown();

  return 0;
}

// --------------------------------

