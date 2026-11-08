#include "gui/Theme.h"

#include <QApplication>
#include <QIcon>
#include <QPalette>
#include <QStyleFactory>

#include "utils/AppInfo.h"

namespace scapms::gui::theme {

QColor categoryColor(const QString& category) {
    if (category.compare(QStringLiteral("Emergency"), Qt::CaseInsensitive) == 0)
        return kCategoryEmergency;
    if (category.compare(QStringLiteral("FollowUp"), Qt::CaseInsensitive) == 0 ||
        category.compare(QStringLiteral("Follow-Up"), Qt::CaseInsensitive) == 0)
        return kCategoryFollowUp;
    return kCategoryGeneral;
}

QColor statusColor(const QString& status) {
    if (status.compare(QStringLiteral("pending"), Qt::CaseInsensitive) == 0)
        return kStatusPending;
    if (status.compare(QStringLiteral("completed"), Qt::CaseInsensitive) == 0)
        return kStatusCompleted;
    return kStatusCancelled;
}

const QString& appStyleSheet() {
    static const QString sheet;
    return sheet;
}

void applyTheme(QApplication& app) {
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    // Pin the light palette. Without this, Qt 6 adopts the OS dark
    // palette on dark-mode systems and the hardcoded light stylesheet above
    // produces white-on-white text (selected tab labels, table text, plain
    // labels) - observed live on Windows dark mode.
    QPalette light;
    light.setColor(QPalette::Window, QColor(0xF0, 0xF0, 0xF0));
    light.setColor(QPalette::WindowText, QColor(0x1F, 0x24, 0x28));
    light.setColor(QPalette::Base, QColor(0xFF, 0xFF, 0xFF));
    light.setColor(QPalette::AlternateBase, QColor(0xF8, 0xF8, 0xF8));
    light.setColor(QPalette::Text, QColor(0x1F, 0x24, 0x28));
    light.setColor(QPalette::PlaceholderText, QColor(0x8A, 0x8F, 0x98));
    light.setColor(QPalette::Button, QColor(0xE0, 0xE0, 0xE0));
    light.setColor(QPalette::ButtonText, QColor(0x1F, 0x24, 0x28));
    light.setColor(QPalette::BrightText, Qt::white);
    light.setColor(QPalette::Highlight, theme::kAccent);
    light.setColor(QPalette::HighlightedText, Qt::white);
    light.setColor(QPalette::ToolTipBase, QColor(0xFF, 0xFF, 0xE1));
    light.setColor(QPalette::ToolTipText, QColor(0x1F, 0x24, 0x28));
    light.setColor(QPalette::Link, theme::kAccent);
    app.setPalette(light);
    app.setStyleSheet(appStyleSheet());
    app.setWindowIcon(QIcon(QStringLiteral(":/resources/appicon.png")));
    app.setApplicationName(QString::fromLatin1(utils::applicationName()));
    app.setApplicationVersion(QString::fromLatin1(utils::versionString()));
}

}  // namespace scapms::gui::theme
