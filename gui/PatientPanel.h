#pragma once

#include <exception>
#include <QWidget>

#include "model/PatientCategory.h"

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::model {
class PatientRecord;
}

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

namespace scapms::gui {

// Patient records tab: handles registration, editing, deletion,
// and viewing of patient records with category-specific options.
class PatientPanel : public QWidget {
    Q_OBJECT

public:
    explicit PatientPanel(controller::ClinicManager& manager,
                          QWidget* parent = nullptr);

signals:
    void actionPerformed(const QString& action);
    void dataChanged();

public slots:
    void refresh();

private slots:
    void onCategoryChanged(int index);
    void onSelectionChanged();
    void onAddClicked();
    void onUpdateClicked();
    void onDeleteClicked();
    void onNewPatientClicked();

private:
    void buildUi();
    void updateDynamicFields();
    void populateFormFrom(const model::PatientRecord& record);
    void clearForm();
    void updateButtonStates();
    void showError(const QString& context, const std::exception& error);
    model::PatientCategory currentCategory() const;
    QString nextVisitDateString() const;
    void selectRowOfPatient(const QString& patientId);

    controller::ClinicManager& manager_;
    bool updating_ = false;  // ignore selection changes while rebuilding table
    bool editing_ = false;   // true if currently editing an existing record
    QString selectedId_;     // ID of patient selected in table

    QTableWidget* table_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QLabel* details_ = nullptr;
    QLineEdit* idEdit_ = nullptr;
    QLineEdit* nameEdit_ = nullptr;
    QSpinBox* ageSpin_ = nullptr;
    QLineEdit* contactEdit_ = nullptr;
    QComboBox* categoryBox_ = nullptr;
    QWidget* nextVisitRow_ = nullptr;   // visible for Follow-Up only
    QCheckBox* nextVisitSet_ = nullptr;
    QDateEdit* nextVisitEdit_ = nullptr;
    QLabel* emergencyNote_ = nullptr;   // visible for Emergency only
    QPushButton* addButton_ = nullptr;
    QPushButton* updateButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
};

}  // namespace scapms::gui
