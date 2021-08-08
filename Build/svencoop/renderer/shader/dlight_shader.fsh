#extension GL_EXT_texture_array : enable

#define GBUFFER_INDEX_DIFFUSE		0.0
#define GBUFFER_INDEX_LIGHTMAP		1.0
#define GBUFFER_INDEX_WORLDNORM		2.0
#define GBUFFER_INDEX_SPECULAR		3.0
#define GBUFFER_INDEX_ADDITIVE		4.0

uniform sampler2DArray gbufferTex;
uniform sampler2D depthTex;
uniform usampler2D stencilTex;

uniform vec3 lightcolor;
uniform float lightcone;
uniform float lightradius;
uniform float lightambient;
uniform float lightdiffuse;
uniform float lightspecular;
uniform float lightspecularpow;

uniform vec3 viewpos;
uniform vec3 lightdir;
uniform vec3 lightpos;

#ifdef VOLUME_ENABLED
varying vec4 projpos;
#endif

varying vec3 fragpos;

vec2 UnitVectorToHemiOctahedron(vec3 dir) {

	dir.y = max(dir.y, 0.0001);
	dir.xz /= dot(abs(dir), vec3(1.0));

	return clamp(0.5 * vec2(dir.x + dir.z, dir.x - dir.z) + 0.5, 0.0, 1.0);

}

vec3 HemiOctahedronToUnitVector(vec2 coord) {

	coord = 2.0 * coord - 1.0;
	coord = 0.5 * vec2(coord.x + coord.y, coord.x - coord.y);

	float y = 1.0 - dot(vec2(1.0), abs(coord));
	return normalize(vec3(coord.x, y + 0.0001, coord.y));

}

vec2 UnitVectorToOctahedron(vec3 dir) {

    dir.xz /= dot(abs(dir), vec3(1.0));

	// Lower hemisphere
	if (dir.y < 0.0) {
		vec2 orig = dir.xz;
		dir.x = (orig.x >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.y));
        dir.z = (orig.y >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.x));
	}

	return clamp(0.5 * vec2(dir.x, dir.z) + 0.5, 0.0, 1.0);

}

vec3 OctahedronToUnitVector(vec2 coord) {

	coord = 2.0 * coord - 1.0;
	float y = 1.0 - dot(abs(coord), vec2(1.0));

	// Lower hemisphere
	if (y < 0.0) {
		vec2 orig = coord;
		coord.x = (orig.x >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.y));
		coord.y = (orig.y >= 0.0 ? 1.0 : -1.0) * (1.0 - abs(orig.x));
	}

	return normalize(vec3(coord.x, y + 0.0001, coord.y));

}

vec4 CalcLightInternal(vec3 World, vec3 LightDirection, vec3 Normal, uint stencil, float specularValue)
{
    vec4 AmbientColor = vec4(lightcolor, 1.0) * lightambient;
    vec4 DiffuseColor = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 SpecularColor = vec4(0.0, 0.0, 0.0, 0.0);

    if(stencil < 2)
    {
        float DiffuseFactor = dot(Normal, -LightDirection);
    
        if (DiffuseFactor > 0.0) {
            DiffuseColor = vec4(lightcolor * lightdiffuse * DiffuseFactor, 1.0);
            vec3 VertexToEye = normalize(viewpos.xyz - World);
            vec3 LightReflect = normalize(reflect(LightDirection, Normal));
            float SpecularFactor = dot(VertexToEye, LightReflect);
            if (SpecularFactor > 0.0) {
                SpecularFactor = pow(SpecularFactor, lightspecularpow);
                SpecularColor = vec4(lightcolor * lightspecular * SpecularFactor * specularValue, 1.0);
            }
        }
    }
    else
    {
        DiffuseColor = vec4(lightcolor * lightdiffuse * 0.8, 1.0);
    }
 
    return (AmbientColor + DiffuseColor + SpecularColor);
}

vec4 CalcPointLight(vec3 World, vec3 Normal, uint stencil, float specularValue)
{
    vec3 LightDirection = World - lightpos.xyz;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);
 
    vec4 Color = CalcLightInternal(World, LightDirection, Normal, stencil, specularValue);

    float r2 = lightradius * lightradius;
    float Attenuation = clamp(( r2 - (Distance * Distance)) / r2, 0.0, 1.0);
 
    return Color * Attenuation;
}

vec4 CalcSpotLight(vec3 World, vec3 Normal, uint stencil, float specularValue)
{
    vec3 LightToPixel = normalize(World - lightpos.xyz);
    float SpotFactor = dot(LightToPixel, lightdir.xyz);
    if (SpotFactor > lightcone) {
        vec4 Color = CalcPointLight(World, Normal, stencil, specularValue);
        return Color * (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - lightcone));
    }
    else {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }
}

void main()
{
    
#ifdef VOLUME_ENABLED
    vec2 vBaseTexCoord = projpos.xy / projpos.w * 0.5 + 0.5;
#else
    vec2 vBaseTexCoord = gl_TexCoord[0].xy;
#endif

    vec4 worldnormColor = texture2DArray(gbufferTex, vec3(vBaseTexCoord, GBUFFER_INDEX_WORLDNORM));
    vec4 specularColor = texture2DArray(gbufferTex, vec3(vBaseTexCoord, GBUFFER_INDEX_SPECULAR));

    //float depthColor = texture2D(depthTex, vBaseTexCoord.xy).r;

    uint stencilColor = texture(stencilTex, vBaseTexCoord).r;

    vec3 normal = OctahedronToUnitVector(worldnormColor.xy);

    vec3 worldpos = viewpos.xyz + normalize(fragpos.xyz - viewpos.xyz) * worldnormColor.z;

    float specularValue = specularColor.x;

#ifdef SPOT_ENABLED
    gl_FragColor = CalcSpotLight(worldpos, normal, stencilColor, specularValue);
#endif

#ifdef POINT_ENABLED
    gl_FragColor = CalcPointLight(worldpos, normal, stencilColor, specularValue);
#endif

}