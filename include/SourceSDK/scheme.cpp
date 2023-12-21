#include <vgui/IScheme.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : font - 
// Output : char const
//-----------------------------------------------------------------------------
char const *IScheme::GetFontName( const HFont& font )
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Reload the fonts in all schemes
//-----------------------------------------------------------------------------
void ISchemeManager::ReloadFonts()
{
}

// first scheme loaded becomes the default scheme, and all subsequent loaded scheme are derivitives of that
HScheme ISchemeManager::LoadSchemeFromFileEx(VPANEL sizingPanel, const char *fileName, const char *tag)
{
	return 0;
}

// gets the proportional coordinates for doing screen-size independant panel layouts
// use these for font, image and panel size scaling (they all use the pixel height of the display for scaling)
int ISchemeManager::GetProportionalScaledValueEx( HScheme scheme, int normalizedValue )
{
	return 0;
}

int ISchemeManager::GetProportionalNormalizedValueEx( HScheme scheme, int scaledValue )
{
	return 0;
}