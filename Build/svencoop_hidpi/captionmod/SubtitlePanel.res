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

		//the Subtitle Panel's position, c-(wide/2) means center-align for the subtitle panel, scaling by (screen height/720).
		"xpos"			"c-375"
		"ypos"			"375"

		//the Subtitle Panel's size, scaling by (screen height/720).
		"wide"			"750"
		"tall"			"300"

		//Custom params for subtitles...
		"cornorsize"	"12"				//Size of Round Cornor, scaling by (screen height/480).
		"xspace"		"18"				//Top/Bottom-Margin, scaling by (screen height/480).
		"yspace"		"12"				//Left/Right-Margin, scaling by (screen height/480).
		"linespace"		"12"				//Line space, scaling by (screen height/480).
		"panelcolor"	"ControlBG" 		//Subtitle panel's background color
		"textfont"		"SubtitleFont"		//Text font
		"textalign"		"L"					//Text Alignment, L/C/R
	}
}
