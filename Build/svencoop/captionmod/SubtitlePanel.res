"CaptionMod/SubtitlePanel.res"
{
	"Subtitle"
	{
		//DO NOT CHANGE THIS
		"ControlName"	"SubtitlePanel"
		"fieldName"		"Subtitle"
		"autoResize"	"1"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"

		//the Subtitle Panel's position, c-(wide/2) means center-align for the subtitle panel, scaling by (screen height/480).
		"xpos"			"c-250"
		"ypos"			"250"

		//the Subtitle Panel's size, scaling by (screen height/480).
		"wide"			"500"
		"tall"			"200"

		//Custom params for subtitles...
		"cornorsize"	"8"					//Size of Round Cornor, scaling by (screen height/480).
		"xspace"		"12"				//Top/Bottom-Margin, scaling by (screen height/480).
		"yspace"		"8"					//Left/Right-Margin, scaling by (screen height/480).
		"linespace"		"8"					//Line space, scaling by (screen height/480).
		"panelcolor"	"ControlBG" 		//Subtitle panel's background color
		"textfont"		"SubtitleFont"		//Text font
		"textalign"		"L"					//Text Alignment, L/C/R
	}
}
