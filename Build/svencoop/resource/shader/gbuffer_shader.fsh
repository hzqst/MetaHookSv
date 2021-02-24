uniform sampler2D diffuseTex;
#ifdef LIGHTMAP_ENABLED
uniform sampler2D lightmapTex;
#endif
#ifdef DETAILTEXTURE_ENABLED
uniform sampler2D detailTex;
#endif

varying vec4 worldpos;
varying vec4 normal;
varying vec4 color;

void main()
{
    vec4 diffuseColor = texture2D(diffuseTex, gl_TexCoord[0].xy);
#ifdef LIGHTMAP_ENABLED
    vec4 lightmapColor = texture2D(lightmapTex, gl_TexCoord[1].xy);
#else
    vec4 lightmapColor = color;
#endif

#ifdef DETAILTEXTURE_ENABLED
    vec4 detailColor = texture2D(detailTex, gl_TexCoord[2].xy);
#else
    vec4 detailColor = vec4(1.0, 1.0, 1.0, 1.0);
#endif

    vec4 mixedColor = diffuseColor * detailColor;

#ifdef TRANSPARENT_ENABLED
    gl_FragData[0] = diffuseColor * color;
#else
    gl_FragData[0] = mixedColor;
    gl_FragData[1] = lightmapColor;
    gl_FragData[2] = worldpos;
    gl_FragData[3] = normal;
#endif
}