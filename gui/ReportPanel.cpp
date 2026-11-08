#include "gui/ReportPanel.h"

#include <QBrush>
#include <QDate>
#include <QDateEdit>
#include <QDir>
#include <QIODevice>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QShowEvent>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "controller/ClinicManager.h"
#include "controller/ReportEngine.h"
#include "gui/Theme.h"
#include "utils/AppInfo.h"

namespace scapms::gui {

namespace {

// A read-only table cell; `color` (from Theme.h) tints the text,
// and an optional tooltip lands on the cell.
QTableWidgetItem* makeCell(const QString& text, const QColor& color = {},
                           const QString& tooltip = {}) {
    auto* item = new QTableWidgetItem(text);
    if (color.isValid()) item->setForeground(QBrush(color));
    if (!tooltip.isEmpty()) item->setToolTip(tooltip);
    return item;
}

// The selected report's title, one-line rule (shown under the title), and
// the suggested export file name.
struct ReportMeta {
    QString title;
    QString rule;
    QString fileName;
};

ReportMeta reportMeta(int row) {
    switch (row) {
        case 1:
            return {ReportPanel::tr("Pending appointments"),
                    ReportPanel::tr("Every pending appointment, soonest date "
                                    "first. Overdue = pending and dated "
                                    "before today."),
                    QStringLiteral("scapms_pending_appointments.txt")};
        case 2:
            return {ReportPanel::tr("Category distribution"),
                    ReportPanel::tr("Patients per category with each "
                                    "category's share of all patients."),
                    QStringLiteral("scapms_category_distribution.txt")};
        case 3:
            return {ReportPanel::tr("Daily appointment summary"),
                    ReportPanel::tr("Appointments on the chosen day, split by "
                                    "status."),
                    QStringLiteral("scapms_daily_summary.txt")};
        default:
            return {ReportPanel::tr("Most frequently attended patients"),
                    ReportPanel::tr("Top 5 patients ranked by completed "
                                    "appointments; ties broken by name."),
                    QStringLiteral("scapms_most_attended.txt")};
    }
}

}  // namespace

ReportPanel::ReportPanel(controller::ClinicManager* manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    // Layout (kept): QHBoxLayout - report picker on the left,
    // report content on the right.
    auto* layout = new QHBoxLayout(this);
    buildPicker();
    layout->addWidget(picker_);
    layout->addWidget(buildContent(), 1);

    connect(picker_, &QListWidget::currentRowChanged, this,
            &ReportPanel::onReportSelected);
    connect(dateEdit_, &QDateEdit::dateChanged, this,
            &ReportPanel::onDateChanged);
    connect(exportButton_, &QPushButton::clicked, this,
            &ReportPanel::onExportRequested);

    picker_->setCurrentRow(0);  // renders the first report
}

void ReportPanel::buildPicker() {
    picker_ = new QListWidget(this);
    picker_->setMaximumWidth(240);

    const struct {
        QString label;
        QString tooltip;
    } entries[] = {
        {tr("Most Frequently Attended Patients"),
         tr("Patients ranked by completed appointments - top 5, ties by "
            "name.")},
        {tr("Patients with Pending Appointments"),
         tr("Pending appointments sorted by date; overdue ones flagged.")},
        {tr("Patient Category Distribution"),
         tr("Patient counts and percentages per category.")},
        {tr("Daily Appointment Summary"),
         tr("Totals by status for one day - pick the day below the list.")},
    };
    for (const auto& entry : entries) {
        auto* item = new QListWidgetItem(entry.label, picker_);
        item->setToolTip(entry.tooltip);  // tooltips on the pickers
    }
}

QWidget* ReportPanel::buildContent() {
    auto* content = new QVBoxLayout;

    // Day picker for the daily summary (defaults to today). Hidden for
    // the other three reports.
    dateEdit_ = new QDateEdit(QDate::currentDate(), this);
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    dateEdit_->hide();

    heading_ = new QLabel(this);
    heading_->setWordWrap(true);

    table_ = new QTableWidget(this);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setAlternatingRowColors(true);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);

