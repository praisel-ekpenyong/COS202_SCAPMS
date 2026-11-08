#include "gui/QueuePanel.h"

#include <QBrush>
#include <QComboBox>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

#include "gui/Theme.h"
#include "model/PatientCategory.h"
#include "model/PatientRecord.h"
#include "utils/Exceptions.h"

namespace scapms::gui {

QueuePanel::QueuePanel(controller::ClinicManager* manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    buildUi();

    // The alert timer: a QTimer ticks every 30 s off the event loop,
// whether or not this tab is visible - nobody has to click anything for
    // the >15-min waiting alert to fire.
    alertTimer_ = new QTimer(this);
    alertTimer_->setInterval(30 * 1000);
    connect(alertTimer_, &QTimer::timeout, this, &QueuePanel::onAlertTick);
    alertTimer_->start();

    refresh();
}

void QueuePanel::buildUi() {
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel(tr("Waiting Queue"), this);
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));
    layout->addWidget(title);

    summaryLabel_ = new QLabel(this);
    layout->addWidget(summaryLabel_);

    queueTable_ = new QTableWidget(this);
    queueTable_->setColumnCount(5);
    queueTable_->setHorizontalHeaderLabels({tr("ID"), tr("Name"), tr("Category"),
                                            tr("Arrival"), tr("Waiting (min)")});
    queueTable_->verticalHeader()->setVisible(false);
    queueTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    queueTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    queueTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    QHeaderView* header = queueTable_->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::Stretch);  // names take the slack
    header->setStretchLastSection(true);
    layout->addWidget(queueTable_, 1);

    // Walk-in group: a registered patient joins the queue without an
    // appointment for today.
    auto* walkInBox = new QGroupBox(tr("Add walk-in"), this);
    auto* walkInRow = new QHBoxLayout(walkInBox);
    auto* patientLabel = new QLabel(tr("Patient:"), walkInBox);
    patientCombo_ = new QComboBox(walkInBox);
    auto* arrivalLabel = new QLabel(tr("Arrival:"), walkInBox);
    arrivalEdit_ = new QLineEdit(walkInBox);
    arrivalEdit_->setPlaceholderText(tr("HH:MM (leave empty for current time)"));
    auto* addButton = new QPushButton(tr("Add to queue"), walkInBox);
    addButton->setToolTip(tr("Add patient to waiting queue"));
    connect(addButton, &QPushButton::clicked, this, &QueuePanel::onAddWalkIn);
    walkInRow->addWidget(patientLabel);
    walkInRow->addWidget(patientCombo_, 1);
    walkInRow->addWidget(arrivalLabel);
    walkInRow->addWidget(arrivalEdit_);
    walkInRow->addWidget(addButton);
    layout->addWidget(walkInBox);

    auto* actionRow = new QHBoxLayout();
    auto* callNextButton = new QPushButton(tr("Call next patient"), this);
    callNextButton->setToolTip(tr("Call the next patient in queue"));
    callNextButton->setStyleSheet(QStringLiteral("font-weight: bold;"));
    connect(callNextButton, &QPushButton::clicked, this, &QueuePanel::onCallNext);
    actionRow->addWidget(callNextButton);
    actionRow->addStretch(1);
    layout->addLayout(actionRow);
}

void QueuePanel::refresh() {
    rebuildTable();        // rows + tint pass + summary
    refreshWalkInChoices();
    lastRevision_ = manager_->revision();
}

std::vector<QueuePanel::QueueEntry> QueuePanel::servingOrder(
    const model::ClinicDatabase& database) {
    std::vector<QueueEntry> entries;
    std::queue<std::string> ids = database.waitingQueue();  // copy: the view is const
    while (!ids.empty()) {
        const model::PatientRecord* patient = database.findPatient(ids.front());
        if (patient != nullptr) {
            entries.push_back({QString::fromStdString(patient->id()),
                               QString::fromStdString(patient->name()),
                               QString::fromStdString(model::categoryName(patient->category())),
                               QString::fromStdString(database.queueArrivalTime(patient->id())),
                               patient->priority(),
                               patient->category() == model::PatientCategory::Emergency});
        }
        ids.pop();
    }

    // Insertion sort by (priority desc, arrival asc) for queue display.
    // PatientSorter keys do not include queue priority.
    for (std::size_t i = 1; i < entries.size(); ++i) {
        QueueEntry key = entries[i];
        std::size_t j = i;
        while (j > 0 && (entries[j - 1].priority < key.priority ||
                         (entries[j - 1].priority == key.priority &&
                          entries[j - 1].arrival > key.arrival))) {
            entries[j] = entries[j - 1];
            --j;
        }
        entries[j] = key;
    }
    return entries;
}

