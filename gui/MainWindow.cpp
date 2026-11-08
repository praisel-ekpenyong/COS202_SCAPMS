#include "gui/MainWindow.h"

#include <QAction>
#include <QDate>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTabWidget>
#include <QTime>
#include <QToolBar>

#include "controller/ClinicManager.h"
#include "gui/AdminPanel.h"
#include "gui/AppointmentPanel.h"
#include "gui/DashboardPanel.h"
#include "gui/PatientPanel.h"
#include "gui/QueuePanel.h"
#include "gui/ReportPanel.h"
#include "gui/SearchPanel.h"
#include "utils/AppInfo.h"
#include "utils/Exceptions.h"

namespace {

// QSettings key recording the directory of the most recent successful Save /
// Save As / Load, so the startup auto-load resumes where the last
// session left off. The plain QSettings constructor keys the store off the
// application identity main set (org "COS202", app "SCAPMS").
const char kLastDataDirKey[] = "lastDataDir";

// Persists the directory for the next launch's auto-load. The handle is
// created fresh and destructed on return so the write is flushed.
void rememberDataDir(const QString& dataDir) {
    QSettings settings;
    settings.setValue(QString::fromUtf8(kLastDataDirKey), dataDir);
}

}  // namespace

namespace scapms::gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    // One ClinicManager for the whole window: panels receive it
    // injected and mutate only through it.
    manager_ = std::make_unique<controller::ClinicManager>();
    buildTabs();
    buildMenusAndActions();
    buildToolBar();
    buildStatusBar();

    // Edit->Undo and the toolbar Undo share the Admin panel's undo path so
    // there is a single undo implementation: AdminPanel::undoLast
    // calls ClinicManager::undo, and refreshes depth, history, and all panels.
    connect(undoAction_, &QAction::triggered, adminPanel_, &AdminPanel::undoLast);

    setWindowTitle(QStringLiteral("%1 - Smart Clinic Appointment & Patient Management System")
                       .arg(QString::fromLatin1(utils::applicationName())));
    resize(1000, 700);

    setUndoDepth(0);  // initial state: empty undo stack, action disabled

    // Auto-load the most recent data directory at startup. The
    // directory is stored in QSettings (written after every successful
    // Save / Save As / Load) so a restart resumes the last-used data;
    // without one, fall back to the default demo directory. A stored
    // directory that no longer holds its data files - or none at all -
    // lands in the FileError path inside loadDataDir: start empty,
    // without an intrusive error dialog.
    const QSettings settings;
    const QString lastDataDir = settings.value(QString::fromUtf8(kLastDataDirKey)).toString();
    dataDir_ = lastDataDir.isEmpty() ? QStringLiteral("data") : lastDataDir;
    loadDataDir(dataDir_, /*startup=*/true);
}

