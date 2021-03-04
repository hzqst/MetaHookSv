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

#ifdef TRANSPARENT_ENABLED

    #ifdef ADDITIVE_ENABLED

        gl_FragColor = diffuseColor * lightmapColor;

    #else

        gl_FragColor = diffuseColor * lightmapColor;

    #endif

#else

    vec4 mixedColor = diffuseColor * detailColor;
    gl_FragData[0] = mixedColor;
    gl_FragData[1] = lightmapColor;
    gl_FragData[2] = worldpos;
    gl_FragData[3] = normal;

#endif
}