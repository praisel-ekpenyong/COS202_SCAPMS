#include <QApplication>

#include "gui/MainWindow.h"
#include "gui/Theme.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Set application name and organization for QSettings
    app.setOrganizationName(QStringLiteral("COS202"));
    app.setApplicationName(QStringLiteral("SCAPMS"));

    scapms::gui::theme::applyTheme(app);

    scapms::gui::MainWindow window;
    window.show();

    return app.exec();
}
