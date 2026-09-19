#pragma once

#include <cstring>

inline int StudioResolveTextureIndex(const short* skinrefs, int numSkinrefs, int numTextures, int meshSkinref)
{
	if (!skinrefs || meshSkinref < 0 || meshSkinref >= numSkinrefs)
		return -1;

	const int textureIndex = skinrefs[meshSkinref];
	if (textureIndex < 0 || textureIndex >= numTextures)
		return -1;

	return textureIndex;
}

inline bool StudioShouldReplaceMissingTextureModel(bool isSvEngine, int textureIndex, const char* resolvedName)
{
	// SvEngine changes model_t::name on substitution, not the requested path.
	return isSvEngine && textureIndex == 0 && resolvedName && !std::strcmp(resolvedName, "models/error.mdl");
}
