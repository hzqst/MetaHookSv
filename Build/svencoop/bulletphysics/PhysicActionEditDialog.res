"bulletphysics/PhysicActionEditDialog.res"
{
    "PhysicActionEditDialog"
    {
        "ControlName"   "Frame"
        "fieldName"     "PhysicActionEditDialog"
        "xpos"          "20"
        "ypos"          "20"
        "wide"          "520"
        "tall"          "560"
        "autoResize"    "0"
        "pinCorner"     "0"
        "visible"       "1"
        "enabled"       "1"
        "tabPosition"   "0"
        "title"         "#BulletPhysics_ActionEditor"
    }

    // Name and DebugDrawLevel on the same row
    "NameLabel"
    {
        "ControlName"   "Label"
        "fieldName"     "NameLabel"
        "xpos"          "20"
        "ypos"          "40"
        "wide"          "60"
        "tall"          "24"
        "labelText"     "#BulletPhysics_Name"
        "textAlignment" "west"
    }
    "Name"
    {
        "ControlName"   "TextEntry"
        "fieldName"     "Name"
        "xpos"          "80"
        "ypos"          "40"
        "wide"          "360"
        "tall"          "24"
        "tabPosition"   "1"
    }
    "DebugDrawLevelLabel"
    {
        "ControlName"   "Label"
        "fieldName"     "DebugDrawLevelLabel"
        "xpos"          "280"
        "ypos"          "80"
        "wide"          "100"
        "tall"          "24"
        "labelText"     "#BulletPhysics_DebugDrawLevel"
        "textAlignment" "west"
    }
    "DebugDrawLevel"
    {
        "ControlName"   "TextEntry"
        "fieldName"     "DebugDrawLevel"
        "xpos"          "380"
        "ypos"          "80"
        "wide"          "60"
        "tall"          "24"
        "tabPosition"   "2"
    }

    // Type on the row between Name/DebugDrawLevel and RigidBody/Constraint
    "TypeLabel"
    {
        "ControlName"   "Label"
        "fieldName"     "TypeLabel"
        "xpos"          "20"
        "ypos"          "80"
        "wide"          "60"
        "tall"          "24"
        "labelText"     "#BulletPhysics_Type"
        "textAlignment" "west"
    }
    "Type"
    {
        "ControlName"   "ComboBox"
        "fieldName"     "Type"
        "xpos"          "80"
        "ypos"          "80"
        "wide"          "180"
        "tall"          "24"
        "tabPosition"   "3"
        "textHidden"    "0"
        "editable"      "0"
        "maxchars"      "-1"
    }

    // RigidBody and Constraint on the same row
    "RigidBodyLabel"
    {
        "ControlName"   "Label"
        "fieldName"     "RigidBodyLabel"
        "xpos"          "20"
        "ypos"          "120"
        "wide"          "60"
        "tall"          "24"
        "labelText"     "#BulletPhysics_RigidBody"
        "textAlignment" "west"
    }
    "RigidBody"
    {
        "ControlName"   "ComboBox"
        "fieldName"     "RigidBody"
        "xpos"          "80"
        "ypos"          "120"
        "wide"          "180"
        "tall"          "24"
        "tabPosition"   "4"
    }
    "ConstraintLabel"
    {
        "ControlName"   "Label"
        "fieldName"     "ConstraintLabel"
        "xpos"          "20"
        "ypos"          "120"
        "wide"          "60"
        "tall"          "24"
        "labelText"     "#BulletPhysics_Constraint"
        "textAlignment" "west"
    }
    "Constraint"
    {
        "ControlName"   "ComboBox"
        "fieldName"     "Constraint"
        "xpos"          "80"
        "ypos"          "120"
        "wide"          "360"
        "tall"          "24"
        "tabPosition"   "5"
    }

    // OriginX, OriginY, OriginZ on the same row
    "OriginLabel"
    {
        "ControlName"   "Label"
        "fieldName"     "OriginLabel"
        "xpos"          "20"
        "ypos"          "160"
        "wide"          "60"
        "tall"          "24"
        "labelText"     "#BulletPhysics_Origin"
        "textAlignment" "west"
    }
    "OriginX"
    {
        "ControlName"   "TextEntry"
        "fieldName"     "OriginX"
        "xpos"          "80"
        "ypos"          "160"
        "wide"          "50"
        "tall"          "24"
        "tabPosition"   "6"
    }
    "OriginY"
    {
        "ControlName"   "TextEntry"
        "fieldName"     "OriginY"
        "xpos"          "140"
        "ypos"          "160"
        "wide"          "50"
        "tall"          "24"
        "tabPosition"   "7"
    }
    "OriginZ"
    {
        "ControlName"   "TextEntry"
        "fieldName"     "OriginZ"
        "xpos"          "200"
        "ypos"          "160"
        "wide"          "50"
        "tall"          "24"
        "tabPosition"   "8"
    }

    // AnglesX, AnglesY, AnglesZ on the same row
    "AnglesLabel"
    {
        "ControlName"   "Label"
        "fieldName"     "AnglesLabel"
        "xpos"          "20"
        "ypos"          "200"
        "wide"          "60"
        "tall"          "24"
        "labelText"     "#BulletPhysics_Angles"
        "textAlignment" "west"
    }
    "AnglesX"
    {
        "ControlName"   "TextEntry"
        "fieldName"     "AnglesX"
        "xpos"          "80"
        "ypos"          "200"
        "wide"          "50"
        "tall"          "24"
        "tabPosition"   "9"
    }
    "AnglesY"
    {
        "ControlName"   "TextEntry"
        "fieldName"     "AnglesY"
        "xpos"          "140"
        "ypos"          "200"
        "wide"          "50"
        "tall"          "24"
        "tabPosition"   "10"
    }
    "AnglesZ"
    {
        "ControlName"   "TextEntry"
        "fieldName"     "AnglesZ"
        "xpos"          "200"
        "ypos"          "200"
        "wide"          "50"
        "tall"          "24"
        "tabPosition"   "11"
    }

    // Barnacle and Gargantua CheckButtons on the same row
    "Barnacle"
    {
        "ControlName"   "CheckButton"
        "fieldName"     "Barnacle"
        "xpos"          "20"
        "ypos"          "240"
        "wide"          "250"
        "tall"          "24"
        "tabPosition"   "12"
        "labelText"     "#BulletPhysics_Barnacle"
        "textAlignment" "west"
    }
    "Gargantua"
    {
        "ControlName"   "CheckButton"
        "fieldName"     "Gargantua"
        "xpos"          "290"
        "ypos"          "240"
        "wide"          "250"
        "tall"          "24"
        "tabPosition"   "13"
        "labelText"     "#BulletPhysics_Gargantua"
        "textAlignment" "west"
    }

    // PhysicFactorListPanel in the bottom half (height adjusted)
    "PhysicFactorListPanel"
    {
        "ControlName"   "ListPanel"
        "fieldName"     "PhysicFactorListPanel"
        "xpos"          "20"
        "ypos"          "280"
        "wide"          "480"
        "tall"          "220"  // Height adjusted to avoid overlapping with buttons
        "autoResize"    "3"
        "pinCorner"     "3"
        "visible"       "1"
        "enabled"       "1"
        "tabPosition"   "0"
        "paintbackground" "1"
    }

    // OK, Cancel, and Apply Buttons slightly moved right
    "OK"
    {
        "ControlName"   "Button"
        "fieldName"     "OK"
        "xpos"          "260"   // Shifted to the right
        "ypos"          "520"
        "wide"          "72"
        "tall"          "24"
        "tabPosition"   "14"
		"pinCorner"		"3"
        "labelText"     "#GameUI_OK"
        "command"       "OK"
        "default"       "1"
    }
    "Cancel"
    {
        "ControlName"   "Button"
        "fieldName"     "Cancel"
        "xpos"          "340"   // Shifted to the right
        "ypos"          "520"
        "wide"          "72"
        "tall"          "24"
        "tabPosition"   "15"
		"pinCorner"		"3"
        "labelText"     "#GameUI_Cancel"
        "command"       "Close"
    }
    "Apply"
    {
        "ControlName"   "Button"
        "fieldName"     "Apply"
        "xpos"          "420"   // Shifted to the right
        "ypos"          "520"
        "wide"          "72"
        "tall"          "24"
        "tabPosition"   "16"
		"pinCorner"		"3"
        "labelText"     "#GameUI_Apply"
        "command"       "Apply"
    }
}
