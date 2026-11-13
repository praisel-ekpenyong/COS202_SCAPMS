#pragma once

#include <QMainWindow>

#include <memory>

class QTabWidget;
class QLabel;
class QAction;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

// Main window application shell. Manages tabs, menus, toolbar,
// and coordinates clinic operations across panels.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

public slots:
    void showLastAction(const QString& action);
    void setUndoDepth(int depth);
    void setAlgorithmUsed(const QString& algorithm);

private slots:
    // File operations
    void onFileNew();
    void onLoadSampleData();
    void onSave();
    void onSaveAs();
    void onLoad();
    void onQuit();
    // Admin operations
    void onClearAllData();

private:
    void buildTabs();
    void buildMenusAndActions();
    void buildToolBar();
    void buildStatusBar();
    void refreshDataViews();

    // Loads data files from data directory.
    void loadDataDir(const QString& dataDir, bool startup);
    // Prompts user before discarding unsaved changes.
    bool confirmDiscardChanges(const QString& action);

    // Loads sample clinic records from data/ or in-memory fallback.
    void seedSampleQueue();
    void seedSampleData();

    std::unique_ptr<controller::ClinicManager> manager_;
    class PatientPanel* patientPanel_ = nullptr;
    class AppointmentPanel* appointmentPanel_ = nullptr;
    class SearchPanel* searchPanel_ = nullptr;
    class DashboardPanel* dashboardPanel_ = nullptr;
    class QueuePanel* queuePanel_ = nullptr;
    class AdminPanel* adminPanel_ = nullptr;

    QTabWidget* tabs_;
    QAction* saveAction_;
    QAction* undoAction_;
    QAction* findAction_;
    QLabel* lastActionLabel_;
    QLabel* undoDepthLabel_;
    QLabel* algorithmLabel_;

    // Current data directory path.
    QString dataDir_ = QStringLiteral("data");
    // True if there are unsaved changes.
    bool dirty_ = false;
};

}  // namespace scapms::gui
