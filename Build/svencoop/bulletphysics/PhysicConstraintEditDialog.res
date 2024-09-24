"bulletphysics/PhysicConstraintEditDialog.res"
{
    "PhysicConstraintEditDialog"
    {
        "ControlName"       "Frame"
        "fieldName"         "PhysicConstraintEditDialog"
        "xpos"              "20"
        "ypos"              "20"
        "wide"              "800"
        "tall"              "560"
        "autoResize"        "0"
        "pinCorner"         "0"
        "visible"           "1"
        "enabled"           "1"
        "tabPosition"       "0"
        "title"             "#BulletPhysics_ConstraintEditor"
    }

    // Name Label and TextEntry
    "NameLabel"
    {
        "ControlName"       "Label"
        "fieldName"         "NameLabel"
        "xpos"              "20"
        "ypos"              "40"
        "wide"              "60"
        "tall"              "24"
        "labelText"         "#BulletPhysics_Name"
        "textAlignment"     "west"
    }
    "Name"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "Name"
        "xpos"              "80"
        "ypos"              "40"
        "wide"              "180"
        "tall"              "24"
        "tabPosition"       "1"
    }

    // Debug Draw Level Label and TextEntry
    "DebugDrawLevelLabel"
    {
        "ControlName"       "Label"
        "fieldName"         "DebugDrawLevelLabel"
        "xpos"              "280"
        "ypos"              "40"
        "wide"              "100"
        "tall"              "24"
        "labelText"         "#BulletPhysics_DebugDrawLevel"
        "textAlignment"     "west"
    }
    "DebugDrawLevel"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "DebugDrawLevel"
        "xpos"              "380"
        "ypos"              "40"
        "wide"              "60"
        "tall"              "24"
        "tabPosition"       "2"
    }

    // Type ComboBox
    "TypeLabel"
    {
        "ControlName"       "Label"
        "fieldName"         "TypeLabel"
        "xpos"              "20"
        "ypos"              "80"
        "wide"              "60"
        "tall"              "24"
        "labelText"         "#BulletPhysics_Type"
        "textAlignment"     "west"
    }
    "Type"
    {
        "ControlName"       "ComboBox"
        "fieldName"         "Type"
        "xpos"              "80"
        "ypos"              "80"
        "wide"              "180"
        "tall"              "24"
        "tabPosition"       "3"
		"textHidden"	"0"
		"editable"		"0"
		"maxchars"		"-1"
    }

    "RotOrderLabel"
    {
        "ControlName"       "Label"
        "fieldName"         "RotOrderLabel"
        "xpos"              "280"
        "ypos"              "80"
        "wide"              "100"
        "tall"              "24"
        "labelText"         "#BulletPhysics_RotOrder"
        "textAlignment"     "west"
    }
    "RotOrder"
    {
        "ControlName"       "ComboBox"
        "fieldName"         "RotOrder"
        "xpos"              "380"
        "ypos"              "80"
        "wide"              "60"
        "tall"              "24"
        "tabPosition"       "4"
		"textHidden"	"0"
		"editable"		"0"
		"maxchars"		"-1"
    }

    // RigidBody A and B ComboBoxes
    "RigidBodyALabel"
    {
        "ControlName"       "Label"
        "fieldName"         "RigidBodyALabel"
        "xpos"              "20"
        "ypos"              "120"
        "wide"              "60"
        "tall"              "24"
        "labelText"         "#BulletPhysics_RigidBodyA"
        "textAlignment"     "west"
    }
    "RigidBodyA"
    {
        "ControlName"       "ComboBox"
        "fieldName"         "RigidBodyA"
        "xpos"              "80"
        "ypos"              "120"
        "wide"              "180"
        "tall"              "24"
        "tabPosition"       "5"
    }
    "RigidBodyBLabel"
    {
        "ControlName"       "Label"
        "fieldName"         "RigidBodyBLabel"
        "xpos"              "280"
        "ypos"              "120"
        "wide"              "60"
        "tall"              "24"
        "labelText"         "#BulletPhysics_RigidBodyB"
        "textAlignment"     "west"
    }
    "RigidBodyB"
    {
        "ControlName"       "ComboBox"
        "fieldName"         "RigidBodyB"
        "xpos"              "340"
        "ypos"              "120"
        "wide"              "140"
        "tall"              "24"
        "tabPosition"       "6"
    }

    // Origin A X, Y, Z TextEntries
    "OriginALabel"
    {
        "ControlName"       "Label"
        "fieldName"         "OriginALabel"
        "xpos"              "20"
        "ypos"              "160"
        "wide"              "60"
        "tall"              "24"
        "labelText"         "#BulletPhysics_OriginA"
        "textAlignment"     "west"
    }
    "OriginAX"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "OriginAX"
        "xpos"              "80"
        "ypos"              "160"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "7"
    }
    "OriginAY"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "OriginAY"
        "xpos"              "140"
        "ypos"              "160"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "8"
    }
    "OriginAZ"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "OriginAZ"
        "xpos"              "200"
        "ypos"              "160"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "9"
    }

    // Angles A X, Y, Z TextEntries
    "AnglesALabel"
    {
        "ControlName"       "Label"
        "fieldName"         "AnglesALabel"
        "xpos"              "20"
        "ypos"              "200"
        "wide"              "60"
        "tall"              "24"
        "labelText"         "#BulletPhysics_AnglesA"
        "textAlignment"     "west"
    }
    "AnglesAX"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "AnglesAX"
        "xpos"              "80"
        "ypos"              "200"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "10"
    }
    "AnglesAY"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "AnglesAY"
        "xpos"              "140"
        "ypos"              "200"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "11"
    }
    "AnglesAZ"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "AnglesAZ"
        "xpos"              "200"
        "ypos"              "200"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "12"
    }