void QueuePanel::rebuildTable() {
    const std::vector<QueueEntry> entries = servingOrder(manager_->database());
    queueTable_->setRowCount(static_cast<int>(entries.size()));
    for (int row = 0; row < static_cast<int>(entries.size()); ++row) {
        const QueueEntry& entry = entries[static_cast<std::size_t>(row)];
        // category colouring goes on the Category cell; row tints (alert
        // amber / emergency wash) are applied by paintRows.
        auto* categoryItem = new QTableWidgetItem(entry.category);
        categoryItem->setForeground(theme::categoryColor(entry.category));
        queueTable_->setItem(row, 0, new QTableWidgetItem(entry.id));
        queueTable_->setItem(row, 1, new QTableWidgetItem(entry.name));
        queueTable_->setItem(row, 2, categoryItem);
        queueTable_->setItem(row, 3, new QTableWidgetItem(entry.arrival));
        queueTable_->setItem(row, 4,
                             new QTableWidgetItem(QString::number(waitingMinutes(entry.arrival))));
    }
    paintRows(entries);
}

void QueuePanel::paintRows(const std::vector<QueueEntry>& entries) {
    QStringList newlyAlerted;
    QSet<QString> stillAlerted;
    for (int row = 0; row < static_cast<int>(entries.size()); ++row) {
        const QueueEntry& entry = entries[static_cast<std::size_t>(row)];
        const int minutes = waitingMinutes(entry.arrival);
        const bool alerting = minutes >= kWaitingAlertMinutes;

        auto* waitingItem = queueTable_->item(row, 4);
        waitingItem->setText(QString::number(minutes));
        QFont waitingFont = waitingItem->font();
        waitingFont.setBold(alerting);
        waitingItem->setFont(waitingFont);

        // an alerting row tints amber above everything; otherwise an
        // Emergency row keeps a light red wash so it reads as "jumps the
        // queue" even before the category column is read.
        QColor tint;
        if (alerting) {
            tint = theme::kStatusPending;
            tint.setAlpha(70);
        } else if (entry.emergency) {
            tint = theme::kCategoryEmergency;
            tint.setAlpha(40);
        }
        for (int column = 0; column < queueTable_->columnCount(); ++column) {
            queueTable_->item(row, column)->setBackground(tint.isValid() ? QBrush(tint) : QBrush());
        }

        if (alerting) {
            stillAlerted.insert(entry.id);
            if (!alertedIds_.contains(entry.id)) {
                newlyAlerted << tr("%1 %2 - waiting %3 min")
                                    .arg(entry.id, entry.name)
                                    .arg(minutes);
            }
        }
    }
    alertedIds_ = stillAlerted;

    // alert: a status-bar message. It fires once per patient per waiting
    // spell (the amber tint persists; no 30-second repeats), so the message
    // stays meaningful instead of drowning the feed.
    if (!newlyAlerted.isEmpty()) {
        emit statusMessage(tr("Waiting-time alert (over %1 min): %2")
                               .arg(kWaitingAlertMinutes)
                               .arg(newlyAlerted.join(QStringLiteral("; "))));
    }
    updateSummaryLabel(entries);
}

void QueuePanel::refreshWalkInChoices() {
    const QString previous = patientCombo_->currentData().toString();
    patientCombo_->clear();
    int reopen = -1;
    for (const auto& record : manager_->database().patients()) {
        const QString id = QString::fromStdString(record->id());
        if (manager_->database().isQueued(record->id())) {
            continue;  // already waiting - not a walk-in candidate
        }
        patientCombo_->addItem(QStringLiteral("%1 - %2 (%3)")
                                   .arg(id, QString::fromStdString(record->name()),
                                        QString::fromStdString(model::categoryName(record->category()))),
                               id);
        if (id == previous) {
            reopen = patientCombo_->count() - 1;
        }
    }
    if (patientCombo_->count() == 0) {
        patientCombo_->addItem(tr("- No patients available to queue -"));
        patientCombo_->setEnabled(false);
    } else {
        patientCombo_->setEnabled(true);
        patientCombo_->setCurrentIndex(reopen >= 0 ? reopen : 0);
    }
}

