
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

EM_JS(void, chooseFile, (), {
    const choose_file_dialog = document.getElementById("choose_file_dialog");
    choose_file_dialog.style.display = "block";
    });

EM_JS(void, saveFile, (const char* _filename, const char* _data), {
    var blob = new Blob([Module.UTF8ToString(_data)], { type: "text/plain;charset=utf-8" });
    saveAs(blob, Module.UTF8ToString(_filename));
    });

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
  GLuint vao     = 0;
  GLuint vbo     = 0;
  GLuint texture = 0;

  GLuint texture_unit = 0;
  GLint  screen_size_location = 0;
};

Display display;

struct VPU
{
  GLuint program = 0;
  GLuint vao     = 0;
  GLuint vbo     = 0;
  GLuint fbo     = 0;
  GLuint texture = 0;

  GLuint texture_unit = 1;
  GLint  screen_size_location = 0;
};

VPU vpu;

// --------------------------------

bool initWindow()
{
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

bool initDisplay()
{
  glGenVertexArrays(1, &display.vao);
  glBindVertexArray(display.vao);

  glGenBuffers(1, &display.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, display.vbo);
  glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(GLfloat), nullptr, GL_STATIC_DRAW);

  display.program = createProgram(pixel_upscale_vs, pixel_upscale_fs);

  if(!display.program) { return false; }

  glUseProgram(display.program);

  GLint position_location = glGetAttribLocation(display.program, "position");
  glEnableVertexAttribArray(position_location);
  glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

  GLint uv_location = glGetAttribLocation(display.program, "uv");
  glEnableVertexAttribArray(uv_location);
  glVertexAttribPointer(uv_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void*)(2 * sizeof(GLfloat)));

  GLint screen_sampler_location = glGetUniformLocation(display.program, "screen_sampler");
  glUniform1i(screen_sampler_location, display.texture_unit);

  display.screen_size_location = glGetUniformLocation(display.program, "screen_size");
  glUniform2f(display.screen_size_location, display.width, display.height);

  display.texture = createTexture(display.texture_unit, display.width, display.height, nullptr, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);

  if(!display.texture) { return false; }

  return true;
}

// --------------------------------

