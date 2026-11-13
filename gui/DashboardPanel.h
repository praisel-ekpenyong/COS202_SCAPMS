#pragma once

#include <QWidget>

#include "controller/ClinicManager.h"

class QLabel;
class QListWidget;
class QTableWidget;
class QTimer;

namespace scapms::gui {

// Dashboard tab showing clinic statistics, appointments, and reminders.
class DashboardPanel : public QWidget {
    Q_OBJECT

public:
    explicit DashboardPanel(controller::ClinicManager* manager, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
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

    unsigned lastRevision_ = 0;  // last data revision counter
    int lastOverdueCount_ = -1;  // tracks overdue count for status bar
};

}  // namespace scapms::gui
