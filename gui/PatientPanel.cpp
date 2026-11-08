#include "gui/PatientPanel.h"

#include <QBrush>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "controller/ClinicManager.h"
#include "gui/Theme.h"
#include "model/ClinicDatabase.h"
#include "model/FollowUpPatient.h"
#include "model/PatientCategory.h"
#include "model/PatientRecord.h"
#include "utils/Exceptions.h"

namespace scapms::gui {

namespace {

// Helpers to access description and priority via base reference.
QString describeThroughBase(const model::PatientRecord& record) {
    return QString::fromStdString(record.describe());
}

int priorityThroughBase(const model::PatientRecord& record) {
    return record.priority();
}

QTableWidgetItem* readOnlyItem(const QString& text) {
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

}  // namespace

PatientPanel::PatientPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    buildUi();
    refresh();
}

void PatientPanel::buildUi() {
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel(tr("Patient Records"), this);
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));
    layout->addWidget(title);

    // --- entry form (QFormLayout keeps the field labels column-aligned) ----
    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);

    idEdit_ = new QLineEdit(this);
    idEdit_->setReadOnly(true);
    idEdit_->setPlaceholderText(tr("Assigned automatically (P-xxxx)"));
    form->addRow(tr("ID:"), idEdit_);

    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(tr("e.g. Chidinma Nwankwo"));
    form->addRow(tr("Name:"), nameEdit_);

    ageSpin_ = new QSpinBox(this);
    ageSpin_->setRange(0, 120);  // age validation, enforced at the widget too
    form->addRow(tr("Age:"), ageSpin_);

    contactEdit_ = new QLineEdit(this);
    contactEdit_->setPlaceholderText(tr("e.g. 0801 234 5678"));
    form->addRow(tr("Contact:"), contactEdit_);

    categoryBox_ = new QComboBox(this);
    // Combo order mirrors the enum order {General, Emergency, FollowUp};
    // currentCategory relies on that mapping.
    for (const auto category : {model::PatientCategory::General,
                                model::PatientCategory::Emergency,
                                model::PatientCategory::FollowUp})
        categoryBox_->addItem(QString::fromStdString(model::categoryName(category)),
                              static_cast<int>(category));
    connect(categoryBox_, &QComboBox::currentIndexChanged,
            this, &PatientPanel::onCategoryChanged);
    form->addRow(tr("Category:"), categoryBox_);

    // Dynamic field: the next-visit date exists only for Follow-Up.
    nextVisitRow_ = new QWidget(this);
    auto* nextVisitLayout = new QHBoxLayout(nextVisitRow_);
    nextVisitLayout->setContentsMargins(0, 0, 0, 0);
    nextVisitSet_ = new QCheckBox(tr("Scheduled"), nextVisitRow_);
    nextVisitEdit_ = new QDateEdit(nextVisitRow_);
    nextVisitEdit_->setCalendarPopup(true);
    nextVisitEdit_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    nextVisitEdit_->setDate(QDate::currentDate());
    nextVisitLayout->addWidget(nextVisitSet_);
    nextVisitLayout->addWidget(nextVisitEdit_);
    nextVisitLayout->addStretch();
    connect(nextVisitSet_, &QCheckBox::toggled, nextVisitEdit_, &QDateEdit::setEnabled);
    nextVisitEdit_->setEnabled(false);
    form->addRow(tr("Next visit:"), nextVisitRow_);

    layout->addLayout(form);

    // Dynamic field: the severity note exists only for Emergency.
    emergencyNote_ = new QLabel(
        tr("Emergency - this patient is served before all other waiting patients (queue priority 3)."),
        this);
    emergencyNote_->setStyleSheet(
        QStringLiteral("color: #%1; font-weight: bold;")
            .arg(theme::kCategoryEmergency.name().mid(1)));  // theme colour, no local palette
    emergencyNote_->setWordWrap(true);
    layout->addWidget(emergencyNote_);

    // --- button row ---------------------------------------------------------
    auto* buttons = new QHBoxLayout();
    addButton_ = new QPushButton(tr("Add Patient"), this);
    addButton_->setToolTip(tr("Register new patient"));
    connect(addButton_, &QPushButton::clicked, this, &PatientPanel::onAddClicked);
    updateButton_ = new QPushButton(tr("Update Patient"), this);
    updateButton_->setToolTip(tr("Update selected patient"));
    connect(updateButton_, &QPushButton::clicked, this, &PatientPanel::onUpdateClicked);
    deleteButton_ = new QPushButton(tr("Delete Patient"), this);
    deleteButton_->setToolTip(tr("Delete selected patient"));
    connect(deleteButton_, &QPushButton::clicked, this, &PatientPanel::onDeleteClicked);
    auto* newButton = new QPushButton(tr("New Patient Form"), this);
    newButton->setToolTip(tr("Clear form for new entry"));
    connect(newButton, &QPushButton::clicked, this, &PatientPanel::onNewPatientClicked);
    buttons->addWidget(addButton_);
    buttons->addWidget(updateButton_);
    buttons->addWidget(deleteButton_);
    buttons->addWidget(newButton);
    buttons->addStretch();
    layout->addLayout(buttons);

    // --- table ---------------------------------------------------------------
    countLabel_ = new QLabel(this);
    layout->addWidget(countLabel_);

    table_ = new QTableWidget(this);
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels({tr("ID"), tr("Name"), tr("Age"),
                                       tr("Contact"), tr("Category"),
                                       tr("Queue priority")});
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    connect(table_, &QTableWidget::itemSelectionChanged,
            this, &PatientPanel::onSelectionChanged);
    layout->addWidget(table_, 1);

    // detail pane: shows describe through the base reference.
    details_ = new QLabel(tr("Select a patient to see details."), this);
    details_->setWordWrap(true);
    layout->addWidget(details_);
}