    // Empty-data case (acceptance): a message instead of an empty grid.
    emptyState_ = new QLabel(this);
    emptyState_->setWordWrap(true);
    emptyState_->hide();

    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);
    auto* refreshButton = new QPushButton(tr("Refresh"), this);
    refreshButton->setToolTip(tr("Refresh report"));
    connect(refreshButton, &QPushButton::clicked, this, [this] {
        renderCurrentReport();
        emit statusMessage(tr("Reports refreshed"));
    });
    buttons->addWidget(refreshButton);

    exportButton_ = new QPushButton(tr("Export report (.txt)..."), this);
    exportButton_->setToolTip(tr("Export report to text file"));
    buttons->addWidget(exportButton_);

    content->addWidget(dateEdit_);
    content->addWidget(heading_);
    content->addWidget(table_, 1);
    content->addWidget(emptyState_);
    content->addLayout(buttons);

    auto* holder = new QWidget(this);
    holder->setLayout(content);
    return holder;
}

void ReportPanel::onReportSelected(int row) {
    current_ = static_cast<ReportKind>(row < 0 ? 0 : row);
    renderCurrentReport();
}

void ReportPanel::onDateChanged() {
    // Only the daily summary reacts to the day picker.
    if (current_ == ReportKind::DailySummary) renderCurrentReport();
}

void ReportPanel::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // Re-read on every activation: the records may have changed in any other
    // tab (the Qt-free ClinicManager has no change signals to subscribe to).
    renderCurrentReport();
}

void ReportPanel::renderCurrentReport() {
    if (!manager_) return;
    const controller::ReportEngine engine(manager_->database());
    const ReportMeta meta = reportMeta(static_cast<int>(current_));
    heading_->setText(QStringLiteral("<b>%1</b><br>%2").arg(meta.title, meta.rule));
    exportedTitle_ = meta.title;

    dateEdit_->setVisible(current_ == ReportKind::DailySummary);

    switch (current_) {
        case ReportKind::MostAttended:
            renderMostAttended(engine);
            break;
        case ReportKind::Pending:
            renderPending(engine);
            break;
        case ReportKind::CategoryDistribution:
            renderCategoryDistribution(engine);
            break;
        case ReportKind::DailySummary:
            renderDailySummary(engine);
            break;
    }

    // status bar: every report is ordered by the merge sort.
    emit algorithmUsed(tr("Merge Sort"));
}

void ReportPanel::beginTable(const QStringList& headers) {
    table_->clear();
    table_->setColumnCount(headers.size());
    table_->setHorizontalHeaderLabels(headers);
    table_->setRowCount(0);
}

void ReportPanel::finishTable(const QString& emptyMessage) {
    const bool empty = table_->rowCount() == 0;
    table_->setVisible(!empty);
    emptyState_->setText(empty ? emptyMessage : QString());
    emptyState_->setVisible(empty);
    if (!empty) table_->resizeColumnsToContents();
}

void ReportPanel::renderMostAttended(const controller::ReportEngine& engine) {
    const auto rows = engine.mostAttended();

    beginTable({tr("Rank"), tr("Patient ID"), tr("Name"), tr("Category"),
                tr("Completed")});
    int rank = 1;
    for (const auto& row : rows) {
        const int y = table_->rowCount();
        table_->insertRow(y);
        const QString category =
            QString::fromStdString(model::categoryName(row.category));
        table_->setItem(y, 0, makeCell(QString::number(rank)));
        table_->setItem(y, 1, makeCell(QString::fromStdString(row.patientId)));
        table_->setItem(y, 2, makeCell(QString::fromStdString(row.name)));
        table_->setItem(y, 3,
                        makeCell(category, theme::categoryColor(category),
                                 tr("Patient category")));
        table_->setItem(y, 4,
                        makeCell(QString::number(row.completedCount), QColor(),
                                 tr("Appointments with status \"completed\"")));
        ++rank;
    }
    finishTable(tr("No completed appointments yet - nothing to rank."));
    exportedBody_ = controller::ReportEngine::mostAttendedText(rows);
}

void ReportPanel::renderPending(const controller::ReportEngine& engine) {
    const auto rows =
        engine.pendingAppointments(controller::ClinicManager::currentDate());

    beginTable({tr("Appointment"), tr("Patient"), tr("Name"), tr("Date"),
                tr("Time"), tr("Status")});
    for (const auto& row : rows) {
        const int y = table_->rowCount();
        table_->insertRow(y);
        table_->setItem(y, 0,
                        makeCell(QString::fromStdString(row.appointmentId)));
        table_->setItem(y, 1, makeCell(QString::fromStdString(row.patientId)));
        table_->setItem(y, 2,
                        makeCell(QString::fromStdString(row.patientName)));
        table_->setItem(y, 3, makeCell(QString::fromStdString(row.date)));
        table_->setItem(y, 4, makeCell(QString::fromStdString(row.time)));
        if (row.overdue) {
            // overdue flagged - amber (Theme.h's pending colour) and a
            // tooltip saying why.
            table_->setItem(
                y, 5,
                makeCell(tr("Overdue"),
                         theme::statusColor(QStringLiteral("pending")),
                         tr("Pending appointment dated before today")));
        } else {
            table_->setItem(
                y, 5,
                makeCell(tr("Scheduled"),
                         theme::statusColor(QStringLiteral("cancelled"))));
        }
    }
    finishTable(tr("No pending appointments."));
    exportedBody_ = controller::ReportEngine::pendingAppointmentsText(rows);
}

