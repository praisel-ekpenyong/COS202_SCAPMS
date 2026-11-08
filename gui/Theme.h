#pragma once

#include <QColor>
#include <QString>

class QApplication;

namespace scapms::gui {

// Central theme: Fusion style + light stylesheet, with every
// category/status colour defined in this one place so all panels stay
// consistent. Panels reference these constants; none of them invent colours.
namespace theme {

// Patient category colours: Emergency red, Follow-Up green, General
// neutral.
inline constexpr QColor kCategoryGeneral{0x5F, 0x6A, 0x7A};    // neutral slate
inline constexpr QColor kCategoryEmergency{0xC0, 0x2B, 0x2B};  // red
inline constexpr QColor kCategoryFollowUp{0x2E, 0x7D, 0x4F};   // green

// Appointment status colours: pending amber, completed green, cancelled
// muted.
inline constexpr QColor kStatusPending{0xB5, 0x7A, 0x00};   // amber
inline constexpr QColor kStatusCompleted{0x2E, 0x7D, 0x4F}; // green
inline constexpr QColor kStatusCancelled{0x8A, 0x8F, 0x98}; // muted grey

// Accent used for links/selection highlights in the light stylesheet.
inline constexpr QColor kAccent{0x1F, 0x6F, 0x8B};

// Maps a category/status name (as stored in the CSV files: "General",
// "Emergency", "FollowUp", "pending", "completed", "cancelled") to its theme
// colour; unknown names fall back to the neutral values so a bad row can never
// paint garbage.
QColor categoryColor(const QString& category);
QColor statusColor(const QString& status);

// The shared light stylesheet. Built once, queried by applyTheme.
const QString& appStyleSheet();

// Applies Fusion style, the light stylesheet, and the application icon.
// Call once from main after constructing QApplication.
void applyTheme(QApplication& app);

}  // namespace theme

}  // namespace scapms::gui
