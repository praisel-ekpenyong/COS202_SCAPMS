#pragma once

#include <QWidget>

namespace scapms::controller {
class ClinicManager;
}

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

// Appointments tab: handles scheduling and cancelling appointments,
// status filtering, and chronological display of clinic appointments.
class AppointmentPanel : public QWidget {
    Q_OBJECT

public:
    explicit AppointmentPanel(controller::ClinicManager& manager,
                              QWidget* parent = nullptr);

signals:
    void actionPerformed(const QString& action);
    void dataChanged();

public slots:
    void refresh();

private slots:
    void onScheduleClicked();
    void onCancelClicked();

private:
    void buildUi();
    void rebuildTable();
    int selectedRow() const;

    controller::ClinicManager& manager_;
    bool updating_ = false;  // flag to ignore events while rebuilding table

    QTableWidget* table_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QComboBox* statusFilter_ = nullptr;
    QCheckBox* dateFilterOn_ = nullptr;
    QDateEdit* dateFilter_ = nullptr;
    QPushButton* scheduleButton_ = nullptr;
    QPushButton* cancelButton_ = nullptr;
};

}  // namespace scapms::gui