// Origin B X, Y, Z TextEntries
    "OriginBLabel"
    {
        "ControlName"       "Label"
        "fieldName"         "OriginBLabel"
        "xpos"              "280"
        "ypos"              "160"
        "wide"              "60"
        "tall"              "24"
        "labelText"         "#BulletPhysics_OriginB"
        "textAlignment"     "west"
    }
    "OriginBX"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "OriginBX"
        "xpos"              "340"
        "ypos"              "160"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "13"
    }
    "OriginBY"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "OriginBY"
        "xpos"              "400"
        "ypos"              "160"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "14"
    }
    "OriginBZ"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "OriginBZ"
        "xpos"              "460"
        "ypos"              "160"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "15"
    }

    // Angles B X, Y, Z TextEntries
    "AnglesBLabel"
    {
        "ControlName"       "Label"
        "fieldName"         "AnglesBLabel"
        "xpos"              "280"
        "ypos"              "200"
        "wide"              "60"
        "tall"              "24"
        "labelText"         "#BulletPhysics_AnglesB"
        "textAlignment"     "west"
    }
    "AnglesBX"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "AnglesBX"
        "xpos"              "340"
        "ypos"              "200"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "16"
    }
    "AnglesBY"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "AnglesBY"
        "xpos"              "400"
        "ypos"              "200"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "17"
    }
    "AnglesBZ"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "AnglesBZ"
        "xpos"              "460"
        "ypos"              "200"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "18"
    }

// MaxTolerantLinearError TextEntries
    "MaxTolerantLinearErrorLabel"
    {
        "ControlName"       "Label"
        "fieldName"         "MaxTolerantLinearErrorLabel"
        "xpos"              "20"
        "ypos"              "240"
        "wide"              "160"
        "tall"              "24"
        "labelText"         "#BulletPhysics_MaxTolerantLinearError"
        "textAlignment"     "west"
    }
    "MaxTolerantLinearError"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "MaxTolerantLinearError"
        "xpos"              "180"
        "ypos"              "240"
        "wide"              "60"
        "tall"              "24"
        "tabPosition"       "31"
    }

    // Forward X, Y, Z TextEntries
    "ForwardLabel"
    {
        "ControlName"       "Label"
        "fieldName"         "ForwardLabel"
        "xpos"              "280"
        "ypos"              "240"
        "wide"              "60"
        "tall"              "24"
        "labelText"         "#BulletPhysics_Forward"
        "textAlignment"     "west"
    }
    "ForwardX"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "ForwardX"
        "xpos"              "340"
        "ypos"              "240"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "32"
    }
    "ForwardY"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "ForwardY"
        "xpos"              "400"
        "ypos"              "240"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "33"
    }
    "ForwardZ"
    {
        "ControlName"       "TextEntry"
        "fieldName"         "ForwardZ"
        "xpos"              "460"
        "ypos"              "240"
        "wide"              "50"
        "tall"              "24"
        "tabPosition"       "34"
    }

