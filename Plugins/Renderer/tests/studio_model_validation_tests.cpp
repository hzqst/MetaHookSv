#include <cassert>
#include <cstdio>

#include "../studio_model_validation.h"

int main()
{
	const short skins[] = { 0, 1, -1, 2 };
	assert(0 == StudioResolveTextureIndex(skins, 4, 2, 0));
	assert(1 == StudioResolveTextureIndex(skins, 4, 2, 1));
	assert(-1 == StudioResolveTextureIndex(skins, 4, 2, -1));
	assert(-1 == StudioResolveTextureIndex(skins, 4, 2, 4));
	assert(-1 == StudioResolveTextureIndex(skins, 4, 2, 2));
	assert(-1 == StudioResolveTextureIndex(skins, 4, 2, 3));
	assert(-1 == StudioResolveTextureIndex(nullptr, 4, 2, 0));
	assert(-1 == StudioResolveTextureIndex(skins, 0, 2, 0));
	assert(-1 == StudioResolveTextureIndex(skins, 4, 0, 0));

	// Issue #863: the replacement has one skinref, but the body uses 0..25.
	const short errorSkin[] = { 0 };
	for (int skinref = 1; skinref <= 25; ++skinref)
		assert(-1 == StudioResolveTextureIndex(errorSkin, 1, 1, skinref));
	assert(0 == StudioResolveTextureIndex(errorSkin, 1, 1, 0));

	assert(StudioShouldReplaceMissingTextureModel(true, 0, "models/error.mdl"));
	assert(!StudioShouldReplaceMissingTextureModel(false, 0, "models/error.mdl"));
	assert(!StudioShouldReplaceMissingTextureModel(true, 244, "models/error.mdl"));
	assert(!StudioShouldReplaceMissingTextureModel(true, 0, "models/exampleT.mdl"));
	assert(!StudioShouldReplaceMissingTextureModel(true, 0, nullptr));
	std::puts("Studio model validation tests passed.");
}
