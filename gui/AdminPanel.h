#pragma once

#include <QStringList>
#include <QWidget>

#include "controller/ClinicManager.h"

class QLabel;
class QListWidget;
class QPushButton;

namespace scapms::gui {

// Admin tab: displays undo history and provides administrative actions
// (undo, sample data loading, clear database).
class AdminPanel : public QWidget {
    Q_OBJECT

public:
    explicit AdminPanel(controller::ClinicManager* manager, QWidget* parent = nullptr);

public slots:
    void refresh();
    void undoLast();

signals:
    void statusMessage(const QString& message);
    void undoDepthChanged(int depth);
    void dataChanged();
    void clearAllRequested();
    void loadSampleRequested();

private:
    void buildUi();
    void syncHistoryList();

    controller::ClinicManager* manager_;
    QListWidget* historyList_ = nullptr;
    QLabel* nextUndoLabel_ = nullptr;
    QPushButton* undoButton_ = nullptr;
    // List of action descriptions for undo history
    QStringList historyEntries_;
};

}  // namespace scapms::gui
