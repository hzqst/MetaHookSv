#extension GL_EXT_texture_array : enable

#ifdef DIFFUSE_ENABLED
uniform sampler2D diffuseTex;
#endif

#ifdef LIGHTMAP_ENABLED

    #ifdef LIGHTMAP_ARRAY_ENABLED

    uniform sampler2DArray lightmapTexArray;

    #else

    uniform sampler2D lightmapTex;
    
    #endif

#endif

#ifdef DETAILTEXTURE_ENABLED
uniform sampler2D detailTex;
#endif

varying vec4 worldpos;
varying vec4 normal;
varying vec4 color;

vec2 encodeNormal(vec3 n)
{
    vec2 enc = normalize(n.xy) * (sqrt(-n.z*0.5+0.5));
    enc = vec2(enc.x*0.5+0.5, enc.y*0.5+0.5);
    return enc;
}

vec3 decodeNormal(vec2 enc)
{
    vec4 nn = vec4(enc.x * 2.0, enc.y * 2.0, 0.0, 0.0) + vec4(-1.0, -1.0, 1.0, -1.0);
    float l = dot(nn.xyz, -nn.xyw);
    nn.z = l;
    nn.xy *= sqrt(l);
    return vec3(nn.x * 2.0, nn.y * 2.0, nn.z * 2.0) + vec3(0.0, 0.0, -1.0);
}

void main()
{
#ifdef DIFFUSE_ENABLED
    vec4 diffuseColor = texture2D(diffuseTex, gl_TexCoord[0].xy);
#else
    vec4 diffuseColor = color;
#endif

#ifdef LIGHTMAP_ENABLED

    #ifdef LIGHTMAP_ARRAY_ENABLED
        vec4 lightmapColor = texture2DArray(lightmapTexArray, gl_TexCoord[1].xyz);
    #else
        vec4 lightmapColor = texture2D(lightmapTex, gl_TexCoord[1].xy);
    #endif

#else

    vec4 lightmapColor = color;

#endif

#ifdef DETAILTEXTURE_ENABLED
    vec4 detailColor = texture2D(detailTex, gl_TexCoord[2].xy);
    detailColor.xyz *= 2.0;
    detailColor.a = 1.0;
#else
    vec4 detailColor = vec4(1.0, 1.0, 1.0, 1.0);
#endif

#ifdef MASKED_ENABLED
    if(diffuseColor.a < 0.5)
        discard;
#endif

#ifdef SHADOW_ENABLED

	gl_FragColor = vec4(worldpos.xyz, gl_FragCoord.z);

#else

    vec2 normalenc = encodeNormal(normal.xyz);

    #ifdef TRANSPARENT_ENABLED

        #ifdef ADDITIVE_ENABLED

            gl_FragColor = diffuseColor * lightmapColor;

        #else

            gl_FragData[0] = diffuseColor;
            gl_FragData[1] = lightmapColor;
            gl_FragData[2] = worldpos;
            gl_FragData[3] = vec4(normalenc.x, normalenc.y, 0.0, 0.0);
            gl_FragData[4] = vec4(0.0, 0.0, 0.0, 1.0);

        #endif

    #else

        vec4 mixedColor = diffuseColor * detailColor;
        gl_FragData[0] = mixedColor;
        gl_FragData[1] = lightmapColor;
        gl_FragData[2] = worldpos;
        gl_FragData[3] = vec4(normalenc.x, normalenc.y, 0.0, 0.0);
        gl_FragData[4] = vec4(0.0, 0.0, 0.0, 1.0);

    #endif

#endif
}