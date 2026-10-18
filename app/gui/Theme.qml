pragma Singleton
import QtQuick 2.9

// Central design tokens for the Moonlight Armada UI. Every view pulls its
// colors, radii and spacing from here so the look stays consistent and can
// be tweaked in one place.
QtObject {
    // Surfaces, from the window background up to raised cards
    readonly property color background: "#0D1017"
    readonly property color backgroundGradientEnd: "#151A2B"
    readonly property color surface: "#171C28"
    readonly property color surfaceRaised: "#1F2533"
    readonly property color surfaceHover: "#262E40"
    readonly property color outline: "#2C3447"

    // Brand accent (also used for keyboard/gamepad focus)
    readonly property color accent: "#4FC3F7"
    readonly property color accentStrong: "#29B6F6"
    readonly property color accentSoft: "#1A4FC3F7"

    // Text
    readonly property color textPrimary: "#F2F5FA"
    readonly property color textSecondary: "#A3ADC2"
    readonly property color textMuted: "#6B768C"

    // Status colors
    readonly property color success: "#4ADE80"
    readonly property color warning: "#FBBF24"
    readonly property color danger: "#F87171"

    // Shape
    readonly property int radiusSmall: 8
    readonly property int radiusMedium: 14
    readonly property int radiusLarge: 20

    // Spacing
    readonly property int spacingSmall: 6
    readonly property int spacingMedium: 12
    readonly property int spacingLarge: 20

    // Motion
    readonly property int animationFast: 120
    readonly property int animationNormal: 200

    // Typography
    readonly property int fontTitle: 22
    readonly property int fontHeading: 15
    readonly property int fontBody: 12
    readonly property int fontCaption: 10
}
