#pragma once

#include <QWidget>

#include "controller/ClinicManager.h"

class QLabel;
class QListWidget;
class QTableWidget;
class QTimer;

namespace scapms::gui {

// Clinic-at-a-glance view: stat cards (total patients, in queue,
// today's appointments, pending), today's appointment table colored by
// status, the frequently-accessed cache displayed as the last 10
// records, and reminders for today's and overdue appointments.
//
// Refresh triggers: after any action (MainWindow refreshDataViews wiring), on
// tab show, and on a 30 s timer. The timer rebuilds everything when
// manager revision changes and otherwise refreshes only the reminders -
// overdue flags depend on the calendar date, which rolls over without any
// record mutation.
class DashboardPanel : public QWidget {
    Q_OBJECT

public:
    explicit DashboardPanel(controller::ClinicManager* manager, QWidget* parent = nullptr);

public slots:
    // Re-reads every count, table and list from the manager's read-only view.
    void refresh();

signals:
    // Status-bar feed (MainWindow::showLastAction) - used sparingly, only
    // when the overdue-reminder count changes, never for periodic rebuilds.
    void statusMessage(const QString& message);

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void onClockTick();

private:
    void buildUi();
    void rebuildCards();
    void rebuildTodayTable();
    void rebuildCacheList();
    void updateReminders();

    controller::ClinicManager* manager_;

    // Stat cards (daily summary at a glance).
    QLabel* totalPatientsValue_ = nullptr;
    QLabel* queuedValue_ = nullptr;
    QLabel* todaysValue_ = nullptr;
    QLabel* pendingValue_ = nullptr;

    QLabel* statusCountsLabel_ = nullptr;
    QTableWidget* todayTable_ = nullptr;
    QListWidget* cacheList_ = nullptr;
    QLabel* remindersLabel_ = nullptr;
    QTimer* refreshTimer_ = nullptr;

    unsigned lastRevision_ = 0;  // revision at the last full rebuild
    int lastOverdueCount_ = -1;  // reminder-change detection for the status bar
};

}  // namespace scapms::gui
