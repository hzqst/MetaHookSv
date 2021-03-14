#ifdef LIGHT_PASS

uniform sampler2D positionTex;
uniform sampler2D normalTex;

uniform vec3 lightcolor;
uniform float lightcone;
uniform float lightradius;
uniform float lightambient;
uniform float lightdiffuse;
uniform float lightspecular;
uniform float lightspecularpow;

uniform vec4 viewpos;
uniform vec4 lightdir;
uniform vec4 lightpos;

#endif

#ifdef FINAL_PASS
uniform sampler2D diffuseTex;
uniform sampler2D lightmapTex;
uniform sampler2D additiveTex;
uniform sampler2D depthTex;
uniform vec4 clipInfo;
#endif

#ifdef DECAL_PASS

varying vec4 projpos;
uniform mat4 decalToWorldMatrix;
uniform mat4 worldToDecalMatrix;
uniform float decalScale;
uniform vec4 decalCenter;
uniform sampler2D decalTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;

void main()
{
    vec2 vBaseTexCoord = projpos.xy / projpos.w * 0.5 + 0.5;

    vec4 world = vec4(texture2D(positionTex, vBaseTexCoord).xyz, 1.0);

    if(  length(world.xyz - decalCenter.xyz) > decalScale)
        discard;

    vec3 worldNormal = texture2D(normalTex, vBaseTexCoord).xyz;

    vec3 decalNormal = mat3(worldToDecalMatrix) * worldNormal;

    vec4 decalPos = worldToDecalMatrix * world;
    decalPos.xyz /= decalPos.w;

    vec2 decal_texcoord = decalPos.xy / decalScale + 0.5;

    float threshold = 0.1f;
    if(abs(dot(vec3(worldToDecalMatrix[2]), worldNormal)) < threshold)
    {
        decal_texcoord = (decalPos.xy + decalPos.z * decalNormal.xy) / decalScale + 0.5;
    }

    if (decal_texcoord.x < 0.0 || decal_texcoord.x > 1.0 || decal_texcoord.y < 0.0 || decal_texcoord.y > 1.0)
       discard;

    vec4 albedo = texture2D(decalTex, decal_texcoord);

    gl_FragColor = albedo;
}

#endif

#ifdef LIGHT_PASS
vec4 CalcLightInternal(vec3 World, vec3 LightDirection, vec3 Normal)
{
    vec4 AmbientColor = vec4(lightcolor, 1.0) * lightambient;
    float DiffuseFactor = dot(Normal, -LightDirection);
 
    vec4 DiffuseColor = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 SpecularColor = vec4(0.0, 0.0, 0.0, 0.0);
 
    if (DiffuseFactor > 0.0) {
        DiffuseColor = vec4(lightcolor * lightdiffuse * DiffuseFactor, 1.0);
        vec3 VertexToEye = normalize(viewpos.xyz - World);
        vec3 LightReflect = normalize(reflect(LightDirection, Normal));
        float SpecularFactor = dot(VertexToEye, LightReflect);
        if (SpecularFactor > 0.0) {
            SpecularFactor = pow(SpecularFactor, lightspecularpow);
            SpecularColor = vec4(lightcolor * lightspecular * SpecularFactor, 1.0);
        }
    }
 
    return (AmbientColor + DiffuseColor + SpecularColor);
}

vec4 CalcPointLight(vec3 World, vec3 Normal)
{
    vec3 LightDirection = World - lightpos.xyz;
    float Distance = length(LightDirection);
    LightDirection = normalize(LightDirection);
 
    vec4 Color = CalcLightInternal(World, LightDirection, Normal);

    float r2 = lightradius * lightradius;
    float Attenuation = clamp(( r2 - (Distance * Distance)) / r2, 0.0, 1.0);
 
    return Color * Attenuation;
}

vec4 CalcSpotLight(vec3 World, vec3 Normal)
{
    vec3 LightToPixel = normalize(World - lightpos.xyz);
    float SpotFactor = dot(LightToPixel, lightdir.xyz);
    if (SpotFactor > lightcone) {
        vec4 Color = CalcPointLight(World, Normal);
        return Color * (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - lightcone));
    }
    else {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }
}

void main()
{
    vec4 positionColor = texture2D(positionTex, gl_TexCoord[0].xy);
    vec4 normalColor = texture2D(normalTex, gl_TexCoord[0].xy);

    vec3 worldpos = positionColor.xyz;
    //worldpos *= 1024.0;
    vec3 normal = normalColor.xyz;
    
#ifdef LIGHT_PASS_SPOT
    gl_FragColor = CalcSpotLight(worldpos, normal);
#endif

#ifdef LIGHT_PASS_POINT
    gl_FragColor = CalcPointLight(worldpos, normal);
#endif
}
#endif

#ifdef FINAL_PASS

#ifdef LINEAR_FOG_ENABLED

    float reconstructCSZ(float d) {
        return (clipInfo[0] / (clipInfo[1] * d + clipInfo[2]));
    }

#endif

void main()
{
    vec4 diffuseColor = texture2D(diffuseTex, gl_TexCoord[0].xy);
    vec4 lightmapColor = texture2D(lightmapTex, gl_TexCoord[0].xy);
    vec4 additiveColor = texture2D(additiveTex, gl_TexCoord[0].xy);
    
    vec4 finalColor = diffuseColor * lightmapColor + additiveColor;

    gl_FragColor = finalColor;

#ifdef LINEAR_FOG_ENABLED
    vec4 depthColor = texture2D(depthTex, gl_TexCoord[0].xy);

	float z = reconstructCSZ(depthColor.x);
	float fogFactor = ( gl_Fog.end - z ) / ( gl_Fog.end - gl_Fog.start );
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	gl_FragColor.xyz = mix(gl_Fog.color.xyz, finalColor.xyz, fogFactor );
#endif

}
#endif