void PatientPanel::onCategoryChanged(int index) {
    Q_UNUSED(index);
    if (updating_) return;
    updateDynamicFields();
}

void PatientPanel::updateDynamicFields() {
    // dynamic fields: switching the category combo reconfigures the form
    // live - the next-visit field exists only for Follow-Up, the severity
    // note only for Emergency.
    const auto category = currentCategory();
    nextVisitRow_->setVisible(category == model::PatientCategory::FollowUp);
    emergencyNote_->setVisible(category == model::PatientCategory::Emergency);
}

void PatientPanel::onSelectionChanged() {
    if (updating_) return;
    const int row = table_->currentRow();
    if (row < 0) {
        updateButtonStates();
        return;
    }
    const QString id = table_->item(row, 0)->text();
    const model::PatientRecord* record =
        manager_.database().findPatient(id.toStdString());  // const read, no cache effect
    if (record == nullptr) return;
    populateFormFrom(*record);
}

void PatientPanel::populateFormFrom(const model::PatientRecord& record) {
    updating_ = true;  // combo feedback must not re-enter while we load
    editing_ = true;
    selectedId_ = QString::fromStdString(record.id());

    idEdit_->setText(selectedId_);
    nameEdit_->setText(QString::fromStdString(record.name()));
    ageSpin_->setValue(record.age());
    contactEdit_->setText(QString::fromStdString(record.contact()));

    const int categoryIndex = categoryBox_->findData(static_cast<int>(record.category()));
    if (categoryIndex >= 0) categoryBox_->setCurrentIndex(categoryIndex);
    // Category is immutable after registration - the form shows
    // it, but edit mode cannot change it.
    categoryBox_->setEnabled(false);

    // nextVisitDate lives only on FollowUpPatient, so after the base-reference
    // processing above the form queries the concrete type (dynamic_cast) -
    // the same structure manager uses for Appointable.
    nextVisitSet_->setChecked(false);
    if (const auto* followUp = dynamic_cast<const model::FollowUpPatient*>(&record)) {
        const std::string& nextVisit = followUp->nextVisitDate();
        if (!nextVisit.empty()) {
            nextVisitSet_->setChecked(true);
            nextVisitEdit_->setDate(QDate::fromString(QString::fromStdString(nextVisit),
                                                      QStringLiteral("yyyy-MM-dd")));
        }
    }
    updating_ = false;

    updateDynamicFields();
    details_->setText(describeThroughBase(record));
    updateButtonStates();
}

void PatientPanel::clearForm() {
    updating_ = true;
    editing_ = false;
    selectedId_.clear();
    idEdit_->clear();
    nameEdit_->clear();
    ageSpin_->setValue(0);
    contactEdit_->clear();
    categoryBox_->setEnabled(true);
    categoryBox_->setCurrentIndex(0);  // General - fires updateDynamicFields
    nextVisitSet_->setChecked(false);
    updating_ = false;
    updateDynamicFields();
    details_->setText(tr("Select a patient to see details."));
    updateButtonStates();
}

void PatientPanel::updateButtonStates() {
    updateButton_->setEnabled(editing_);
    deleteButton_->setEnabled(editing_);
}

void PatientPanel::showError(const QString& context, const std::exception& error) {
    // validation dialog: the specific wording comes straight from the
    // controller's exception - invalid input never crashes the panel.
    QMessageBox::warning(this, context, QString::fromUtf8(error.what()));
}

model::PatientCategory PatientPanel::currentCategory() const {
    return static_cast<model::PatientCategory>(categoryBox_->currentData().toInt());
}

QString PatientPanel::nextVisitDateString() const {
    if (currentCategory() != model::PatientCategory::FollowUp) return QString();
    if (!nextVisitSet_->isChecked()) return QString();
    return nextVisitEdit_->date().toString(QStringLiteral("yyyy-MM-dd"));
}

