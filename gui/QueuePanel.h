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

// Waiting queue tab: displays live waiting queue ordered by priority
// (Emergency first, then arrival time). Includes wait-time alerts.
class QueuePanel : public QWidget {
    Q_OBJECT

public:
    // Alert threshold in minutes for long wait times.
    static constexpr int kWaitingAlertMinutes = 15;

    explicit QueuePanel(controller::ClinicManager* manager, QWidget* parent = nullptr);

public slots:
    void refresh();

signals:
    void statusMessage(const QString& message);
    void dataChanged();

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void onAddWalkIn();
    void onCallNext();
    void onAlertTick();

private:
    struct QueueEntry {
        QString id;
        QString name;
        QString category;
        QString arrival;
        int priority = 0;
        bool emergency = false;
    };

    // Orders queue entries by priority (highest first), then arrival time
    static std::vector<QueueEntry> servingOrder(const model::ClinicDatabase& database);

    void buildUi();
    void rebuildTable();
    void paintRows(const std::vector<QueueEntry>& entries);
    void refreshWalkInChoices();
    void updateSummaryLabel(const std::vector<QueueEntry>& entries);
    int waitingMinutes(const QString& arrivalTime) const;

    controller::ClinicManager* manager_;
    QTableWidget* queueTable_ = nullptr;
    QComboBox* patientCombo_ = nullptr;
    QLineEdit* arrivalEdit_ = nullptr;
    QLabel* summaryLabel_ = nullptr;
    QTimer* alertTimer_ = nullptr;
    unsigned lastRevision_ = 0;
    QSet<QString> alertedIds_;
};

}  // namespace scapms::gui