void MainWindow::buildTabs() {
    // Panels with controller access receive the shell's single ClinicManager
    // (injection pattern: panels never construct their own
    // manager); they never reach storage directly (data flow, Architecture).
    patientPanel_ = new PatientPanel(*manager_, this);
    appointmentPanel_ = new AppointmentPanel(*manager_, this);
    searchPanel_ = new SearchPanel(*manager_, this);
    dashboardPanel_ = new DashboardPanel(manager_.get(), this);
    queuePanel_ = new QueuePanel(manager_.get(), this);
    adminPanel_ = new AdminPanel(manager_.get(), this);
    auto* reportPanel = new ReportPanel(manager_.get(), this);

    tabs_ = new QTabWidget(this);
    tabs_->addTab(dashboardPanel_, tr("Dashboard"));
    tabs_->addTab(patientPanel_, tr("Patient Records"));
    tabs_->addTab(appointmentPanel_, tr("Appointments"));
    tabs_->addTab(queuePanel_, tr("Waiting Queue"));
    tabs_->addTab(searchPanel_, tr("Search & Sort"));
    tabs_->addTab(reportPanel, tr("Reports"));
    tabs_->addTab(adminPanel_, tr("Administration"));
    setCentralWidget(tabs_);

    // Status-bar feed: every wired panel's action lands in "last action";
    // the sort/search and report panels also report the "algorithm used".
    connect(patientPanel_, &PatientPanel::actionPerformed,
            this, &MainWindow::showLastAction);
    connect(appointmentPanel_, &AppointmentPanel::actionPerformed,
            this, &MainWindow::showLastAction);
    connect(searchPanel_, &SearchPanel::actionPerformed,
            this, &MainWindow::showLastAction);
    connect(searchPanel_, &SearchPanel::algorithmUsed,
            this, &MainWindow::setAlgorithmUsed);
    connect(reportPanel, &ReportPanel::algorithmUsed,
            this, &MainWindow::setAlgorithmUsed);
    connect(reportPanel, &ReportPanel::statusMessage,
            this, &MainWindow::showLastAction);
    connect(dashboardPanel_, &DashboardPanel::statusMessage,
            this, &MainWindow::showLastAction);
    connect(queuePanel_, &QueuePanel::statusMessage,
            this, &MainWindow::showLastAction);
    connect(adminPanel_, &AdminPanel::statusMessage,
            this, &MainWindow::showLastAction);

    // Mediator wiring: a mutation through any panel refreshes EVERY panel,
    // so e.g. "Call next" on the Queue tab updates the Dashboard at once.
    // Record mutations also arm the quit-guard (the in-memory clinic now
    // differs from the last-saved data directory). Queue operations change
    // appointment records too (call-next completes one), so they arm it as
    // well - conservative, never silently losing a save prompt.
    connect(patientPanel_, &PatientPanel::dataChanged, this, [this] {
        dirty_ = true;
        refreshDataViews();
    });
    connect(appointmentPanel_, &AppointmentPanel::dataChanged, this, [this] {
        dirty_ = true;
        refreshDataViews();
    });
    connect(queuePanel_, &QueuePanel::dataChanged, this, [this] {
        dirty_ = true;
        refreshDataViews();
    });
    connect(adminPanel_, &AdminPanel::dataChanged, this, [this] {
        dirty_ = true;
        refreshDataViews();
    });

    // Undo bookkeeping ( status bar): the Admin panel re-publishes the
    // undo depth after every recorded operation and after each undo.
    connect(adminPanel_, &AdminPanel::undoDepthChanged,
            this, &MainWindow::setUndoDepth);

    // The Admin panel's maintenance buttons route through the shell so the
    // confirmation dialogs and the manager mutations have a single home.
    connect(adminPanel_, &AdminPanel::clearAllRequested,
            this, &MainWindow::onClearAllData);
    connect(adminPanel_, &AdminPanel::loadSampleRequested,
            this, &MainWindow::onLoadSampleData);
}

// Defined here (not the header) so the unique_ptr<ClinicManager> destructor
// sees the complete type.
MainWindow::~MainWindow() = default;

void MainWindow::buildMenusAndActions() {
    // File menu.
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* newAction =
        fileMenu->addAction(tr("&New"), this, &MainWindow::onFileNew);
    newAction->setShortcut(QKeySequence::New);
    newAction->setToolTip(tr("New session"));

    QAction* loadSampleAction =
        fileMenu->addAction(tr("Load &Sample Data"), this, &MainWindow::onLoadSampleData);
    loadSampleAction->setToolTip(tr("Load sample clinic data"));

    fileMenu->addSeparator();

    saveAction_ = fileMenu->addAction(tr("&Save"), this, &MainWindow::onSave);
    saveAction_->setShortcut(QKeySequence::Save);
    saveAction_->setToolTip(tr("Save clinic data (Ctrl+S)"));

    QAction* saveAsAction =
        fileMenu->addAction(tr("Save &As..."), this, &MainWindow::onSaveAs);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    saveAsAction->setToolTip(tr("Save clinic data as..."));

    QAction* loadAction =
        fileMenu->addAction(tr("&Load..."), this, &MainWindow::onLoad);
    loadAction->setShortcut(QKeySequence::Open);
    loadAction->setToolTip(tr("Load clinic data"));

    fileMenu->addSeparator();

    QAction* quitAction = fileMenu->addAction(tr("E&xit"), this, &MainWindow::onQuit);
    quitAction->setShortcut(QKeySequence::Quit);
    quitAction->setToolTip(tr("Exit application (Ctrl+Q)"));

    // Edit menu: undo recent administrative operations.
    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
    undoAction_ = editMenu->addAction(tr("&Undo"));
    undoAction_->setShortcut(QKeySequence::Undo);
    undoAction_->setToolTip(tr("Undo last action (Ctrl+Z)"));
    undoAction_->setEnabled(false);  // enabled by setUndoDepth when depth > 0

    // Admin menu.
    QMenu* adminMenu = menuBar()->addMenu(tr("&Admin"));
    QAction* clearAction =
        adminMenu->addAction(tr("&Clear All Data..."), this, &MainWindow::onClearAllData);
    clearAction->setToolTip(tr("Clear all clinic data"));

    // Help menu with the About dialog.
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction* aboutAction = helpMenu->addAction(tr("&About SCAPMS"), this, [this] {
        QMessageBox::about(
            this, tr("About SCAPMS"),
            QStringLiteral(
                "<b>%1</b> v%2 - Smart Clinic Appointment & Patient Management System"
                "<br><br>Qt %3, C++17 (CMake, MSYS2 UCRT64).<br>"
                "<br>A clinic management system: patient registration, "
                "appointments, a waiting queue with an emergency-first policy, "
                "custom search and sort algorithms, undo history, and CSV persistence.")
                .arg(QString::fromLatin1(utils::applicationName()),
                     QString::fromLatin1(utils::versionString()), QT_VERSION_STR));
    });
    aboutAction->setToolTip(tr("About application"));

    // Ctrl+F jumps to the Search & Sort tab.
    findAction_ = new QAction(tr("&Find"), this);
    findAction_->setShortcut(QKeySequence::Find);
    findAction_->setToolTip(tr("Search patients (Ctrl+F)"));
    addAction(findAction_);  // no menu: shortcuts-only action
    connect(findAction_, &QAction::triggered, this, [this] {
        tabs_->setCurrentIndex(4);  // Search & Sort
        showLastAction(tr("Search & Sort focused"));
    });
}

