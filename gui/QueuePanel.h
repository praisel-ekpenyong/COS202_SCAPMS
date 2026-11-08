#pragma once

#include <QWidget>

#include "controller/ClinicManager.h"

#include <QSet>
#include <vector>

class QComboBox;
class QLabel;
class QLineEdit;
class QTableWidget;
class QTimer;

namespace scapms::gui {

// Waiting queue tab: the live view of who is waiting and who gets
// served next. The table shows the serving order - Emergency
// patients first, FIFO among equals - with category colouring, arrival time
// and live waiting minutes; "Call next" serves through
// ClinicManager::callNext behind a confirmation dialog (serving is a
// clinical event and never undoable). The walk-in group adds registered
// patients to the queue without requiring an appointment.
//
// Alert timer: a QTimer ticks every 30 s, recomputes waiting minutes,
// repaints the row highlights and, when someone has waited more than
// kWaitingAlertMinutes, raises the status-bar alert.
class QueuePanel : public QWidget {
    Q_OBJECT

public:
    // waiting-time alert threshold: a patient waiting at least this many
    // minutes gets an amber row tint and a status-bar alert.
    static constexpr int kWaitingAlertMinutes = 15;

    explicit QueuePanel(controller::ClinicManager* manager, QWidget* parent = nullptr);

public slots:
    // Rebuilds the table from the manager's read-only view and re-checks the
    // waiting-time alert. Runs on the 30 s tick, on tab show, and after any
    // queue or appointment mutation reported by MainWindow.
    void refresh();

signals:
    // Status-bar feed (MainWindow::showLastAction); used both for actions and
    // for the waiting-time alert.
    void statusMessage(const QString& message);
    // Emitted after addToQueue/callNext change queue or appointment state.
    void dataChanged();

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void onAddWalkIn();
    void onCallNext();
    void onAlertTick();

private:
    // One waiting patient as shown in the table. Rows are rebuilt from these
    // so the periodic repaint never needs widget-state bookkeeping.
    struct QueueEntry {
        QString id;
        QString name;
        QString category;  // "General" / "Emergency" / "Follow-Up"
        QString arrival;   // "HH:MM"
        int priority = 0;  // PatientRecord::priority - Emergency is highest
        bool emergency = false;
    };

    // The serving order the table displays: highest priority first,
    // FIFO among equals (earlier "HH:MM" first). The stored queue is plain
    // FIFO; this ordering is computed for display and matches
    // ClinicManager::callNext's selection.
    static std::vector<QueueEntry> servingOrder(const model::ClinicDatabase& database);

    void buildUi();
    void rebuildTable();
    // Repaints waiting minutes + row tints and raises the alert for
    // newly-over-threshold patients; also refreshes the summary line.
    void paintRows(const std::vector<QueueEntry>& entries);
    void refreshWalkInChoices();
    void updateSummaryLabel(const std::vector<QueueEntry>& entries);
    // Minutes since an "HH:MM" arrival this morning, clamped at 0 (a queue
    // session crossing midnight is not modelled - arrivals carry no date).
    int waitingMinutes(const QString& arrivalTime) const;

    controller::ClinicManager* manager_;
    QTableWidget* queueTable_ = nullptr;
    QComboBox* patientCombo_ = nullptr;
    QLineEdit* arrivalEdit_ = nullptr;
    QLabel* summaryLabel_ = nullptr;
    QTimer* alertTimer_ = nullptr;
    // revision at the last full rebuild - the 30 s tick rebuilds when it
    // moved (a delete/undo elsewhere changed the rows) and only repaints
    // otherwise, since queue state itself does not bump the revision.
    unsigned lastRevision_ = 0;
    // Patients already announced as over the threshold: the alert message
    // fires once per patient per waiting spell, the amber tint persists
    // while they keep waiting.
    QSet<QString> alertedIds_;
};

}  // namespace scapms::gui
