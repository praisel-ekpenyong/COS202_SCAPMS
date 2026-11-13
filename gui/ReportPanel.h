#pragma once

#include <QString>
#include <QStringList>
#include <QWidget>

#include <string>

namespace scapms::controller {
class ClinicManager;
class ReportEngine;
}

class QDateEdit;
class QLabel;
class QListWidget;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

// Reports tab for clinic statistics and export.
class ReportPanel : public QWidget {
    Q_OBJECT

public:
    explicit ReportPanel(controller::ClinicManager* manager,
                         QWidget* parent = nullptr);

signals:
    // Status bar update for sorting algorithm used in report.
    void algorithmUsed(const QString& algorithm);
    // Status bar update message.
    void statusMessage(const QString& message);

public:
    // Formats report text for exporting to file.
    QString exportText() const;

private slots:
    void onReportSelected(int row);
    void onDateChanged();
    void onExportRequested();

private:
    enum class ReportKind {
        MostAttended = 0,
        Pending,
        CategoryDistribution,
        DailySummary,
    };

    void buildPicker();
    QWidget* buildContent();
    void renderCurrentReport();
    void renderMostAttended(const controller::ReportEngine& engine);
    void renderPending(const controller::ReportEngine& engine);
    void renderCategoryDistribution(const controller::ReportEngine& engine);
    void renderDailySummary(const controller::ReportEngine& engine);

    // Prepares the shared table for a report: clears it and sets the headers.
    void beginTable(const QStringList& headers);
    // Shows table or displays message if there is no data to show.
    void finishTable(const QString& emptyMessage);

    void showEvent(QShowEvent* event) override;

    controller::ClinicManager* manager_;
    QListWidget* picker_;
    QDateEdit* dateEdit_;
    QLabel* heading_;
    QTableWidget* table_;
    QLabel* emptyState_;
    QPushButton* exportButton_;

    ReportKind current_ = ReportKind::MostAttended;
    // Text representation of the report for export
    std::string exportedBody_;
    QString exportedTitle_;
};

}  // namespace scapms::gui
