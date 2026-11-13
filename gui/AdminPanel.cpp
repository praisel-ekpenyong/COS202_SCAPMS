#include "gui/AdminPanel.h"

#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace scapms::gui {

AdminPanel::AdminPanel(controller::ClinicManager* manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    buildUi();
    refresh();
}

void AdminPanel::buildUi() {
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel(tr("Administration"), this);
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));
    layout->addWidget(title);

    auto* historyBox = new QGroupBox(tr("Undo History"), this);
    auto* historyLayout = new QVBoxLayout(historyBox);
    historyList_ = new QListWidget(historyBox);
    historyLayout->addWidget(historyList_, 1);

    nextUndoLabel_ = new QLabel(historyBox);
    historyLayout->addWidget(nextUndoLabel_);
    layout->addWidget(historyBox, 1);

    auto* buttonRow = new QHBoxLayout();
    undoButton_ = new QPushButton(tr("Undo"), this);
    undoButton_->setStyleSheet(QStringLiteral("font-weight: bold;"));
    undoButton_->setToolTip(tr("Undo last action (Ctrl+Z)"));
    connect(undoButton_, &QPushButton::clicked, this, &AdminPanel::undoLast);
    buttonRow->addWidget(undoButton_);

    buttonRow->addStretch(1);

    auto* sampleButton = new QPushButton(tr("Load Sample Data"), this);
    sampleButton->setToolTip(tr("Load sample clinic data"));
    connect(sampleButton, &QPushButton::clicked, this, &AdminPanel::loadSampleRequested);
    buttonRow->addWidget(sampleButton);

    auto* clearButton = new QPushButton(tr("Clear Data..."), this);
    clearButton->setToolTip(tr("Clear all clinic data"));
    connect(clearButton, &QPushButton::clicked, this, &AdminPanel::clearAllRequested);
    buttonRow->addWidget(clearButton);

    layout->addLayout(buttonRow);
}

void AdminPanel::refresh() {
    syncHistoryList();
}

void AdminPanel::undoLast() {
    if (manager_->undoDepth() == 0) {
        emit statusMessage(tr("Nothing to undo"));
        return;
    }
    const std::string description = manager_->nextUndoDescription();
    manager_->undo();
    emit statusMessage(tr("Undone: %1").arg(QString::fromStdString(description)));
    refresh();
    emit dataChanged();
}

void AdminPanel::syncHistoryList() {
    const int depth = static_cast<int>(manager_->undoDepth());
    const QString top = QString::fromStdString(manager_->nextUndoDescription());

    if (depth == 0) {
        historyEntries_.clear();
    } else if (historyEntries_.size() == depth) {
        historyEntries_[0] = top;
    } else if (depth == historyEntries_.size() + 1) {
        historyEntries_.prepend(top);
    } else if (depth < historyEntries_.size()) {
        while (historyEntries_.size() > depth) {
            historyEntries_.removeFirst();
        }
    } else {
        historyEntries_.clear();
        historyEntries_.prepend(top);
    }

    historyList_->clear();
    if (historyEntries_.isEmpty()) {
        auto* empty = new QListWidgetItem(tr("No undoable operations yet"));
        empty->setFlags(Qt::NoItemFlags);
        historyList_->addItem(empty);
    } else {
        for (int i = 0; i < historyEntries_.size(); ++i) {
            auto* entry = new QListWidgetItem(
                QStringLiteral("%1.  %2").arg(i + 1).arg(historyEntries_.at(i)));
            if (i == 0) {
                QFont bold = entry->font();
                bold.setBold(true);
                entry->setFont(bold);
            }
            historyList_->addItem(entry);
        }
        if (depth > historyEntries_.size()) {
            auto* gap = new QListWidgetItem(
                tr("... %n older operation(s) not shown", "", depth - historyEntries_.size()));
            gap->setFlags(Qt::NoItemFlags);
            historyList_->addItem(gap);
        }
    }

    nextUndoLabel_->setText(historyEntries_.isEmpty()
                                ? tr("Undo actions: 0 - nothing to undo")
                                : tr("Undo actions: %1 - next: %2").arg(depth).arg(
                                      historyEntries_.first()));
    undoButton_->setEnabled(depth > 0);
    emit undoDepthChanged(depth);
}

}  // namespace scapms::gui
