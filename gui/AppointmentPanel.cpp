#include "gui/AppointmentPanel.h"

#include <QBrush>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "controller/ClinicManager.h"
#include "controller/SortAlgorithms.h"
#include "gui/Theme.h"
#include "model/Appointment.h"
#include "model/AppointmentStatus.h"
#include "model/ClinicDatabase.h"
#include "model/PatientRecord.h"
#include "utils/Exceptions.h"

namespace scapms::gui {

namespace {

QTableWidgetItem* readOnlyItem(const QString& text) {
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

// Dialog for scheduling an appointment
class ScheduleDialog : public QDialog {
public:
    ScheduleDialog(const model::ClinicDatabase& database, QWidget* parent)
        : QDialog(parent) {
        setWindowTitle(tr("Schedule appointment"));
        auto* form = new QFormLayout(this);

        patientBox_ = new QComboBox(this);
        for (const auto& record : database.patients()) {
            const model::PatientRecord& patient = *record;
            patientBox_->addItem(
                QStringLiteral("%1 - %2 (%3)")
                    .arg(QString::fromStdString(patient.id()),
                         QString::fromStdString(patient.name()),
                         QString::fromStdString(model::categoryName(patient.category()))),
                QString::fromStdString(patient.id()));
        }
        form->addRow(tr("Patient:"), patientBox_);

        dateEdit_ = new QDateEdit(QDate::currentDate(), this);
        dateEdit_->setCalendarPopup(true);
        dateEdit_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
        form->addRow(tr("Date:"), dateEdit_);

        timeEdit_ = new QTimeEdit(QTime(9, 0), this);
        timeEdit_->setDisplayFormat(QStringLiteral("HH:mm"));
        form->addRow(tr("Time:"), timeEdit_);

        notesEdit_ = new QLineEdit(this);
        form->addRow(tr("Notes:"), notesEdit_);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                 QDialogButtonBox::Cancel,
                                             this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        form->addRow(buttons);
    }

    std::string patientId() const { return patientBox_->currentData().toString().toStdString(); }
    std::string date() const { return dateEdit_->date().toString(QStringLiteral("yyyy-MM-dd")).toStdString(); }
    std::string time() const { return timeEdit_->time().toString(QStringLiteral("HH:mm")).toStdString(); }
    std::string notes() const { return notesEdit_->text().trimmed().toStdString(); }

private:
    QComboBox* patientBox_ = nullptr;
    QDateEdit* dateEdit_ = nullptr;
    QTimeEdit* timeEdit_ = nullptr;
    QLineEdit* notesEdit_ = nullptr;
};

}  // namespace

AppointmentPanel::AppointmentPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    buildUi();
    refresh();
}

void AppointmentPanel::buildUi() {
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel(tr("Appointments"), this);
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));
    layout->addWidget(title);

    // Buttons and filters
    auto* controls = new QHBoxLayout();
    scheduleButton_ = new QPushButton(tr("Schedule Appointment..."), this);
    scheduleButton_->setToolTip(tr("Schedule new appointment"));
    connect(scheduleButton_, &QPushButton::clicked, this, &AppointmentPanel::onScheduleClicked);
    cancelButton_ = new QPushButton(tr("Cancel Appointment"), this);
    cancelButton_->setToolTip(tr("Cancel selected appointment"));
    connect(cancelButton_, &QPushButton::clicked, this, &AppointmentPanel::onCancelClicked);
    // Appointments are marked completed when served in the waiting queue.

    statusFilter_ = new QComboBox(this);
    statusFilter_->addItems({tr("All statuses"), tr("Pending"), tr("Completed"),
                             tr("Cancelled")});
    connect(statusFilter_, &QComboBox::currentIndexChanged, this,
            [this](int) { refresh(); });

    dateFilterOn_ = new QCheckBox(tr("On date:"), this);
    connect(dateFilterOn_, &QCheckBox::toggled, this, [this](bool) { refresh(); });
    dateFilter_ = new QDateEdit(QDate::currentDate(), this);
    dateFilter_->setCalendarPopup(true);
    dateFilter_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    dateFilter_->setEnabled(false);
    connect(dateFilter_, &QDateEdit::dateChanged, this, [this](const QDate&) {
        if (dateFilterOn_->isChecked()) refresh();
    });
    connect(dateFilterOn_, &QCheckBox::toggled, dateFilter_, &QDateEdit::setEnabled);

    controls->addWidget(scheduleButton_);
    controls->addWidget(cancelButton_);
    controls->addStretch();
    controls->addWidget(new QLabel(tr("Status:"), this));
    controls->addWidget(statusFilter_);
    controls->addWidget(dateFilterOn_);
    controls->addWidget(dateFilter_);
    layout->addLayout(controls);

    // Appointments table
    countLabel_ = new QLabel(this);
    layout->addWidget(countLabel_);

    table_ = new QTableWidget(this);
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels({tr("ID"), tr("Patient"), tr("Date"),
                                       tr("Time"), tr("Status"), tr("Notes")});
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    layout->addWidget(table_, 1);
}

