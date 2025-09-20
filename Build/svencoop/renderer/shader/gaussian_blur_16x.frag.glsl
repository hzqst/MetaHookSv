#version 430

#include "common.h"

in vec2 texCoord;

layout(location = 0) uniform float du;

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 out_Color;

void main() {
  vec4 color = vec4(0.0);
#if defined(BLUR_HORIZONAL)
  color += 0.000122 * texture( tex, texCoord.xy + vec2(-15.275129 * du, 0.0) );
  color += 0.000725 * texture( tex, texCoord.xy + vec2(-13.300746 * du, 0.0) );
  color += 0.003381 * texture( tex, texCoord.xy + vec2(-11.327668 * du, 0.0) );
  color += 0.012317 * texture( tex, texCoord.xy + vec2(-9.355775 * du, 0.0) );
  color += 0.035068 * texture( tex, texCoord.xy + vec2(-7.384912 * du, 0.0) );
  color += 0.078044 * texture( tex, texCoord.xy + vec2(-5.414899 * du, 0.0) );
  color += 0.135782 * texture( tex, texCoord.xy + vec2(-3.445529 * du, 0.0) );
  color += 0.184690 * texture( tex, texCoord.xy + vec2(-1.476580 * du, 0.0) );
  color += 0.196410 * texture( tex, texCoord.xy + vec2(0.492188 * du, 0.0) );
  color += 0.163306 * texture( tex, texCoord.xy + vec2(2.461017 * du, 0.0) );
  color += 0.106159 * texture( tex, texCoord.xy + vec2(4.430147 * du, 0.0) );
  color += 0.053951 * texture( tex, texCoord.xy + vec2(6.399812 * du, 0.0) );
  color += 0.021433 * texture( tex, texCoord.xy + vec2(8.370225 * du, 0.0) );
  color += 0.006656 * texture( tex, texCoord.xy + vec2(10.341582 * du, 0.0) );
  color += 0.001615 * texture( tex, texCoord.xy + vec2(12.314051 * du, 0.0) );
  color += 0.000306 * texture( tex, texCoord.xy + vec2(14.287767 * du, 0.0) );
#elif defined(BLUR_VERTICAL)
  color += 0.000122 * texture( tex, texCoord.xy + vec2(0.0, -15.275129 * du) );
  color += 0.000725 * texture( tex, texCoord.xy + vec2(0.0, -13.300746 * du) );
  color += 0.003381 * texture( tex, texCoord.xy + vec2(0.0, -11.327668 * du) );
  color += 0.012317 * texture( tex, texCoord.xy + vec2(0.0, -9.355775 * du) );
  color += 0.035068 * texture( tex, texCoord.xy + vec2(0.0, -7.384912 * du) );
  color += 0.078044 * texture( tex, texCoord.xy + vec2(0.0, -5.414899 * du) );
  color += 0.135782 * texture( tex, texCoord.xy + vec2(0.0, -3.445529 * du) );
  color += 0.184690 * texture( tex, texCoord.xy + vec2(0.0, -1.476580 * du) );
  color += 0.196410 * texture( tex, texCoord.xy + vec2(0.0, 0.492188 * du) );
  color += 0.163306 * texture( tex, texCoord.xy + vec2(0.0, 2.461017 * du) );
  color += 0.106159 * texture( tex, texCoord.xy + vec2(0.0, 4.430147 * du) );
  color += 0.053951 * texture( tex, texCoord.xy + vec2(0.0, 6.399812 * du) );
  color += 0.021433 * texture( tex, texCoord.xy + vec2(0.0, 8.370225 * du) );
  color += 0.006656 * texture( tex, texCoord.xy + vec2(0.0, 10.341582 * du) );
  color += 0.001615 * texture( tex, texCoord.xy + vec2(0.0, 12.314051 * du) );
  color += 0.000306 * texture( tex, texCoord.xy + vec2(0.0, 14.287767 * du) );
#endif
  out_Color = color;
}