//CheckButton
    
    "DisableCollision"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "DisableCollision"
        "xpos"              "20"
        "ypos"              "300"  // Adjusted from 240
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "19"
        "labelText"         "#BulletPhysics_DisableCollision"
        "textAlignment"     "west"
    }

    "UseGlobalJointFromA"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "UseGlobalJointFromA"
        "xpos"              "290"
        "ypos"              "300"  // Adjusted from 240
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "20"
        "labelText"         "#BulletPhysics_UseGlobalJointFromA"
        "textAlignment"     "west"
    }

    "UseLookAtOther"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "UseLookAtOther"
        "xpos"              "20"
        "ypos"              "330"  // Adjusted from 270
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "21"
        "labelText"         "#BulletPhysics_UseLookAtOther"
        "textAlignment"     "west"
    }

    "UseGlobalJointOriginFromOther"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "UseGlobalJointOriginFromOther"
        "xpos"              "290"
        "ypos"              "330"  // Adjusted from 270
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "22"
        "labelText"         "#BulletPhysics_UseGlobalJointOriginFromOther"
        "textAlignment"     "west"
    }

    "UseRigidBodyDistanceAsLinearLimit"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "UseRigidBodyDistanceAsLinearLimit"
        "xpos"              "20"
        "ypos"              "360"  // Adjusted from 300
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "23"
        "labelText"         "#BulletPhysics_UseRigidBodyDistanceAsLinearLimit"
        "textAlignment"     "west"
    }

    "UseLinearReferenceFrameA"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "UseLinearReferenceFrameA"
        "xpos"              "290"
        "ypos"              "360"  // Adjusted from 300
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "24"
        "labelText"         "#BulletPhysics_UseLinearReferenceFrameA"
        "textAlignment"     "west"
    }

    "Barnacle"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "Barnacle"
        "xpos"              "20"
        "ypos"              "390"  // Adjusted from 330
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "25"
        "labelText"         "#BulletPhysics_Barnacle"
        "textAlignment"     "west"
    }

    "Gargantua"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "Gargantua"
        "xpos"              "290"
        "ypos"              "390"  // Adjusted from 330
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "26"
        "labelText"         "#BulletPhysics_Gargantua"
        "textAlignment"     "west"
    }

    "DeactiveOnNormalActivity"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "DeactiveOnNormalActivity"
        "xpos"              "20"
        "ypos"              "420"  // Adjusted from 360
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "27"
        "labelText"         "#BulletPhysics_DeactiveOnNormalActivity"
        "textAlignment"     "west"
    }

    "DeactiveOnDeathActivity"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "DeactiveOnDeathActivity"
        "xpos"              "290"
        "ypos"              "420"  // Adjusted from 360
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "28"
        "labelText"         "#BulletPhysics_DeactiveOnDeathActivity"
        "textAlignment"     "west"
    }

    "DeactiveOnBarnacleActivity"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "DeactiveOnBarnacleActivity"
        "xpos"              "20"
        "ypos"              "450"  // Adjusted from 390
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "29"
        "labelText"         "#BulletPhysics_DeactiveOnBarnacleActivity"
        "textAlignment"     "west"
    }

    "DeactiveOnGargantuaActivity"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "DeactiveOnGargantuaActivity"
        "xpos"              "290"
        "ypos"              "450"  // Adjusted from 390
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "30"
        "labelText"         "#BulletPhysics_DeactiveOnGargantuaActivity"
        "textAlignment"     "west"
    }

    "DontResetPoseOnErrorCorrection"
    {
        "ControlName"       "CheckButton"
        "fieldName"         "DontResetPoseOnErrorCorrection"
        "xpos"              "20"
        "ypos"              "480"  // Adjusted from 420
        "wide"              "250"
        "tall"              "24"
        "tabPosition"       "31"
        "labelText"         "#BulletPhysics_DontResetPoseOnErrorCorrection"
        "textAlignment"     "west"
    }

	"PhysicFactorListPanel"
	{
		"ControlName"		"ListPanel"
		"fieldName"		"PhysicFactorListPanel"
		"xpos"		"520"
		"ypos"		"40"
		"wide"		"260"
		"tall"		"500"
		"AutoResize"	"3"
		"PinCorner"		"3"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"paintbackground"		"1"
	}

    // OK, Cancel, and Apply Buttons (these entries are unchanged as per your request)
    "OK"
    {
        "ControlName"       "Button"
        "fieldName"         "OK"
        "xpos"              "180"
        "ypos"              "520"
        "wide"              "72"
        "tall"              "24"
        "autoResize"        "0"
        "pinCorner"         "0"
        "visible"           "1"
        "enabled"           "1"
        "tabPosition"       "24"
        "labelText"         "#GameUI_OK"
        "textAlignment"     "west"
        "dulltext"          "0"
        "command"           "OK"
        "default"           "1"
    }
    "Cancel"
    {
        "ControlName"       "Button"
        "fieldName"         "Cancel"
        "xpos"              "260"
        "ypos"              "520"
        "wide"              "72"
        "tall"              "24"
        "autoResize"        "0"
        "pinCorner"         "0"
        "visible"           "1"
        "enabled"           "1"
        "tabPosition"       "25"
        "labelText"         "#GameUI_Cancel"
        "textAlignment"     "west"
        "dulltext"          "0"
        "command"           "Close"
        "default"           "0"
    }
    "Apply"
    {
        "ControlName"       "Button"
        "fieldName"         "Apply"
        "xpos"              "340"
        "ypos"              "520"
        "wide"              "72"
        "tall"              "24"
        "autoResize"        "0"
        "pinCorner"         "0"
        "visible"           "1"
        "enabled"           "1"
        "tabPosition"       "26"
        "labelText"         "#GameUI_Apply"
        "textAlignment"     "west"
        "dulltext"          "0"
        "command"           "Apply"
    }
}