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
		"fadein"		"0.3"				//Text fade in duration.
		"fadeout"		"0.3"				//Text fade out duration.
		"holdtime"		"10.0"				//Text lifetime as default.

		//Add speaker's name to the head of subtitle text?
		"prefix" 		 "0"
		
		//Wait for all former lines to be played before playing a new line, 1 for wait and 0 for don't wait.
		"waitplay"		"0"

		//The bigger this value is, the longer it will take before displaying a queued lines.
		//The default value is 1, if set to 0, all lines will be displayed instantly. If set to 1, lines will gradually disappear line by line.
		"stimescale"	"1.0"

		//This value scales the holdtime. default value = 1.0, if set to 0 or negative, 1.0 will be used.
		"htimescale"	"1.0"
	}
}
