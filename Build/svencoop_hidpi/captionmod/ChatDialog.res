"captionmod/ChatDialog.res"
{
	"chat"
	{
		"ControlName"		"EditablePanel"
		"fieldName" 		"chat"
		"visible" 		"1"
		"enabled" 		"1"
		"xpos"			"60"
		"ypos"			"r+360"
		"wide"	 		"400"
		"tall"	 		"200"
		"autoResize"	"1"
		"PaintBackgroundType"	"2"
		"settitlebarvisible"	"0"
	}

	"ChatInputLine"
	{
		"ControlName"	"EditablePanel"
		"fieldName" 	"ChatInputLine"
		"visible" 		"1"
		"enabled" 		"1"
		"xpos"			"30"
		"ypos"			"r+160"
		"wide"	 		"380"
		"tall"	 		"2"
		"PaintBackgroundType"	"0"
	}

	"ChatHistory"
	{
		"ControlName"	"RichText"
		"fieldName"		"ChatHistory"
		"xpos"			"10"
		"ypos"			"17"
		"wide"	 		"380"
		"tall"			"75"
		"wrap"			"1"
		"autoResize"	"1"
		"pinCorner"		"1"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		""
		"textAlignment"	"south-west"
		"font"			"ChatFont"
		"maxchars"		"-1"
	}
}
