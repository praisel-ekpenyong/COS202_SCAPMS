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
    void actionPerformed(const QString& action);
    void algorithmUsed(const QString& algorithm);

public slots:
    void refresh();

private slots:
    void onSortClicked();
    void onSearchClicked();

private:
    // Fields available for searching
    enum class Field { Id, Name, Category };

    // Parameters saved from last search
    struct SearchParams {
        Field field = Field::Id;
        QString query;
        bool prefix = false;
        bool recursive = false;
        int categoryIndex = 0;
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

    // Parameters from the last sort and search runs
    bool hasSort_ = false;
    controller::SortKey lastSortKey_ = controller::SortKey::Name;
    controller::SortAlgorithm lastSortAlgorithm_ =
        controller::SortAlgorithm::SelectionSort;
    bool hasSearch_ = false;
    SearchParams lastSearch_;

    QComboBox* keyBox_ = nullptr;
    QComboBox* algorithmBox_ = nullptr;
    QLabel* algorithmReasonLabel_ = nullptr;
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
    QLabel* matchLabel_ = nullptr;
    QLabel* techniqueLabel_ = nullptr;
    QLabel* categoryLabel_ = nullptr;
};

}  // namespace scapms::gui
