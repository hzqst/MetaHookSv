#ifdef DIFFUSE_ENABLED
uniform sampler2D diffuseTex;
#endif

#ifdef LIGHTMAP_ENABLED
uniform sampler2D lightmapTex;
#endif

#ifdef DETAIL_ENABLED
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
    vec4 lightmapColor = texture2D(lightmapTex, gl_TexCoord[1].xy);
#else
    vec4 lightmapColor = color;
#endif

#ifdef DETAIL_ENABLED
    vec4 detailColor = texture2D(detailTex, gl_TexCoord[2].xy);
#else
    vec4 detailColor = vec4(1.0, 1.0, 1.0, 1.0);
#endif

#ifdef TRANSPARENT_ENABLED
    vec4 mixedColor = diffuseColor * color;
    gl_FragData[0] = mixedColor;
#else
    vec4 mixedColor = diffuseColor * detailColor;
    gl_FragData[0] = mixedColor;
    gl_FragData[1] = lightmapColor;
    gl_FragData[2] = worldpos;
    gl_FragData[3] = normal;
#endif
}