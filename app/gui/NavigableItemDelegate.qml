import QtQuick 2.0
import QtQuick.Controls 2.2

ItemDelegate {
    property GridView grid

    highlighted: grid.activeFocus && grid.currentItem === this
    hoverEnabled: true

    // Lift the focused tile slightly so it's easy to spot from across the
    // room or on a small handheld screen.
    scale: highlighted ? 1.04 : (hovered ? 1.02 : 1.0)
    z: highlighted ? 1 : 0

    Behavior on scale {
        NumberAnimation { duration: Theme.animationNormal; easing.type: Easing.OutCubic }
    }

    background: Card {
        hovered: parent.hovered
        focused: parent.highlighted
        pressed: parent.down
    }

    Keys.onLeftPressed: {
        grid.moveCurrentIndexLeft()
    }
    Keys.onRightPressed: {
        grid.moveCurrentIndexRight()
    }
    Keys.onDownPressed: {
        grid.moveCurrentIndexDown()
    }
    Keys.onUpPressed: {
        grid.moveCurrentIndexUp()

        // If we've reached the top of the grid, move focus to the toolbar
        if (grid.currentItem === this) {
            nextItemInFocusChain(false).forceActiveFocus(Qt.TabFocus)
        }
    }
    Keys.onReturnPressed: {
        clicked()
    }
    Keys.onEnterPressed: {
        clicked()
    }
}
