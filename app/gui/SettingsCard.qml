import QtQuick 2.9
import QtQuick.Controls 2.2

// A settings section rendered as a rounded card with an accent heading
GroupBox {
    id: card

    topPadding: cardTitle.implicitHeight + 2 * Theme.spacingMedium
    leftPadding: Theme.spacingLarge
    rightPadding: Theme.spacingLarge
    bottomPadding: Theme.spacingLarge

    background: Rectangle {
        radius: Theme.radiusMedium
        color: Theme.surface
        border.width: 1
        border.color: Theme.outline
    }

    label: Label {
        id: cardTitle
        x: card.leftPadding
        y: Theme.spacingMedium
        width: card.availableWidth
        text: card.title
        color: Theme.accent
        font.pointSize: Theme.fontHeading
        font.bold: true
        elide: Text.ElideRight
    }
}
