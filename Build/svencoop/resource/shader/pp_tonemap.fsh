#version 120

uniform sampler2D basetex;
uniform sampler2D blurtex;
uniform float blurfactor;
uniform float darkness;
uniform float gamma;
varying float lum;

vec3 vignette(vec3 c, vec2 win_bias)
{
    // convert window coord to [-1, 1] range
    vec2 wpos = 2.0*(win_bias - vec2(0.5, 0.5)); 

    // calculate distance from origin
    float r = length(wpos);
    r = 1.0 - smoothstep(0.9, 1.9, r);
	return c * r;
}

void main() 
{
    vec3 vColor = (1.0f - blurfactor) * texture2D(basetex, gl_TexCoord[0].xy).xyz + blurfactor * texture2D(blurtex, gl_TexCoord[0].xy).xyz * 0.33;

	vColor.x = pow(vColor.x, darkness);
	vColor.y = pow(vColor.y, darkness);
	vColor.z = pow(vColor.z, darkness);
	
	vec3 L = vColor * lum;

    // exposure
	vColor = L / (1 + L);

    // vignette effect (makes brightness drop off with distance from center)
	vColor = vignette(vColor, gl_TexCoord[0].xy); 

	//gamma correction
	vColor.x = pow(vColor.x, gamma);
	vColor.y = pow(vColor.y, gamma);
	vColor.z = pow(vColor.z, gamma);

	gl_FragColor = vec4(vColor, 1.0);
}