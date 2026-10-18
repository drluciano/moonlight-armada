import QtQuick 2.9

// Rounded surface used for grid tiles and settings sections. Shows an accent
// ring when the owning control has keyboard/gamepad focus.
Rectangle {
    property bool hovered: false
    property bool focused: false
    property bool pressed: false

    radius: Theme.radiusLarge
    color: pressed ? Theme.surfaceRaised : (hovered || focused) ? Theme.surfaceHover : Theme.surface
    border.width: focused ? 3 : 1
    border.color: focused ? Theme.accent : Theme.outline

    Behavior on color {
        ColorAnimation { duration: Theme.animationFast }
    }
    Behavior on border.color {
        ColorAnimation { duration: Theme.animationFast }
    }
}
