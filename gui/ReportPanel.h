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

// Reports tab: a picker list on the left chooses
// one of the four reports - most frequently attended patients, pending
// appointments, category distribution, daily appointment summary - and the
// table on the right renders it. All computation happens in
// controller::ReportEngine over the injected ClinicManager's const database
// view (data flow: GUI -> controller -> database; this panel never touches
// storage directly and holds no records itself - every render re-reads the
// manager, so reports never go stale).
//
// "Export report" writes the rendered report to a .txt chosen with a
// file dialog; the file body is the same ReportEngine text rendering the
// tests pin. Tooltips throughout; colours only from Theme.h.
class ReportPanel : public QWidget {
    Q_OBJECT

public:
    // Non-owning: `manager` must outlive this panel (MainWindow owns it).
    explicit ReportPanel(controller::ClinicManager* manager,
                         QWidget* parent = nullptr);

signals:
    // Status-bar feed (MainWindow::setAlgorithmUsed): names the
    // sorting algorithm behind the report just rendered.
    void algorithmUsed(const QString& algorithm);
    // Status-bar feed (MainWindow::showLastAction): export/refresh notes.
    void statusMessage(const QString& message);

public:
    // The text body for the export: the engine's rendering of the report
    // currently on screen, under a title + generated timestamp. Public so
    // the exact bytes the export writes are verifiable offscreen.
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
    // Shows the table or, when it has no rows, the report's empty-state
    // message instead (acceptance: a sensible message, never an empty grid).
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
    // Rendered alongside the table so the export matches what staff see.
    std::string exportedBody_;
    QString exportedTitle_;
};

}  // namespace scapms::gui