void MainWindow::buildToolBar() {
    QToolBar* bar = addToolBar(tr("Main"));
    bar->setMovable(false);
    QAction* newAction = bar->addAction(tr("New"));
    newAction->setToolTip(tr("New session"));
    connect(newAction, &QAction::triggered, this, &MainWindow::onFileNew);
    QAction* loadAction = bar->addAction(tr("Load"));
    loadAction->setToolTip(tr("Load clinic data"));
    connect(loadAction, &QAction::triggered, this, &MainWindow::onLoad);
    bar->addAction(saveAction_);
    bar->addSeparator();
    bar->addAction(undoAction_);
}

void MainWindow::buildStatusBar() {
    lastActionLabel_ = new QLabel(tr("Ready"), this);
    undoDepthLabel_ = new QLabel(tr("Undo actions: 0"), this);
    algorithmLabel_ = new QLabel(tr("Algorithm: -"), this);
    statusBar()->addWidget(lastActionLabel_, 1);
    statusBar()->addPermanentWidget(undoDepthLabel_);
    statusBar()->addPermanentWidget(algorithmLabel_);
}

void MainWindow::refreshDataViews() {
    // Single funnel: any panel's dataChanged (or an Undo, through
    // AdminPanel::undoLast -> dataChanged) re-reads the database into every
    // wired panel so no table goes stale, and the undo-depth badge travels
    // along - every recorded mutation or undo moves it.
    dashboardPanel_->refresh();
    patientPanel_->refresh();
    appointmentPanel_->refresh();
    queuePanel_->refresh();
    searchPanel_->refresh();
    adminPanel_->refresh();
    setUndoDepth(static_cast<int>(manager_->undoDepth()));
}

void MainWindow::showLastAction(const QString& action) {
    lastActionLabel_->setText(action);
}

void MainWindow::setUndoDepth(int depth) {
    undoDepthLabel_->setText(tr("Undo actions: %1").arg(depth));
    // The Edit->Undo action is enabled when the stack
    // holds at least one reversible operation.
    undoAction_->setEnabled(depth > 0);
}

void MainWindow::setAlgorithmUsed(const QString& algorithm) {
    algorithmLabel_->setText(tr("Algorithm: %1").arg(algorithm));
}

// ---- File/Edit/Admin slots ---------------------------------------------------
// The destructive session actions (New / Clear All / Load Sample) route here
// from the menus, the toolbar, the toolbar buttons and the Admin panel's
    // buttons - a single home for the confirmation dialogs and the manager
// mutations.

