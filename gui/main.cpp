#include <QApplication>

#include "gui/MainWindow.h"
#include "gui/Theme.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Application identity, set before MainWindow is created: QSettings (the
    // "most recently used data directory" store) keys itself off it.
    app.setOrganizationName(QStringLiteral("COS202"));
    app.setApplicationName(QStringLiteral("SCAPMS"));

    scapms::gui::theme::applyTheme(app);  // Fusion + light stylesheet + icon

    scapms::gui::MainWindow window;
    window.show();

    return app.exec();
}
