#pragma once

#include <QStringList>
#include <QWidget>

#include "controller/ClinicManager.h"

class QLabel;
class QListWidget;
class QPushButton;

namespace scapms::gui {

// Admin tab: the undo stack made visible and reversible, plus the
// data-maintenance actions.
//
// The manager exposes only the stack top (nextUndoDescription) - a
// std::stack cannot be iterated and deliberately kept the container
// private - so the full newest-first history is accumulated HERE,
// in the panel: every syncHistoryList diffs manager.undoDepth against the
// accumulated list length and prepends (new operation) or drops (undo)
// entries accordingly, and self-corrects the top row against
// nextUndoDescription on every pass. Deeper rows are historical labels; a
// state that was never observed through the GUI wiring (which refreshes after
// every action) shows honestly as "… N older operation(s) not shown".
class AdminPanel : public QWidget {
    Q_OBJECT

public:
    explicit AdminPanel(controller::ClinicManager* manager, QWidget* parent = nullptr);

public slots:
    // Re-syncs the history list with the manager's stack, re-feeds
    // undoDepthChanged and syncs the Undo button's enabled state. Runs after
    // every action (MainWindow's refreshPanels) and after undoLast.
    void refresh();

    // Reverses the most recent administrative operation: manager.undo,
    // then refresh + dataChanged so every panel re-reads.
    // Connected to MainWindow's Edit>Undo action and this panel's Undo
    // button - the single undo path in the app.
    void undoLast();

signals:
    void statusMessage(const QString& message);
    // Fired on every refresh with the current depth - MainWindow feeds the
    // status-bar depth label and the Edit>Undo enabled state from this.
    void undoDepthChanged(int depth);
    // Emitted after a successful undo: record data changed.
    void dataChanged();
    // Maintenance requests. MainWindow owns the confirmation dialogs and the
    // manager mutations, so clearing/loading has exactly one implementation.
    void clearAllRequested();
    void loadSampleRequested();

private:
    void buildUi();
    void syncHistoryList();

    controller::ClinicManager* manager_;
    QListWidget* historyList_ = nullptr;
    QLabel* nextUndoLabel_ = nullptr;
    QPushButton* undoButton_ = nullptr;
    // Newest-first descriptions accumulated in the panel (class comment).
    QStringList historyEntries_;
};

}  // namespace scapms::gui
