#version 120

uniform sampler2D adatex;
uniform sampler2D curtex;
uniform float frametime;

void main() 
{
    vec4 adaptedLum = texture2D(adatex, vec2(0.5f, 0.5f));
    vec4 currentLum = texture2D(curtex, vec2(0.5f, 0.5f));

	gl_FragColor = (adaptedLum + (currentLum - adaptedLum) * ( 1.0 - pow( 0.98, frametime ) ));
}