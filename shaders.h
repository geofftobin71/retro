#ifndef _shaders_h_
#define _shaders_h_

const char* vertex_shader_source =
R"VS(#version 300 es
/* layout(std140) uniform Registers
{
  float wAspect;
  float sAspect;
} registers; */
in vec2 position;
in vec2 uv;
out vec2 pixel;
void main()
{
  gl_Position = vec4(position, 0.0, 1.0);
  pixel = uv;
}
)VS";

const char* fragment_shader_source =
R"FS(#version 300 es
precision mediump float;
in vec2 pixel;
out vec4 color;
uniform sampler2D texSampler;
void main()
{
  // color = vec4(0.28,0.23,0.67,1.0);
  color = texelFetch(texSampler, ivec2(pixel), 0);
}
)FS";

#endif
