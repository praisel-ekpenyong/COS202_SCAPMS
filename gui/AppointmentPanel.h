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

// Appointments tab. Schedule (patient picker + date/time + notes) and
// cancel routed through the injected ClinicManager; the table is
// status-coloured (via theme::statusColor) and flags overdue pending
// appointments. Status and date filters narrow the view. Chronological
// ordering uses selection sort.
class AppointmentPanel : public QWidget {
    Q_OBJECT

public:
    // The manager is injected (architecture: GUI -> controller -> database);
    // the panel never touches storage behind it. MainWindow owns the manager.
    explicit AppointmentPanel(controller::ClinicManager& manager,
                              QWidget* parent = nullptr);

signals:
    // Human-readable one-liner for the status bar's "last action".
    void actionPerformed(const QString& action);

    // Emitted after every mutation routed through the manager; the other
    // panels connect this to their refresh so no table goes stale.
    void dataChanged();

public slots:
    // Re-reads the database into the table (respecting the active filters).
    // Called by MainWindow after any panel's dataChanged, after File->New,
    // and after Undo.
    void refresh();

private slots:
    void onScheduleClicked();
    void onCancelClicked();

private:
    void buildUi();
    void rebuildTable();
    int selectedRow() const;

    controller::ClinicManager& manager_;
    bool updating_ = false;  // suppresses feedback loops during rebuilds

    QTableWidget* table_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QComboBox* statusFilter_ = nullptr;
    QCheckBox* dateFilterOn_ = nullptr;
    QDateEdit* dateFilter_ = nullptr;
    QPushButton* scheduleButton_ = nullptr;
    QPushButton* cancelButton_ = nullptr;
};

}  // namespace scapms::gui