void QueuePanel::updateSummaryLabel(const std::vector<QueueEntry>& entries) {
    if (entries.empty()) {
        summaryLabel_->setText(tr("The waiting queue is empty."));
        return;
    }
    int alerting = 0;
    for (const QueueEntry& entry : entries) {
        if (waitingMinutes(entry.arrival) >= kWaitingAlertMinutes) {
            ++alerting;
        }
    }
    QString text = tr("%n patient(s) waiting - next: %1 %2 (%3)", "",
                      static_cast<int>(entries.size()))
                       .arg(entries.front().id, entries.front().name, entries.front().category);
    if (alerting > 0) {
        text += tr(", %n waiting over %1 min", "", alerting).arg(kWaitingAlertMinutes);
    }
    summaryLabel_->setText(text);
}

void QueuePanel::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    refresh();  // fresh data whenever the tab becomes visible
}

void QueuePanel::onAddWalkIn() {
    const QString id = patientCombo_->currentData().toString();
    if (id.isEmpty()) {
        QMessageBox::information(this, tr("Add walk-in"),
                                 tr("There is no registered patient to add to the queue."));
        return;
    }
    const QString arrival = arrivalEdit_->text().trimmed();
    try {
        manager_->addToQueue(id.toStdString(), arrival.toStdString());
    } catch (const utils::ValidationError& error) {  // bad HH:MM or already queued
        QMessageBox::warning(this, tr("Add walk-in"),
                             tr("The patient could not be queued:\n%1")
                                 .arg(QString::fromUtf8(error.what())));
        return;
    }
    const QString arrivalUsed = QString::fromStdString(
        manager_->database().queueArrivalTime(id.toStdString()));
    const model::PatientRecord* patient = manager_->database().findPatient(id.toStdString());
    emit statusMessage(tr("Walk-in added: %1 (%2), arrival %3")
                           .arg(id, patient != nullptr ? QString::fromStdString(patient->name())
                                                       : QString())
                           .arg(arrivalUsed));
    emit dataChanged();  // MainWindow refreshes every panel
}

void QueuePanel::onCallNext() {
    const std::vector<QueueEntry> entries = servingOrder(manager_->database());
    if (entries.empty()) {
        emit statusMessage(tr("Call next: nobody is waiting"));
        return;
    }
    const QueueEntry& next = entries.front();
    const auto answer = QMessageBox::question(
        this, tr("Call next patient"),
        tr("Serve %1 (%2, %3) now?\n\nIf they have an appointment pending for today, it is "
           "marked completed. Serving is a clinical event and cannot be undone.")
            .arg(next.id, next.name, next.category),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    model::PatientRecord* served = manager_->callNext();
    if (served == nullptr) {  // queue emptied between the dialog and the call
        emit statusMessage(tr("Call next: nobody is waiting"));
        return;
    }
    emit statusMessage(tr("Called %1 (%2) - consultation started")
                           .arg(QString::fromStdString(served->id()),
                                QString::fromStdString(served->name())));
    emit dataChanged();
}

void QueuePanel::onAlertTick() {
    // The 30 s tick always re-checks waiting times - minutes grow even
    // when no record changed. A revision change additionally means the rows
    // themselves are stale (a delete/undo elsewhere), so rebuild fully.
    if (manager_->revision() != lastRevision_) {
        refresh();
        return;
    }
    paintRows(servingOrder(manager_->database()));
}

int QueuePanel::waitingMinutes(const QString& arrivalTime) const {
    const QTime arrival = QTime::fromString(arrivalTime, QStringLiteral("HH:mm"));
    if (!arrival.isValid()) {
        return 0;  // a blank or malformed arrival simply never alerts
    }
    const QTime now = QTime::currentTime();
    const int minutes = now.hour() * 60 + now.minute() - (arrival.hour() * 60 + arrival.minute());
    return minutes < 0 ? 0 : minutes;
}

}  // namespace scapms::gui