void MainWindow::onFileNew() {
    // File->New clears after confirmation.
    const auto answer = QMessageBox::question(
        this, tr("New session"),
        tr("Start a new session?\n\nEvery patient, appointment and waiting-queue "
           "entry will be deleted. This cannot be undone."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    manager_->clear();
    refreshDataViews();
    showLastAction(tr("New session started - all data cleared"));
}

void MainWindow::onLoadSampleData() {
    // Restores the demo dataset (replaces currently loaded records),
    // after prompting to save any unsaved changes first.
    if (dirty_ && !confirmDiscardChanges(tr("load the sample data"))) {
        showLastAction(tr("Sample data cancelled"));
        return;
    }
    const auto answer = QMessageBox::question(
        this, tr("Load sample data"),
        tr("Replace the current data with the sample clinic set?\n\nThe undo history is "
           "cleared with it."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        showLastAction(tr("Sample data cancelled"));
        return;
    }
    // Seed values are validated by the record constructors; any error
    // surfaces gracefully rather than interrupting the event loop.
    // The demo set lives in the repo's data/ directory in CSV format.
    // When absent, the same dataset is seeded in memory.
    try {
        const QDir dataDir(dataDir_);
        if (dataDir.exists(QStringLiteral("patients.csv")) &&
            dataDir.exists(QStringLiteral("appointments.csv"))) {
            loadDataDir(dataDir_, /*startup=*/false);
        } else {
            seedSampleData();
        }
        seedSampleQueue();
    } catch (const std::exception& error) {
        QMessageBox::critical(this, tr("Load sample data"),
                              tr("The sample set could not be loaded:\n%1")
                                  .arg(QString::fromUtf8(error.what())));
        return;
    }
    refreshDataViews();
    dirty_ = true;  // the queued walk-ins are session-only state
    showLastAction(tr("Sample data loaded: %1 patients, %2 appointments, %3 waiting")
                       .arg(static_cast<int>(manager_->database().patients().size()))
                       .arg(static_cast<int>(manager_->database().appointments().size()))
                       .arg(static_cast<int>(manager_->database().waitingQueue().size())));
}

// ---- sample data (in-memory fallback) ------------------
// The primary sample dataset consists of CSV files in data/, loaded through
// FileHandler. This in-memory seed provides fallback demonstration records
// when running without external data files.
void MainWindow::seedSampleData() {
    manager_->clear();

    const QString today =
        QString::fromStdString(controller::ClinicManager::currentDate());
    const auto dateWithOffset = [](int days) {
        return QDate::currentDate().addDays(days).toString(QStringLiteral("yyyy-MM-dd"));
    };

    // 15 patients - 8 General, 2 Emergency, 5 Follow-Up (spec, sample dataset).
    struct PatientSeed {
        const char* id;
        const char* name;
        int age;
        const char* contact;
        model::PatientCategory category;
        QString nextVisit;  // Follow-Up only 
    };
    const std::vector<PatientSeed> patientSeeds = {
        {"P-0001", "Adaeze Okafor",      36, "0803-210-4501", model::PatientCategory::General,   {}},
        {"P-0002", "Grace Adeyemi",      45, "0802-334-1202", model::PatientCategory::Emergency, {}},
        {"P-0003", "Adamu Bello",        41, "0805-441-7803", model::PatientCategory::FollowUp,  dateWithOffset(3)},
        {"P-0004", "Amina Yusuf",        52, "0807-552-8904", model::PatientCategory::FollowUp,  today},
        {"P-0005", "Chukwuemeka Nwosu",  60, "0812-663-9005", model::PatientCategory::General,   {}},
        {"P-0006", "Kemi Balogun",       48, "0813-774-0106", model::PatientCategory::FollowUp,  dateWithOffset(7)},
        {"P-0007", "Ibrahim Danjuma",    39, "0816-885-1207", model::PatientCategory::Emergency, {}},
        {"P-0008", "Ngozi Eze",          33, "0818-996-2308", model::PatientCategory::FollowUp,  dateWithOffset(2)},
        {"P-0009", "Emeka Obi",          57, "0809-112-3409", model::PatientCategory::General,   {}},
        {"P-0010", "Halima Sule",        29, "0701-223-4510", model::PatientCategory::General,   {}},
        {"P-0011", "Edet Okon",          50, "0703-334-5611", model::PatientCategory::FollowUp,  dateWithOffset(10)},
        {"P-0012", "Femi Adeleke",       44, "0705-445-6712", model::PatientCategory::General,   {}},
        {"P-0013", "Damilola Ajayi",     31, "0708-556-7813", model::PatientCategory::General,   {}},
        {"P-0014", "Sadiq Abubakar",     65, "0902-667-8914", model::PatientCategory::General,   {}},
        {"P-0015", "Chiamaka Okonkwo",   38, "0905-778-9015", model::PatientCategory::General,   {}},
    };
    for (const PatientSeed& seed : patientSeeds) {
        manager_->addPatientRecord(controller::ClinicManager::makePatientRecord(
            seed.id, seed.name, seed.age, seed.contact, seed.category,
            seed.nextVisit.toStdString()));
    }

    // 20 appointments - mixed statuses across past/today/future; P-0001 and
    // P-0002 carry 3 completed each (the "most attended" report seeds, ).
    const auto pending = model::AppointmentStatus::Pending;
    const auto completed = model::AppointmentStatus::Completed;
    const auto cancelled = model::AppointmentStatus::Cancelled;
    struct AppointmentSeed {
        const char* id;
        const char* patientId;
        QString date;
        const char* time;
        model::AppointmentStatus status;
        const char* notes;
    };
    const std::vector<AppointmentSeed> appointmentSeeds = {
        {"A-0001", "P-0001", dateWithOffset(-16), "09:00", completed, ""},
        {"A-0002", "P-0001", dateWithOffset(-12), "09:30", completed, "blood work review"},
        {"A-0003", "P-0001", dateWithOffset(-5),  "10:00", completed, ""},
        {"A-0004", "P-0001", today,               "09:30", pending,   "annual check-up"},
        {"A-0005", "P-0002", dateWithOffset(-9),  "08:30", completed, "triage"},
        {"A-0006", "P-0002", dateWithOffset(-4),  "08:45", completed, "suture removal"},
        {"A-0007", "P-0002", today,               "10:15", pending,   "follow-up on injury"},
        {"A-0008", "P-0003", today,               "11:00", pending,   "post-treatment review"},
        {"A-0009", "P-0003", dateWithOffset(5),   "11:00", pending,   ""},
        {"A-0010", "P-0004", today,               "14:00", pending,   "next scheduled visit"},
        {"A-0011", "P-0005", dateWithOffset(-7),  "09:00", pending,   "missed - came in late"},
        {"A-0012", "P-0005", dateWithOffset(-3),  "15:30", cancelled, "patient called off"},
        {"A-0013", "P-0006", dateWithOffset(2),   "13:00", pending,   ""},
        {"A-0014", "P-0007", dateWithOffset(-2),  "10:30", pending,   "no-show"},
        {"A-0015", "P-0007", today,               "16:00", pending,   "wound check"},
        {"A-0016", "P-0008", dateWithOffset(-1),  "11:15", cancelled, "rescheduling"},
        {"A-0017", "P-0009", dateWithOffset(1),   "09:45", pending,   ""},
        {"A-0018", "P-0010", today,               "08:00", completed, "morning consult"},
        {"A-0019", "P-0011", dateWithOffset(3),   "10:00", pending,   ""},
        {"A-0020", "P-0002", dateWithOffset(-8),  "16:45", completed, "lab results"},
    };
    for (const AppointmentSeed& seed : appointmentSeeds) {
        manager_->addAppointmentRecord(std::make_unique<model::Appointment>(
            seed.id, seed.patientId, seed.date.toStdString(), seed.time, seed.status,
            seed.notes));
    }

    // Counters restored so the next registration continues at P-0016/A-0021.
    manager_->restoreIdCounters(15, 20);

    // 5 queued patients - includes an emergency patient who arrived recently
    // and a patient waiting over 15 minutes to demonstrate queue priority
    // and wait-time alert badges.
    seedSampleQueue();

    // Warm recent patient cache for dashboard display.
    manager_->getPatient("P-0002");
    manager_->getPatient("P-0004");
    manager_->getPatient("P-0001");
}

// Five queued patients of the sample scenario, laid over the active dataset
// (waiting queue is session-only state and is not stored in persistent files).
void MainWindow::seedSampleQueue() {
    const QTime now = QTime::currentTime();
    const auto minutesAgo = [&now](int minutes) {
        return now.addSecs(-60 * minutes).toString(QStringLiteral("HH:mm"));
    };
    manager_->addToQueue("P-0001", minutesAgo(8).toStdString());
    manager_->addToQueue("P-0002", minutesAgo(2).toStdString());   // emergency, newest
    manager_->addToQueue("P-0005", minutesAgo(21).toStdString());  // over the 15-min threshold
    manager_->addToQueue("P-0008", minutesAgo(12).toStdString());
    manager_->addToQueue("P-0011", minutesAgo(4).toStdString());
}

void MainWindow::onSave() {
    try {
        // saveClinic creates dataDir_ if absent, so saving to a new folder succeeds.
        manager_->saveClinic(dataDir_.toStdString());
        dirty_ = false;
        rememberDataDir(dataDir_);  // the next startup auto-loads this directory
        showLastAction(tr("Saved %1 patient(s), %2 appointment(s) to %3")
                           .arg(static_cast<int>(manager_->database().patients().size()))
                           .arg(static_cast<int>(manager_->database().appointments().size()))
                           .arg(QDir::toNativeSeparators(dataDir_)));
    } catch (const utils::FileError& error) {
        QMessageBox::warning(this, tr("Cannot save clinic data"),
                             QString::fromUtf8(error.what()));
    }
}

void MainWindow::onSaveAs() {
    // saveClinic writes a data directory containing patients.csv, appointments.csv,
    // and counters.csv, so Save As chooses a folder.
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Save clinic data to folder"), dataDir_);
    if (dir.isEmpty()) {
        showLastAction(tr("Save As cancelled"));
        return;
    }
    dataDir_ = dir;
    onSave();
}

void MainWindow::onLoad() {
    if (dirty_ && !confirmDiscardChanges(tr("load another data directory"))) {
        showLastAction(tr("Load cancelled"));
        return;
    }
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Load clinic data from folder"), dataDir_);
    if (dir.isEmpty()) {
        showLastAction(tr("Load cancelled"));
        return;
    }
    loadDataDir(dir, /*startup=*/false);
}

void MainWindow::loadDataDir(const QString& dataDir, bool startup) {
    try {
        const controller::ClinicManager::LoadSummary summary =
            manager_->loadClinic(dataDir.toStdString());
        dataDir_ = dataDir;
        dirty_ = false;  // the loaded state IS the saved state
        rememberDataDir(dataDir_);  // the next startup auto-loads this directory
        refreshDataViews();

        const QString loaded =
            tr("Loaded %1 patient(s), %2 appointment(s) from %3")
                .arg(summary.patientsLoaded)
                .arg(summary.appointmentsLoaded)
                .arg(QDir::toNativeSeparators(dataDir));
        showLastAction(loaded);
        if (!summary.issues.empty()) {
            // Every skipped row is reported with row numbers and details.
            QStringList lines;
            lines.reserve(static_cast<qsizetype>(summary.issues.size()));
            for (const std::string& issue : summary.issues)
                lines << QString::fromStdString(issue);
            QMessageBox::warning(
                this, tr("Load finished with skipped rows"),
                tr("%1\n\nSkipped rows and notes:\n%2")
                    .arg(loaded, lines.join(QLatin1Char('\n'))));
        }
    } catch (const utils::FileError& error) {
        if (startup) {
            // Missing or unreadable data directory at startup starts empty cleanly.
            showLastAction(
                tr("No previous data found - starting empty (%1)")
                    .arg(QString::fromUtf8(error.what())));
            return;
        }
        QMessageBox::warning(this, tr("Cannot load clinic data"),
                             QString::fromUtf8(error.what()));
    }
}

bool MainWindow::confirmDiscardChanges(const QString& action) {
    const auto answer = QMessageBox::question(
        this, tr("Unsaved changes"),
        tr("You have unsaved changes. Discard them and %1 anyway?").arg(action),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer == QMessageBox::Yes;
}

void MainWindow::onQuit() {
    // quit-guard: offer to save when the session differs from disk.
    if (dirty_) {
        const auto answer = QMessageBox::question(
            this, tr("Unsaved changes"),
            tr("Save changes to %1 before quitting?")
                .arg(QDir::toNativeSeparators(dataDir_)),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);
        if (answer == QMessageBox::Cancel) return;
        if (answer == QMessageBox::Save) {
            onSave();
            if (dirty_) return;  // the save failed (dialog shown) - stay open
        }
    }
    close();
}

void MainWindow::onClearAllData() {
    const auto answer = QMessageBox::question(
        this, tr("Clear all data"),
        tr("Delete every patient, appointment and waiting-queue entry?\n\n"
           "This cannot be undone."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        showLastAction(tr("Clear cancelled"));
        return;
    }
    // clear empties patients, appointments, queue and cache AND resets both
    // ID counters and wipes the undo history (not itself undoable).
    manager_->clear();
    refreshDataViews();
    dirty_ = true;  // the files on disk still hold the pre-clear data
    showLastAction(tr("All clinic data cleared"));
}

}  // namespace scapms::gui
