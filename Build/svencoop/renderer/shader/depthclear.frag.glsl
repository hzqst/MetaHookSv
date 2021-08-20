#version 430

in vec2 texCoord;

layout(location=0) out vec4 out_Color;
layout (depth_any) out float gl_FragDepth;

void main() {
  out_Color = vec4(0.0);
  gl_FragDepth = 1.0;
}