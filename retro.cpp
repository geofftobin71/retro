
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

struct Display
{
  int    width   = 320;
  int    height  = 240;
  float  aspect  = 320.0f / 240.0f;
  GLuint program = 0;
  GLuint vbo     = 0;
  GLuint texture = 0;
};

Display display;

struct VPU
{
  GLuint program = 0;
  GLuint vbo = 0;
  GLuint tex = 0;
};

VPU vpu;

// --------------------------------

void updateDisplay()
{
  GLfloat vertices[16];

  const GLfloat display_size = 0.95f;

  if(window.aspect < display.aspect)     // Portrait Device
  {
    vertices[0]  =  display_size;
    vertices[1]  =  display_size * window.aspect / display.aspect;
    vertices[2]  =  1.0f; // display.width / 128.0f;
    vertices[3]  =  0.0f;

    vertices[4]  = -display_size;
    vertices[5]  =  display_size * window.aspect / display.aspect;
    vertices[6]  =  0.0f;
    vertices[7]  =  0.0f;

    vertices[8]  =  display_size;
    vertices[9]  = -display_size * window.aspect / display.aspect;
    vertices[10] =  1.0f; // display.width / 128.0f;
    vertices[11] =  1.0f; // display.height / 48.0f;

    vertices[12] = -display_size;
    vertices[13] = -display_size * window.aspect / display.aspect;
    vertices[14] =  0.0f;
    vertices[15] =  1.0f; // display.height / 48.0f;
  }
  else                                  // Landscape Device
  {
    vertices[0]  =  display_size * display.aspect / window.aspect;
    vertices[1]  =  display_size;
    vertices[2]  =  1.0f; // display.width / 128.0f;
    vertices[3]  =  0.0f;

    vertices[4]  = -display_size * display.aspect / window.aspect;
    vertices[5]  =  display_size;
    vertices[6]  =  0.0f;
    vertices[7]  =  0.0f;

    vertices[8]  =  display_size * display.aspect / window.aspect;
    vertices[9]  = -display_size;
    vertices[10] =  1.0f; // display.width / 128.0f;
    vertices[11] =  1.0f; // display.height / 48.0f;

    vertices[12] = -display_size * display.aspect / window.aspect;
    vertices[13] = -display_size;
    vertices[14] =  0.0f;
    vertices[15] =  1.0f; // display.height / 48.0f;
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

  updateDisplay();

  printf("Window: %4d x %4d\n", window.width, window.height);
}

// --------------------------------

void resizeDisplay(int width, int height)
{
  display.width = width;
  display.height = height;
  display.aspect = (float)width / (float)height;

  updateDisplay();

  printf("Display: %4d x %4d\n", display.width, display.height);
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

  display.program = createProgram(pixel_upscale_vs, pixel_upscale_fs);

  if(!display.program) { return false; }

  glUseProgram(display.program);

  GLint position_location = glGetAttribLocation(display.program, "position");
  glEnableVertexAttribArray(position_location);
  glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

  GLint uv_location = glGetAttribLocation(display.program, "uv");
  glEnableVertexAttribArray(uv_location);
  glVertexAttribPointer(uv_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void*)(2 * sizeof(GLfloat)));

  GLint sampler_location = glGetUniformLocation(display.program, "sampler");
  glUniform1i(sampler_location, 0);

  display.texture = loadTexture("doom.png");
  // display.texture = loadFont();

  if(!display.texture) { return false; }

  glGenBuffers(1, &display.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, display.vbo);
  glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(GLfloat), nullptr, GL_STATIC_DRAW);

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
  glDeleteProgram(display.program);
  glDeleteBuffers(1, &display.vbo);
  glDeleteTextures(1, &display.texture);

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