void showDisplay()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, window.width, window.height);
  glClearColor(0.53f, 0.48f, 0.87f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(display.program);
  glBindVertexArray(display.vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// --------------------------------

void destroyDisplay()
{
  glDeleteProgram(display.program);
  glDeleteBuffers(1, &display.vbo);
  glDeleteVertexArrays(1, &display.vao);
  glDeleteTextures(1, &display.texture);
}

// --------------------------------

void updateDisplayVBO()
{
  GLfloat vertices[16];

  const GLfloat display_size = window.width > 320 ? 0.95f : 1.0f;

  if(window.aspect < display.aspect)     // Portrait Device
  {
    vertices[0]  =  display_size;
    vertices[1]  =  display_size * window.aspect / display.aspect;
    vertices[2]  =  1.0f;
    vertices[3]  =  0.0f;

    vertices[4]  = -display_size;
    vertices[5]  =  display_size * window.aspect / display.aspect;
    vertices[6]  =  0.0f;
    vertices[7]  =  0.0f;

    vertices[8]  =  display_size;
    vertices[9]  = -display_size * window.aspect / display.aspect;
    vertices[10] =  1.0f;
    vertices[11] =  1.0f;

    vertices[12] = -display_size;
    vertices[13] = -display_size * window.aspect / display.aspect;
    vertices[14] =  0.0f;
    vertices[15] =  1.0f;
  }
  else                                  // Landscape Device
  {
    vertices[0]  =  display_size * display.aspect / window.aspect;
    vertices[1]  =  display_size;
    vertices[2]  =  1.0f;
    vertices[3]  =  0.0f;

    vertices[4]  = -display_size * display.aspect / window.aspect;
    vertices[5]  =  display_size;
    vertices[6]  =  0.0f;
    vertices[7]  =  0.0f;

    vertices[8]  =  display_size * display.aspect / window.aspect;
    vertices[9]  = -display_size;
    vertices[10] =  1.0f;
    vertices[11] =  1.0f;

    vertices[12] = -display_size * display.aspect / window.aspect;
    vertices[13] = -display_size;
    vertices[14] =  0.0f;
    vertices[15] =  1.0f;
  }

  glBindBuffer(GL_ARRAY_BUFFER, display.vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}

// --------------------------------

void resizeWindow(int width, int height)
{
  window.width = width;
  window.height = height;
  window.aspect = (float)width / (float)height;

  updateDisplayVBO();

  printf("Window: %4d x %4d\n", window.width, window.height);
}

// --------------------------------

void resizeDisplay(int width, int height)
{
  display.width = width;
  display.height = height;
  display.aspect = (float)width / (float)height;

  updateDisplayVBO();

  glUseProgram(vpu.program);
  glUniform2f(vpu.screen_size_location, display.width, display.height);

  glUseProgram(display.program);
  glUniform2f(display.screen_size_location, display.width, display.height);

  resizeTexture(display.texture_unit, display.texture, display.width, display.height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);

  printf("Display: %4d x %4d\n", display.width, display.height);
}

// --------------------------------

bool initVPU()
{
  GLfloat vertices[16];

  vertices[0]  =  1.0f;
  vertices[1]  =  1.0f;
  vertices[2]  =  1.0f;
  vertices[3]  =  1.0f;

  vertices[4]  = -1.0f;
  vertices[5]  =  1.0f;
  vertices[6]  =  0.0f;
  vertices[7]  =  1.0f;

  vertices[8]  =  1.0f;
  vertices[9]  = -1.0f;
  vertices[10] =  1.0f;
  vertices[11] =  0.0f;

  vertices[12] = -1.0f;
  vertices[13] = -1.0f;
  vertices[14] =  0.0f;
  vertices[15] =  0.0f;

  glGenVertexArrays(1, &vpu.vao);
  glBindVertexArray(vpu.vao);

  glGenBuffers(1, &vpu.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vpu.vbo);
  glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

  vpu.program = createProgram(text_mode_vs, text_mode_fs);

  if(!vpu.program) { return false; }

  glUseProgram(vpu.program);

  GLint position_location = glGetAttribLocation(vpu.program, "position");
  glEnableVertexAttribArray(position_location);
  glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

  GLint uv_location = glGetAttribLocation(vpu.program, "uv");
  glEnableVertexAttribArray(uv_location);
  glVertexAttribPointer(uv_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void*)(2 * sizeof(GLfloat)));

  GLint font_sampler_location = glGetUniformLocation(vpu.program, "font_sampler");
  glUniform1i(font_sampler_location, vpu.texture_unit);

  vpu.screen_size_location = glGetUniformLocation(vpu.program, "screen_size");
  glUniform2f(vpu.screen_size_location, display.width, display.height);

  vpu.texture = loadFont(vpu.texture_unit);
  // vpu.texture = loadTexture(vpu.texture_unit, "doom.png");

  if(!vpu.texture) { return false; }

  glGenFramebuffers(1, &vpu.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, vpu.fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, display.texture, 0);
  GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, DrawBuffers);
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    printf("Failed to create Framebuffer\n");
    return false;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return true;
}

// --------------------------------

void renderVPU()
{
  glActiveTexture(GL_TEXTURE0 + display.texture_unit);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, vpu.fbo);
  glViewport(0, 0, display.width, display.height);
  glClearColor(0.28f, 0.23f, 0.67f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(vpu.program);
  glBindVertexArray(vpu.vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindTexture(GL_TEXTURE_2D, display.texture);
}

// --------------------------------

void destroyVPU()
{
  glDeleteProgram(vpu.program);
  glDeleteBuffers(1, &vpu.vbo);
  glDeleteVertexArrays(1, &vpu.vao);
  glDeleteTextures(1, &vpu.texture);
  glDeleteFramebuffers(1, &vpu.fbo);
}

// --------------------------------

bool initSDL(void)
{
  if(SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("Failed to initialize SDL:  %s\n", SDL_GetError());
    return false;
  }

  if(!initWindow()) { return false; }

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

  printf("%s\n",glGetString(GL_VERSION));
  printf("%s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));

  if(!initDisplay()) { return false; }
  if(!initVPU()) { return false; }

  resizeWindow(window.width, window.height);

  return true;
}

// --------------------------------

bool startup(void)
{
  if(!initSDL()) { return false; }
  if(!initOpenGL()) { return false; }

  // saveFile("game", "The time has come the Walrus said, to talk of many things.");
  // chooseFile();

  return true;
}

// --------------------------------

void render(void)
{
  renderVPU();
  showDisplay();

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
  destroyDisplay();
  destroyVPU();

  SDL_Quit();
}

// --------------------------------

#ifdef __cplusplus
extern "C" { // So that the C++ compiler does not rename our function names
#endif

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif

int loadFile(char* _data)
{
  printf("%s\n", _data);
  return 0;
}

#ifdef __cplusplus
}
#endif

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

