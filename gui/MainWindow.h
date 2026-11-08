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

// Tabbed application shell. Owns the tab widget, the File/Edit/Admin menus,
// the toolbar, the status bar (last action / undo depth / algorithm used),
// and the single ClinicManager that every panel receives injected (architecture:
// GUI -> controller -> database; panels never touch storage directly). Also
// coordinates session workflows around the panels: startup auto-load,
// unsaved-changes protection, persistence commands, and sample data loading.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    // Out-of-line destructor because ClinicManager is only forward-declared
    // here (unique_ptr member) - defined in MainWindow.cpp.
    ~MainWindow();

public slots:
    // Status-bar feed. Panels report through their actionPerformed /
    // statusMessage / algorithmUsed signals; these keep the bar updated.
    void showLastAction(const QString& action);
    void setUndoDepth(int depth);
    void setAlgorithmUsed(const QString& algorithm);

private slots:
    // File operations.
    void onFileNew();
    void onLoadSampleData();
    void onSave();
    void onSaveAs();
    void onLoad();
    void onQuit();
    // Admin operations.
    void onClearAllData();

private:
    void buildTabs();
    void buildMenusAndActions();
    void buildToolBar();
    void buildStatusBar();
    // Single refresh funnel: any panel's dataChanged (and an Undo, through
    // AdminPanel::undoLast -> dataChanged) re-reads the database into every
    // wired panel so no table goes stale, and the undo-depth badge updates.
    void refreshDataViews();

    // Persistence and startup behavior. `startup` changes only the failure
    // mode: a FileError at startup means "start empty" with a status-bar note,
    // while an explicit Load shows an error dialog.
    void loadDataDir(const QString& dataDir, bool startup);
    // Confirmation dialog before discarding unsaved changes (quit, Load,
    // sample data). Returns true when clinic staff choose to discard.
    bool confirmDiscardChanges(const QString& action);

    // The sample-data loader (File -> Load Sample Data). Loads the sample
    // records from CSVs in data/; if absent, falls back to in-memory seed data.
    // In either case, the waiting queue is session state, so seedSampleQueue
    // queues the initial patients.
    void seedSampleQueue();
    // In-memory fallback for the sample scenario when CSV files are not found.
    void seedSampleData();

    // Declared first: panels hold a reference to the manager owned here
    // (dependency injection - panels never construct their own manager).
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

    // The data directory Save/Save As write to and Load reads from.
    // The most recently used directory is stored in QSettings and
    // auto-loaded at startup; falls back to the default "data" directory.
    QString dataDir_ = QStringLiteral("data");
    // Change tracker: true when the in-memory clinic state differs from disk.
    bool dirty_ = false;
};

}  // namespace scapms::gui
