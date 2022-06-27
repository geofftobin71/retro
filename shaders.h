#ifndef _shaders_h_
#define _shaders_h_

// --------------------------------

const char* pixel_upscale_vs =
R"VS(#version 300 es
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
  vec2 alpha = 0.7 * vec2(dFdx(pixel.x), dFdy(pixel.y));
  vec2 x = fract(pixel);
  vec2 x_ = clamp(0.5 / alpha * x, 0.0, 0.5) + clamp(0.5 / alpha * (x - 1.0) + 0.5, 0.0, 0.5);

  vec2 uv_ = (floor(pixel) + x_) / screen_size;

  color = texture(screen_sampler, uv_);
}
)FS";

// --------------------------------

const char* text_mode_vs =
R"VS(#version 300 es
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
uniform sampler2D font_sampler;
void main()
{
  const vec4 bg = vec4(0.28, 0.23, 0.67, 1.0);
  const vec4 fg = vec4(0.53, 0.48, 0.87, 1.0);

  float c = texelFetch(font_sampler, ivec2(pixel), 0).r;
  color = mix(bg, fg, c);

  // color = texture(font_sampler, pixel);
}
)FS";

// --------------------------------

#endif
