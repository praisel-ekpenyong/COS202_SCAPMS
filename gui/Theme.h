#pragma once

#include <QColor>
#include <QString>

class QApplication;

namespace scapms::gui {

// Application theme colors and stylesheet.
namespace theme {

// Category colors: Emergency red, Follow-Up green, General neutral.
inline constexpr QColor kCategoryGeneral{0x5F, 0x6A, 0x7A};    // neutral slate
inline constexpr QColor kCategoryEmergency{0xC0, 0x2B, 0x2B};  // red
inline constexpr QColor kCategoryFollowUp{0x2E, 0x7D, 0x4F};   // green

// Status colors: pending amber, completed green, cancelled muted.
inline constexpr QColor kStatusPending{0xB5, 0x7A, 0x00};   // amber
inline constexpr QColor kStatusCompleted{0x2E, 0x7D, 0x4F}; // green
inline constexpr QColor kStatusCancelled{0x8A, 0x8F, 0x98}; // muted grey

inline constexpr QColor kAccent{0x1F, 0x6F, 0x8B};

// Maps category/status name to its theme color.
QColor categoryColor(const QString& category);
QColor statusColor(const QString& status);

const QString& appStyleSheet();

// Applies theme and stylesheet to application.
void applyTheme(QApplication& app);

}  // namespace theme

}  // namespace scapms::gui
