import QtQuick 2.9
import QtQuick.Controls 2.2

// Small pill with a colored dot, e.g. "● Online"
Rectangle {
    property alias text: chipLabel.text
    property color dotColor: Theme.success

    implicitWidth: chipRow.implicitWidth + 2 * Theme.spacingMedium
    implicitHeight: chipRow.implicitHeight + Theme.spacingSmall
    width: implicitWidth
    height: implicitHeight
    radius: height / 2
    color: Theme.surfaceRaised
    border.width: 1
    border.color: Theme.outline

    Row {
        id: chipRow
        anchors.centerIn: parent
        spacing: Theme.spacingSmall

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 8
            height: 8
            radius: 4
            color: dotColor
        }

        Label {
            id: chipLabel
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: Theme.fontCaption
            font.bold: true
            color: Theme.textSecondary
        }
    }
}