void PatientPanel::onAddClicked() {
    try {
        const model::PatientRecord& created = manager_.registerPatient(
            nameEdit_->text().trimmed().toStdString(), ageSpin_->value(),
            contactEdit_->text().trimmed().toStdString(), currentCategory(),
            nextVisitDateString().toStdString());
        refresh();
        clearForm();
        emit actionPerformed(tr("Registered patient %1 - %2")
                                 .arg(QString::fromStdString(created.id()),
                                      QString::fromStdString(created.name())));
        emit dataChanged();
    } catch (const utils::ValidationError& error) {
        showError(tr("Cannot register patient"), error);
    } catch (const utils::DuplicateIdError& error) {
        showError(tr("Cannot register patient"), error);
    } catch (const std::exception& error) {  // defensive net - never crash
        showError(tr("Cannot register patient"), error);
    }
}

void PatientPanel::onUpdateClicked() {
    if (!editing_) {
        QMessageBox::information(this, tr("Update patient"),
                                 tr("Select a patient in the table first."));
        return;
    }
    try {
        manager_.editPatient(selectedId_.toStdString(),
                             nameEdit_->text().trimmed().toStdString(), ageSpin_->value(),
                             contactEdit_->text().trimmed().toStdString(),
                             nextVisitDateString().toStdString());
        refresh();
        emit actionPerformed(tr("Updated patient %1").arg(selectedId_));
        emit dataChanged();
    } catch (const utils::ValidationError& error) {
        showError(tr("Cannot update patient"), error);
    } catch (const std::exception& error) {
        showError(tr("Cannot update patient"), error);
    }
}

void PatientPanel::onDeleteClicked() {
    if (!editing_) {
        QMessageBox::information(this, tr("Delete patient"),
                                 tr("Select a patient in the table first."));
        return;
    }
    const std::string id = selectedId_.toStdString();
    const std::size_t appointmentCount =
        manager_.database().appointmentsFor(id).size();
    const auto answer = QMessageBox::question(
        this, tr("Delete patient"),
        tr("Delete patient %1?\nTheir %2 appointment(s) will be removed as well, "
           "and the deletion can be undone (Edit -> Undo).")
            .arg(selectedId_)
            .arg(appointmentCount),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    try {
        manager_.deletePatient(id);
        refresh();
        clearForm();
        emit actionPerformed(tr("Deleted patient %1").arg(selectedId_));
        emit dataChanged();
    } catch (const utils::ValidationError& error) {
        showError(tr("Cannot delete patient"), error);
    } catch (const std::exception& error) {
        showError(tr("Cannot delete patient"), error);
    }
}

void PatientPanel::onNewPatientClicked() {
    table_->clearSelection();
    clearForm();
}

void PatientPanel::refresh() {
    // Rebuild from the database view; selection/edit state survives if the
    // record still exists, and edit mode exits cleanly if it was deleted.
    updating_ = true;
    table_->setRowCount(0);
    const auto& patients = manager_.database().patients();
    for (const auto& record : patients) {
        const model::PatientRecord& patient = *record;
        const int row = table_->rowCount();
        table_->insertRow(row);
        auto* idItem = readOnlyItem(QString::fromStdString(patient.id()));
        const QString tooltip = describeThroughBase(patient);
        table_->setItem(row, 0, idItem);
        table_->setItem(row, 1, readOnlyItem(QString::fromStdString(patient.name())));
        table_->setItem(row, 2, readOnlyItem(QString::number(patient.age())));
        table_->setItem(row, 3, readOnlyItem(QString::fromStdString(patient.contact())));
        table_->setItem(row, 4, readOnlyItem(QString::fromStdString(
                                    model::categoryName(patient.category()))));
        // the priority number comes from the record's own override.
        auto* priorityItem =
            readOnlyItem(QString::number(priorityThroughBase(patient)));
        table_->setItem(row, 5, priorityItem);

        // category colouring: one brush for the whole row, from the theme.
        const QBrush rowColor(theme::categoryColor(QString::fromStdString(
            model::categoryName(patient.category()))));
        for (int column = 0; column < table_->columnCount(); ++column)
            table_->item(row, column)->setForeground(rowColor);
    }
    countLabel_->setText(tr("%n patient(s) registered", "", static_cast<int>(patients.size())));
    updating_ = false;

    if (editing_) {
        const model::PatientRecord* record =
            manager_.database().findPatient(selectedId_.toStdString());
        if (record == nullptr) {
            clearForm();  // deleted elsewhere (e.g. another panel, later Undo)
        } else {
            selectRowOfPatient(selectedId_);
            details_->setText(describeThroughBase(*record));
        }
    } else {
        details_->setText(tr("Select a patient to see details."));
    }
    updateButtonStates();
}

void PatientPanel::selectRowOfPatient(const QString& patientId) {
    for (int row = 0; row < table_->rowCount(); ++row) {
        if (table_->item(row, 0)->text() == patientId) {
            table_->selectRow(row);
            return;
        }
    }
}

}  // namespace scapms::gui