void AppointmentPanel::onScheduleClicked() {
    if (manager_.database().patients().empty()) {
        QMessageBox::information(this, tr("Schedule appointment"),
                                 tr("Register a patient first (Patients tab)."));
        return;
    }
    ScheduleDialog dialog(manager_.database(), this);
    if (dialog.exec() != QDialog::Accepted) return;
    try {
        const model::Appointment& created = manager_.scheduleAppointment(
            dialog.patientId(), dialog.date(), dialog.time(), dialog.notes());
        refresh();
        emit actionPerformed(tr("Scheduled %1 for %2 on %3 at %4")
                                 .arg(QString::fromStdString(created.id()),
                                      QString::fromStdString(created.patientId()),
                                      QString::fromStdString(created.date()),
                                      QString::fromStdString(created.time())));
        emit dataChanged();
    } catch (const utils::ValidationError& error) {
        QMessageBox::warning(this, tr("Cannot schedule appointment"),
                             QString::fromUtf8(error.what()));
    } catch (const std::exception& error) {
        QMessageBox::warning(this, tr("Cannot schedule appointment"),
                             QString::fromUtf8(error.what()));
    }
}

void AppointmentPanel::onCancelClicked() {
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Cancel appointment"),
                                 tr("Select an appointment in the table first."));
        return;
    }
    const QString id = table_->item(row, 0)->text();
    const auto answer = QMessageBox::question(
        this, tr("Cancel appointment"),
        tr("Cancel appointment %1?\nThe record is kept for history (status becomes cancelled).").arg(id),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    try {
        manager_.cancelAppointment(id.toStdString());
        refresh();
        emit actionPerformed(tr("Cancelled appointment %1").arg(id));
        emit dataChanged();
    } catch (const utils::ValidationError& error) {
        QMessageBox::warning(this, tr("Cannot cancel appointment"),
                             QString::fromUtf8(error.what()));
    } catch (const std::exception& error) {
        QMessageBox::warning(this, tr("Cannot cancel appointment"),
                             QString::fromUtf8(error.what()));
    }
}

int AppointmentPanel::selectedRow() const {
    return table_->currentRow();
}

void AppointmentPanel::refresh() {
    rebuildTable();
}

void AppointmentPanel::rebuildTable() {
    updating_ = true;

    // Sort appointments chronologically for display
    std::vector<const model::Appointment*> rows;
    rows.reserve(manager_.database().appointments().size());
    for (const auto& appointment : manager_.database().appointments())
        rows.push_back(appointment.get());
    controller::selectionSort(rows, [](const model::Appointment* a,
                                       const model::Appointment* b) {
        if (a->date() != b->date()) return a->date() < b->date();
        return a->time() < b->time();
    });

    const QString statusFilter = statusFilter_->currentText();
    const bool filterByStatus = statusFilter != tr("All statuses");
    const bool filterByDate = dateFilterOn_->isChecked();
    const QString dateFilter = dateFilter_->date().toString(QStringLiteral("yyyy-MM-dd"));
    const QString today =
        QString::fromStdString(controller::ClinicManager::currentDate());

    table_->setRowCount(0);
    int shown = 0;
    int overdueCount = 0;
    for (const model::Appointment* appointment : rows) {
        const QString status = QString::fromStdString(
            model::appointmentStatusName(appointment->status()));
        if (filterByStatus && status.compare(statusFilter, Qt::CaseInsensitive) != 0)
            continue;
        if (filterByDate &&
            QString::fromStdString(appointment->date()) != dateFilter)
            continue;

        // overdue flag: still pending and dated before today.
        const bool overdue = appointment->isOverdue(today.toStdString());
        if (overdue) ++overdueCount;

        const int row = table_->rowCount();
        table_->insertRow(row);

        auto* idItem = readOnlyItem(QString::fromStdString(appointment->id()));
        table_->setItem(row, 0, idItem);

        // Resolve patient name for display column
        QString patientText = QStringLiteral("(unknown)");
        QString patientTooltip;
        if (const model::PatientRecord* patient = manager_.database().findPatient(
                appointment->patientId())) {
            patientText = QStringLiteral("%1 - %2")
                              .arg(QString::fromStdString(patient->id()),
                                   QString::fromStdString(patient->name()));
            patientTooltip = QString::fromStdString(patient->describe());
        }
        auto* patientItem = readOnlyItem(patientText);
        table_->setItem(row, 1, patientItem);

        auto* dateItem = readOnlyItem(QString::fromStdString(appointment->date()) +
                                      (overdue ? tr(" (Overdue)") : QString()));
        if (overdue) {
            dateItem->setForeground(QBrush(theme::kCategoryEmergency));
        }
        table_->setItem(row, 2, dateItem);

        table_->setItem(row, 3, readOnlyItem(QString::fromStdString(appointment->time())));

        auto* statusItem = readOnlyItem(status);
        statusItem->setForeground(QBrush(theme::statusColor(status)));
        table_->setItem(row, 4, statusItem);

        table_->setItem(row, 5, readOnlyItem(QString::fromStdString(appointment->notes())));
        ++shown;
    }

    countLabel_->setText(tr("Showing %1 of %2 appointment(s) - %3 overdue")
                             .arg(shown)
                             .arg(static_cast<int>(rows.size()))
                             .arg(overdueCount));
    updating_ = false;
}

}  // namespace scapms::gui
