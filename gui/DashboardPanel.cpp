#include "gui/DashboardPanel.h"

#include <QFont>
#include <QFrame>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "gui/Theme.h"
#include "model/Appointment.h"
#include "model/AppointmentStatus.h"
#include "model/PatientCategory.h"
#include "model/PatientRecord.h"

namespace scapms::gui {

DashboardPanel::DashboardPanel(controller::ClinicManager* manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    buildUi();

    // The refresh timer: reminders must move with the wall clock (a pending
    // appointment becomes overdue at midnight without any record changing),
    // so the tick runs unconditionally; a revision change escalates the tick
    // to a full rebuild (see onClockTick).
    refreshTimer_ = new QTimer(this);
    refreshTimer_->setInterval(30 * 1000);
    connect(refreshTimer_, &QTimer::timeout, this, &DashboardPanel::onClockTick);
    refreshTimer_->start();

    refresh();
}

void DashboardPanel::buildUi() {
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel(tr("Clinic Dashboard"), this);
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));
    layout->addWidget(title);

    // --- stat cards ---------------------------------------------------
    // Chrome colours (#FFFFFF surface, #D3D8DE border) mirror the theme
    // stylesheet; the value/caption colours come from theme:: constants.
    auto* summaryBox = new QGroupBox(tr("Clinic Summary"), this);
    auto* summaryLayout = new QGridLayout(summaryBox);

    auto* lblTotal = new QLabel(tr("Total Registered Patients:"), summaryBox);
    totalPatientsValue_ = new QLabel(QStringLiteral("0"), summaryBox);
    totalPatientsValue_->setStyleSheet(QStringLiteral("font-weight: bold;"));

    auto* lblQueue = new QLabel(tr("In Waiting Queue:"), summaryBox);
    queuedValue_ = new QLabel(QStringLiteral("0"), summaryBox);
    queuedValue_->setStyleSheet(QStringLiteral("font-weight: bold;"));

    auto* lblToday = new QLabel(tr("Today's Appointments:"), summaryBox);
    todaysValue_ = new QLabel(QStringLiteral("0"), summaryBox);
    todaysValue_->setStyleSheet(QStringLiteral("font-weight: bold;"));

    auto* lblPending = new QLabel(tr("Pending Appointments:"), summaryBox);
    pendingValue_ = new QLabel(QStringLiteral("0"), summaryBox);
    pendingValue_->setStyleSheet(QStringLiteral("font-weight: bold;"));

    summaryLayout->addWidget(lblTotal, 0, 0);
    summaryLayout->addWidget(totalPatientsValue_, 0, 1);
    summaryLayout->addWidget(lblToday, 0, 2);
    summaryLayout->addWidget(todaysValue_, 0, 3);

    summaryLayout->addWidget(lblQueue, 1, 0);
    summaryLayout->addWidget(queuedValue_, 1, 1);
    summaryLayout->addWidget(lblPending, 1, 2);
    summaryLayout->addWidget(pendingValue_, 1, 3);

    layout->addWidget(summaryBox);

    // --- reminders (today's / overdue flagged) --------------------------
    remindersLabel_ = new QLabel(this);
    remindersLabel_->setWordWrap(true);
    layout->addWidget(remindersLabel_);

    // --- today's appointments ------------------------------------------------
    auto* todayBox = new QGroupBox(tr("Today's appointments"), this);
    auto* todayLayout = new QVBoxLayout(todayBox);
    statusCountsLabel_ = new QLabel(todayBox);
    todayLayout->addWidget(statusCountsLabel_);
    todayTable_ = new QTableWidget(todayBox);
    todayTable_->setColumnCount(4);
    todayTable_->setHorizontalHeaderLabels({tr("Time"), tr("ID"), tr("Patient"), tr("Status")});
    todayTable_->verticalHeader()->setVisible(false);
    todayTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    todayTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    todayTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    QHeaderView* todayHeader = todayTable_->horizontalHeader();
    todayHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
    todayHeader->setSectionResizeMode(2, QHeaderView::Stretch);
    todayTable_->setMaximumHeight(180);
    todayLayout->addWidget(todayTable_);
    layout->addWidget(todayBox);

    // --- frequently-accessed cache (recent patients) -------------------------
    auto* cacheBox = new QGroupBox(tr("Frequently accessed - last 10 records"), this);
    auto* cacheLayout = new QVBoxLayout(cacheBox);
    cacheList_ = new QListWidget(cacheBox);
    cacheList_->setMaximumHeight(160);
    cacheLayout->addWidget(cacheList_);
    layout->addWidget(cacheBox, 1);

    layout->addStretch(1);
}

