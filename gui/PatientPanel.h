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

// Patient records tab. Full CRUD routed through the injected
// ClinicManager - validation failures surface as dialogs and never
// crash. The table is category-coloured (via theme::categoryColor),
// the form switches its fields per category (next-visit date appears
// only for Follow-Up, a note for Emergency), and the runtime-polymorphism
// demo is visible here: every row's priority column and the detail pane are
// produced through the abstract PatientRecord& base reference
// (priority/describe dispatch to the concrete subclass).
class PatientPanel : public QWidget {
    Q_OBJECT

public:
    // The manager is injected (architecture: GUI -> controller -> database);
    // the panel never touches storage behind it. MainWindow owns the manager.
    explicit PatientPanel(controller::ClinicManager& manager,
                          QWidget* parent = nullptr);

signals:
    // Human-readable one-liner for the status bar's "last action".
    void actionPerformed(const QString& action);

    // Emitted after every mutation routed through the manager; the other
    // panels connect this to their refresh so no table goes stale.
    void dataChanged();

public slots:
    // Re-reads the database into the table. Called by MainWindow after any
    // panel's dataChanged, after File->New, and after Undo.
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
    bool updating_ = false;  // suppresses selection feedback during rebuilds
    bool editing_ = false;   // form is bound to the selected record
    QString selectedId_;     // the record the form is bound to (editing_)

    QTableWidget* table_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QLabel* details_ = nullptr;
    QLineEdit* idEdit_ = nullptr;
    QLineEdit* nameEdit_ = nullptr;
    QSpinBox* ageSpin_ = nullptr;
    QLineEdit* contactEdit_ = nullptr;
    QComboBox* categoryBox_ = nullptr;
    QWidget* nextVisitRow_ = nullptr;   // dynamic field: Follow-Up only
    QCheckBox* nextVisitSet_ = nullptr;
    QDateEdit* nextVisitEdit_ = nullptr;
    QLabel* emergencyNote_ = nullptr;   // dynamic field: Emergency only
    QPushButton* addButton_ = nullptr;
    QPushButton* updateButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
};

}  // namespace scapms::gui