void ReportPanel::renderCategoryDistribution(
    const controller::ReportEngine& engine) {
    const auto rows = engine.categoryDistribution();
    int total = 0;
    for (const auto& row : rows) total += row.count;

    beginTable({tr("Category"), tr("Patients"), tr("Share")});
    if (total > 0) {
        for (const auto& row : rows) {
        const int y = table_->rowCount();
        table_->insertRow(y);
        const QString category =
            QString::fromStdString(model::categoryName(row.category));
        table_->setItem(y, 0,
                        makeCell(category, theme::categoryColor(category),
                                 tr("Patient category")));
        table_->setItem(y, 1, makeCell(QString::number(row.count)));
        table_->setItem(
            y, 2,
            makeCell(
                QString::fromStdString(
                    controller::ReportEngine::sharePercentText(row.sharePercent)) +
                    QStringLiteral("%"),
                QColor(),
                tr("%1 of %2 patients").arg(row.count).arg(total)));
        }
    }
    finishTable(tr("No patients registered yet - nothing to distribute."));
    exportedBody_ = controller::ReportEngine::categoryDistributionText(rows);
}

void ReportPanel::renderDailySummary(const controller::ReportEngine& engine) {
    const std::string day =
        dateEdit_->date().toString(Qt::ISODate).toStdString();
    const auto summary = engine.dailySummary(day);

    beginTable({tr("Status"), tr("Appointments")});
    const struct {
        QString label;
        int count;
        QString statusName;
    } rows[] = {
        {tr("Total"), summary.total, QString()},
        {tr("Completed"), summary.completed, QStringLiteral("completed")},
        {tr("Pending"), summary.pending, QStringLiteral("pending")},
        {tr("Cancelled"), summary.cancelled, QStringLiteral("cancelled")},
    };
    for (const auto& row : rows) {
        const int y = table_->rowCount();
        table_->insertRow(y);
        table_->setItem(y, 0,
                        makeCell(row.label,
                                 row.statusName.isEmpty()
                                     ? QColor()
                                     : theme::statusColor(row.statusName)));
        table_->setItem(y, 1, makeCell(QString::number(row.count)));
    }
    // A day with nothing on it still shows the zero rows - the confirmation
    // IS the report - so no empty-state swap here.
    finishTable(QString());
    exportedBody_ = controller::ReportEngine::dailySummaryText(summary);
}

QString ReportPanel::exportText() const {
    QString text;
    text += QStringLiteral("%1 - %2\n").arg(
        QString::fromLatin1(utils::applicationName()), exportedTitle_);
    text += QStringLiteral("Generated: %1 %2\n\n")
                .arg(QString::fromStdString(
                         controller::ClinicManager::currentDate()),
                     QString::fromStdString(
                         controller::ClinicManager::currentTime()));
    text += QString::fromStdString(exportedBody_);
    return text;
}

void ReportPanel::onExportRequested() {
    if (!manager_) return;
    const ReportMeta meta = reportMeta(static_cast<int>(current_));
    const QString documents =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export report"), QDir(documents).filePath(meta.fileName),
        tr("Text files (*.txt);;All files (*)"));
    if (path.isEmpty()) return;

    // Report export placement decision: utils::FileHandler is deliberately format-blind CSV I/O -
    // RFC 4180 quoting is the wrong tool for a plain-text report - so the
    // .txt write lives here in the view. QSaveFile, so a failed write leaves
    // no half-written report behind.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export failed"),
                             tr("Could not open %1 for writing.")
                                 .arg(QDir::toNativeSeparators(path)));
        return;
    }
    file.write(exportText().toUtf8());
    if (!file.commit()) {
        QMessageBox::warning(this, tr("Export failed"),
                             tr("Writing %1 failed.")
                                 .arg(QDir::toNativeSeparators(path)));
        return;
    }
    emit statusMessage(tr("Report exported to %1")
                           .arg(QDir::toNativeSeparators(path)));
}

}  // namespace scapms::gui