void DashboardPanel::refresh() {
    rebuildCards();
    rebuildTodayTable();
    rebuildCacheList();
    updateReminders();
    lastRevision_ = manager_->revision();
}

void DashboardPanel::rebuildCards() {
    const auto& appointments = manager_->database().appointments();
    const std::string today = controller::ClinicManager::currentDate();
    int todays = 0;
    int pending = 0;
    for (const auto& appointment : appointments) {
        if (appointment->date() == today) {
            ++todays;
        }
        if (appointment->status() == model::AppointmentStatus::Pending) {
            ++pending;
        }
    }
    totalPatientsValue_->setText(QString::number(manager_->database().patients().size()));
    queuedValue_->setText(QString::number(manager_->database().waitingQueue().size()));
    todaysValue_->setText(QString::number(todays));
    pendingValue_->setText(QString::number(pending));
}

void DashboardPanel::rebuildTodayTable() {
    const std::string today = controller::ClinicManager::currentDate();
    std::vector<model::Appointment*> todays;
    for (const auto& appointment : manager_->database().appointments()) {
        if (appointment->date() == today) {
            todays.push_back(appointment.get());
        }
    }

    // Insertion sort by (time, id) for today's appointment list.
    for (std::size_t i = 1; i < todays.size(); ++i) {
        model::Appointment* key = todays[i];
        std::size_t j = i;
        while (j > 0 && (todays[j - 1]->time() > key->time() ||
                         (todays[j - 1]->time() == key->time() &&
                          todays[j - 1]->id() > key->id()))) {
            todays[j] = todays[j - 1];
            --j;
        }
        todays[j] = key;
    }

    int pending = 0;
    int completed = 0;
    int cancelled = 0;
    todayTable_->setRowCount(static_cast<int>(todays.size()));
    for (int row = 0; row < static_cast<int>(todays.size()); ++row) {
        const model::Appointment* appointment = todays[static_cast<std::size_t>(row)];
        const QString status =
            QString::fromStdString(model::appointmentStatusName(appointment->status()));
        switch (appointment->status()) {
            case model::AppointmentStatus::Pending: ++pending; break;
            case model::AppointmentStatus::Completed: ++completed; break;
            case model::AppointmentStatus::Cancelled: ++cancelled; break;
        }
        const model::PatientRecord* patient =
            manager_->database().findPatient(appointment->patientId());
        // status colouring on the Status cell.
        auto* statusItem = new QTableWidgetItem(status);
        statusItem->setForeground(theme::statusColor(status));
        todayTable_->setItem(row, 0, new QTableWidgetItem(
                                          QString::fromStdString(appointment->time())));
        todayTable_->setItem(row, 1, new QTableWidgetItem(
                                          QString::fromStdString(appointment->id())));
        todayTable_->setItem(row, 2, new QTableWidgetItem(
                                          patient != nullptr
                                              ? QString::fromStdString(patient->name())
                                              : tr("(unknown patient)")));
        todayTable_->setItem(row, 3, statusItem);
    }
    statusCountsLabel_->setText(
        tr("%n appointment(s) today - %1 pending, %2 completed, %3 cancelled", "",
           static_cast<int>(todays.size()))
            .arg(pending)
            .arg(completed)
            .arg(cancelled));
}

