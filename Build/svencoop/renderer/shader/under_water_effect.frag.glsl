#version 430

#include "common.h"

in vec2 texCoord;

layout (location = 0) uniform float wave_amount = 10.0;
layout (location = 1) uniform float wave_speed = 1.0;
layout (location = 2) uniform float wave_size = 0.01;

layout(binding = 0) uniform sampler2D baseTex;

layout(location=0) out vec4 out_Color;

void main() {

  vec2 uv = texCoord;
  uv.x += cos(uv.y * wave_amount + SceneUBO.time * wave_speed) * wave_size;
  uv.y += sin(uv.x * wave_amount + SceneUBO.time * wave_speed) * wave_size;

  vec4 baseColor = texture(baseTex, uv);
  
  out_Color = baseColor;
}