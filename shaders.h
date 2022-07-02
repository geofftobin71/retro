#ifndef _shaders_h_
#define _shaders_h_

// --------------------------------

const char* pixel_upscale_vs =
R"VS(#version 300 es
precision highp float;
in vec2 position;
in vec2 uv;
out vec2 pixel;
uniform vec2 screen_size;
void main()
{
  gl_Position = vec4(position, 0.0, 1.0);
  pixel = uv * screen_size;
}
)VS";

// --------------------------------

const char* pixel_upscale_fs =
R"FS(#version 300 es
precision highp float;
in vec2 pixel;
out vec4 color;
uniform vec2 screen_size;
uniform sampler2D screen_sampler;
void main()
{
  vec2 seam = floor(pixel + 0.5);
  vec2 dudv = fwidth(pixel);
  vec2 uv = (seam + clamp((pixel - seam) / dudv, -0.5, 0.5)) / screen_size;

  color = texture(screen_sampler, uv);
}
)FS";

// --------------------------------

const char* text_mode_vs =
R"VS(#version 300 es
precision highp float;
in vec2 position;
in vec2 uv;
out vec2 pixel;
uniform vec2 screen_size;
void main()
{
  gl_Position = vec4(position, 0.0, 1.0);
  pixel = uv * screen_size;
}
)VS";

// --------------------------------

const char* text_mode_fs =
R"FS(#version 300 es
precision highp float;
in vec2 pixel;
out vec4 color;
uniform highp sampler2D font_sampler;
uniform highp usampler2D map_sampler;
void main()
{
  const vec4 bg = vec4(0.28, 0.23, 0.67, 1.0);
  const vec4 fg = vec4(0.53, 0.48, 0.87, 1.0);

  uint cell = texelFetch(map_sampler, ivec2(pixel) >> 3, 0).r;

  uint cell_x = (cell & 0x0FU) << 3;
  uint cell_y = (cell & 0xF0U) >> 1;

  uint pixel_x = uint(pixel.x) & 0x07U;
  uint pixel_y = uint(pixel.y) & 0x07U;

  float c = texelFetch(font_sampler, ivec2(cell_x + pixel_x, cell_y + pixel_y), 0).r;

  color = mix(bg, fg, c);
}
)FS";

// --------------------------------

#endif