void DashboardPanel::rebuildCacheList() {
    cacheList_->clear();
    const std::size_t count = manager_->database().recentCount();
    if (count == 0) {
        auto* empty = new QListWidgetItem(tr("No record accesses yet"));
        empty->setFlags(Qt::NoItemFlags);
        cacheList_->addItem(empty);
        return;
    }
    for (std::size_t i = 0; i < count; ++i) {
        const model::PatientRecord* record = manager_->database().recentAt(i);
        if (record == nullptr) {
            continue;  // defensive: never expected (forgetRecent keeps it compact)
        }
        const QString category =
            QString::fromStdString(model::categoryName(record->category()));
        auto* entry = new QListWidgetItem(QStringLiteral("%1.  %2 - %3, %4")
                                              .arg(i + 1)
                                              .arg(QString::fromStdString(record->id()),
                                                   QString::fromStdString(record->name()),
                                                   category));
        entry->setForeground(theme::categoryColor(category));  // category colouring
        cacheList_->addItem(entry);
    }
}

void DashboardPanel::updateReminders() {
    const std::string today = controller::ClinicManager::currentDate();
    QStringList dueToday;
    QStringList overdue;
    for (const auto& appointment : manager_->database().appointments()) {
        if (appointment->status() != model::AppointmentStatus::Pending) {
            continue;
        }
        const QString id = QString::fromStdString(appointment->id());
        const QString patientId = QString::fromStdString(appointment->patientId());
        if (appointment->date() == today) {
            dueToday << tr("%1 at %2 (%3)")
                            .arg(id, QString::fromStdString(appointment->time()), patientId);
        } else if (appointment->isOverdue(today)) {
            overdue << tr("%1 for %2, due %3")
                           .arg(id, patientId, QString::fromStdString(appointment->date()));
        }
    }

    // Keep the label bounded no matter how the dataset grows.
    constexpr int kMaxListed = 4;
    auto shorten = [](QStringList& items) {
        if (items.size() > kMaxListed) {
            items.resize(kMaxListed);
            items << QStringLiteral("...");
        }
    };
    shorten(dueToday);
    shorten(overdue);

    QString text;
    if (dueToday.isEmpty() && overdue.isEmpty()) {
        text = tr("Reminders - nothing pending for today; no overdue appointments.");
    } else {
        text = tr("Reminders - %n pending for today: %1", "", dueToday.size())
                   .arg(dueToday.isEmpty() ? QStringLiteral("-") : dueToday.join(QStringLiteral("; ")));
        if (!overdue.isEmpty()) {
            text += tr("\nOverdue: %n pending appointment(s) from earlier days: %1", "",
                       overdue.size())
                        .arg(overdue.join(QStringLiteral("; ")));
        }
    }
    remindersLabel_->setText(text);
    remindersLabel_->setStyleSheet(
        overdue.isEmpty()
            ? QString()
            : QStringLiteral("color: %1; font-weight: bold;")
                  .arg(theme::kCategoryEmergency.name()));

    // Status-bar feed only when the overdue picture changes - periodic
    // rebuilds must not overwrite the "last action" display.
    if (lastOverdueCount_ >= 0 && overdue.size() != lastOverdueCount_) {
        emit statusMessage(overdue.isEmpty()
                               ? tr("Reminder cleared: no overdue appointments")
                               : tr("Reminder: %n overdue appointment(s)", "",
                                    static_cast<int>(overdue.size())));
    }
    lastOverdueCount_ = static_cast<int>(overdue.size());
}

void DashboardPanel::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    refresh();  // fresh counts whenever the tab becomes visible
}

void DashboardPanel::onClockTick() {
    if (manager_->revision() != lastRevision_) {
        refresh();  // record data moved under us (another tab's panel)
        return;
    }
    updateReminders();  // wall-clock drift only: today/overdue lines
}

}  // namespace scapms::gui
