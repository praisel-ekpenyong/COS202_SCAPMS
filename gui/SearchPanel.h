#pragma once

#include <vector>
#include <QWidget>

#include "controller/PatientSorter.h"
#include "controller/SearchEngine.h"

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::model {
class PatientRecord;
}

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace scapms::gui {

// Search & Sort panel: handles algorithm selection, execution, and result display.
class SearchPanel : public QWidget {
    Q_OBJECT

public:
    // Constructs panel with reference to clinic manager.
    explicit SearchPanel(controller::ClinicManager& manager,
                         QWidget* parent = nullptr);

signals:
    // Human-readable one-liner for the status bar's "last action".
    void actionPerformed(const QString& action);

    // The technique that produced the current result - feeds the status
    // bar's "algorithm used" field.
    void algorithmUsed(const QString& algorithm);

public slots:
    // Re-runs the last sort and search (or clears the tables if none has
    // run) so a mutation elsewhere (dataChanged / File->New / Undo) can
    // never leave stale rows on screen.
    void refresh();

private slots:
    void onSortClicked();
    void onSearchClicked();

private:
    // search target: ID, Name, or the mandatory Category filter.
    enum class Field { Id, Name, Category };

    // Everything one search run needs, captured so refresh can re-run the
    // last search with identical parameters without re-reading the combos.
    struct SearchParams {
        Field field = Field::Id;
        QString query;
        bool prefix = false;      // NameMatch::Prefix instead of Exact
        bool recursive = false;   // explicit Recursive technique (name only)
        int categoryIndex = 0;    // index into the category combo
    };

    void buildUi();
    void updateSearchControlVisibility();
    void runSort(controller::SortKey key, controller::SortAlgorithm algorithm,
                 bool report);
    void runSearch(const SearchParams& params, bool report);
    Field currentField() const;
    SearchParams currentSearchParams() const;
    model::PatientCategory currentCategory() const;
    void fillPatientRows(QTableWidget* table,
                         const std::vector<const model::PatientRecord*>& patients);

    controller::ClinicManager& manager_;
    controller::SearchEngine engine_;

    // Last run's parameters, for silent re-runs from refresh.
    bool hasSort_ = false;
    controller::SortKey lastSortKey_ = controller::SortKey::Name;
    controller::SortAlgorithm lastSortAlgorithm_ =
        controller::SortAlgorithm::SelectionSort;
    bool hasSearch_ = false;
    SearchParams lastSearch_;

    QComboBox* keyBox_ = nullptr;
    QComboBox* algorithmBox_ = nullptr;
    QPushButton* sortButton_ = nullptr;
    QTableWidget* resultTable_ = nullptr;
    QLabel* resultCount_ = nullptr;

    QLineEdit* searchEdit_ = nullptr;
    QComboBox* searchFieldBox_ = nullptr;
    QComboBox* searchModeBox_ = nullptr;
    QComboBox* searchTechniqueBox_ = nullptr;
    QComboBox* searchCategoryBox_ = nullptr;
    QPushButton* searchButton_ = nullptr;
    QTableWidget* searchTable_ = nullptr;
    QLabel* searchCount_ = nullptr;
    // The option labels hide/show together with their combos (dynamic fields).
    QLabel* matchLabel_ = nullptr;
    QLabel* techniqueLabel_ = nullptr;
    QLabel* categoryLabel_ = nullptr;
};

}  // namespace scapms::gui
