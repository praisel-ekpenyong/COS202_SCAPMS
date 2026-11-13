#include "gui/SearchPanel.h"

#include <QBrush>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "controller/ClinicManager.h"
#include "controller/PatientSorter.h"
#include "controller/SearchEngine.h"
#include "gui/Theme.h"
#include "model/ClinicDatabase.h"
#include "model/PatientCategory.h"
#include "model/PatientRecord.h"

namespace scapms::gui {

namespace {

QTableWidgetItem* readOnlyItem(const QString& text) {
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

}  // namespace

SearchPanel::SearchPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager), engine_(manager) {
    buildUi();
    updateSearchControlVisibility();
}

void SearchPanel::buildUi() {
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel(tr("Search & Sort"), this);
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));
    layout->addWidget(title);

    // Sort section
    auto* sortGroup = new QGroupBox(tr("Sort patients"), this);
    auto* sortLayout = new QGridLayout(sortGroup);

    sortLayout->addWidget(new QLabel(tr("Sort by:"), sortGroup), 0, 0);
    keyBox_ = new QComboBox(sortGroup);
    for (const auto key : {controller::SortKey::Name, controller::SortKey::Id,
                           controller::SortKey::Age,
                           controller::SortKey::AppointmentDate})
        keyBox_->addItem(QString::fromStdString(controller::sortKeyName(key)),
                         static_cast<int>(key));
    keyBox_->setToolTip(tr("Select attribute to sort by"));
    sortLayout->addWidget(keyBox_, 0, 1);

    sortLayout->addWidget(new QLabel(tr("Algorithm:"), sortGroup), 0, 2);
    algorithmBox_ = new QComboBox(sortGroup);
    for (const auto algorithm : {controller::SortAlgorithm::SelectionSort,
                                 controller::SortAlgorithm::MergeSort})
        algorithmBox_->addItem(
            QString::fromStdString(controller::sortAlgorithmName(algorithm)),
            static_cast<int>(algorithm));
    sortLayout->addWidget(algorithmBox_, 0, 3);

    sortButton_ = new QPushButton(tr("Sort"), sortGroup);
    sortButton_->setToolTip(tr("Sort patient records"));
    connect(sortButton_, &QPushButton::clicked, this, &SearchPanel::onSortClicked);
    sortLayout->addWidget(sortButton_, 0, 4);

    algorithmReasonLabel_ = new QLabel(sortGroup);
    algorithmReasonLabel_->setStyleSheet(QStringLiteral("color: #64748b; font-size: 11px; font-style: italic;"));
    algorithmReasonLabel_->setWordWrap(true);
    sortLayout->addWidget(algorithmReasonLabel_, 1, 0, 1, 5);

    auto updateAlgorithmReason = [this]() {
        const auto algo = static_cast<controller::SortAlgorithm>(
            algorithmBox_->currentData().toInt());
        const QString reason = QString::fromStdString(controller::sortAlgorithmDescription(algo));
        algorithmBox_->setToolTip(reason);
        algorithmReasonLabel_->setText(reason);
    };
    connect(algorithmBox_, &QComboBox::currentIndexChanged, this, updateAlgorithmReason);
    updateAlgorithmReason();

    sortLayout->setColumnStretch(1, 1);
    layout->addWidget(sortGroup);

    resultCount_ = new QLabel(tr("No sort run yet."), this);
    layout->addWidget(resultCount_);

    resultTable_ = new QTableWidget(this);
    resultTable_->setColumnCount(5);
    resultTable_->setHorizontalHeaderLabels({tr("ID"), tr("Name"), tr("Age"),
                                             tr("Category"),
                                             tr("Queue priority")});
    resultTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    resultTable_->horizontalHeader()->setStretchLastSection(true);
    resultTable_->verticalHeader()->setVisible(false);
    layout->addWidget(resultTable_, 1);

    // Search section
    auto* searchGroup = new QGroupBox(tr("Search patients"), this);
    auto* searchLayout = new QGridLayout(searchGroup);

    searchLayout->addWidget(new QLabel(tr("Search for:"), searchGroup), 0, 0);
    searchEdit_ = new QLineEdit(searchGroup);
    searchEdit_->setToolTip(tr("Enter search query"));
    searchEdit_->setPlaceholderText(tr("Search query..."));
    connect(searchEdit_, &QLineEdit::returnPressed,
            this, &SearchPanel::onSearchClicked);
    connect(searchEdit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (text.trimmed().isEmpty() && currentField() != Field::Category) {
            hasSearch_ = false;
            searchTable_->setRowCount(0);
            searchCount_->setText(tr("No search run yet."));
        }
    });
    searchLayout->addWidget(searchEdit_, 0, 1);

    searchFieldBox_ = new QComboBox(searchGroup);
    searchFieldBox_->addItem(tr("ID"), static_cast<int>(Field::Id));
    searchFieldBox_->addItem(tr("Name"), static_cast<int>(Field::Name));
    searchFieldBox_->addItem(tr("Category"), static_cast<int>(Field::Category));
    connect(searchFieldBox_, &QComboBox::currentIndexChanged, this, [this](int) {
        updateSearchControlVisibility();
    });
    searchLayout->addWidget(searchFieldBox_, 0, 2);

    matchLabel_ = new QLabel(tr("Match:"), searchGroup);
    searchLayout->addWidget(matchLabel_, 1, 0);
    searchModeBox_ = new QComboBox(searchGroup);
    searchModeBox_->addItem(tr("Exact"));
    searchModeBox_->addItem(tr("Starts with"));
    searchLayout->addWidget(searchModeBox_, 1, 1, 1, 2);

    techniqueLabel_ = new QLabel(tr("Technique:"), searchGroup);
    searchLayout->addWidget(techniqueLabel_, 2, 0);
    searchTechniqueBox_ = new QComboBox(searchGroup);
    searchTechniqueBox_->addItem(tr("Auto (by sorted state)"));
    searchTechniqueBox_->addItem(tr("Recursive"));
    searchLayout->addWidget(searchTechniqueBox_, 2, 1, 1, 2);

    categoryLabel_ = new QLabel(tr("Category:"), searchGroup);
    searchLayout->addWidget(categoryLabel_, 3, 0);
    searchCategoryBox_ = new QComboBox(searchGroup);
    for (const auto category : {model::PatientCategory::General,
                                model::PatientCategory::Emergency,
                                model::PatientCategory::FollowUp})
        searchCategoryBox_->addItem(
            QString::fromStdString(model::categoryName(category)),
            static_cast<int>(category));
    searchLayout->addWidget(searchCategoryBox_, 3, 1, 1, 2);

    searchButton_ = new QPushButton(tr("Search"), searchGroup);
    searchButton_->setToolTip(tr("Search patient records"));
    connect(searchButton_, &QPushButton::clicked, this, &SearchPanel::onSearchClicked);
    searchLayout->addWidget(searchButton_, 4, 1);
    searchLayout->setColumnStretch(1, 1);
    layout->addWidget(searchGroup);

    searchCount_ = new QLabel(tr("No search run yet."), this);
    layout->addWidget(searchCount_);

    searchTable_ = new QTableWidget(this);
    searchTable_->setColumnCount(5);
    searchTable_->setHorizontalHeaderLabels({tr("ID"), tr("Name"), tr("Age"),
                                             tr("Category"),
                                             tr("Queue priority")});
    searchTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    searchTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    searchTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    searchTable_->horizontalHeader()->setStretchLastSection(true);
    searchTable_->verticalHeader()->setVisible(false);
    layout->addWidget(searchTable_, 2);
}

void SearchPanel::updateSearchControlVisibility() {
    // Show only the controls relevant to the selected search field
    const Field field = currentField();
    const bool category = field == Field::Category;
    const bool name = field == Field::Name;
    searchEdit_->setVisible(!category);
    categoryLabel_->setVisible(category);
    searchCategoryBox_->setVisible(category);
    matchLabel_->setVisible(name);
    searchModeBox_->setVisible(name);
    techniqueLabel_->setVisible(name);
    searchTechniqueBox_->setVisible(name);
}

SearchPanel::Field SearchPanel::currentField() const {
    return static_cast<Field>(searchFieldBox_->currentData().toInt());
}

model::PatientCategory SearchPanel::currentCategory() const {
    return static_cast<model::PatientCategory>(
        searchCategoryBox_->currentData().toInt());
}

SearchPanel::SearchParams SearchPanel::currentSearchParams() const {
    SearchParams params;
    params.field = currentField();
    params.query = searchEdit_->text().trimmed();
    params.prefix = searchModeBox_->currentIndex() == 1;
    params.recursive = searchTechniqueBox_->currentIndex() == 1;
    params.categoryIndex = searchCategoryBox_->currentIndex();
    return params;
}

void SearchPanel::onSortClicked() {
    runSort(static_cast<controller::SortKey>(keyBox_->currentData().toInt()),
            static_cast<controller::SortAlgorithm>(
                algorithmBox_->currentData().toInt()),
            true);
}

void SearchPanel::onSearchClicked() {
    runSearch(currentSearchParams(), true);
}

void SearchPanel::runSort(controller::SortKey key,
                          controller::SortAlgorithm algorithm, bool report) {
    // Sort patient records
    const controller::SortResult result =
        controller::PatientSorter::sort(manager_, key, algorithm);

    hasSort_ = true;
    lastSortKey_ = key;
    lastSortAlgorithm_ = algorithm;
    fillPatientRows(resultTable_, result.patients);

    // Update search engine with sorted view for binary search
    controller::installSearchView(engine_, result);

    resultCount_->setText(tr("%n patient(s) sorted by %1 (%2)",
                             "", static_cast<int>(result.patients.size()))
                              .arg(QString::fromStdString(controller::sortKeyName(result.key)))
                              .arg(QString::fromStdString(controller::sortAlgorithmName(result.algorithm))));
    if (report) {
        // "algorithm used" status-bar feedback.
        emit algorithmUsed(
            QString::fromStdString(controller::sortAlgorithmName(result.algorithm)));
        emit actionPerformed(tr("Sorted %n record(s) (%1)",
                                "", static_cast<int>(result.patients.size()))
                                 .arg(QString::fromStdString(controller::sortAlgorithmName(result.algorithm))));
    }
}

void SearchPanel::runSearch(const SearchParams& params, bool report) {
    controller::SearchResult result;
    QString what;
    switch (params.field) {
    case Field::Id: {
        result = engine_.findById(params.query.toStdString());
        what = tr("ID \"%1\"").arg(params.query);
        break;
    }
    case Field::Name: {
        const std::string name = params.query.toStdString();
        const auto mode = params.prefix ? controller::NameMatch::Prefix
                                        : controller::NameMatch::Exact;
        result = params.recursive ? engine_.findByNameRecursive(name, mode)
                                  : engine_.findByName(name, mode);
        what = params.prefix ? tr("names starting with \"%1\"").arg(params.query)
                             : tr("name \"%1\"").arg(params.query);
        break;
    }
    case Field::Category: {
        const auto category = static_cast<model::PatientCategory>(
            searchCategoryBox_->itemData(params.categoryIndex).toInt());
        result = engine_.filterByCategory(category);
        what = tr("category %1")
                   .arg(searchCategoryBox_->itemText(params.categoryIndex));
        break;
    }
    }

    hasSearch_ = true;
    lastSearch_ = params;
    fillPatientRows(searchTable_, result.matches);

    // Display the technique actually used (linear, binary, or recursive)
    const QString technique =
        QString::fromStdString(controller::techniqueLabel(result.technique));
    searchCount_->setText(tr("%n match(es) (%1)", "",
                             static_cast<int>(result.matches.size()))
                              .arg(technique));
    emit algorithmUsed(technique);
    if (report)
        emit actionPerformed(tr("Found %n record(s) (%1)", "",
                                static_cast<int>(result.matches.size()))
                                 .arg(technique));
}

void SearchPanel::refresh() {
    // Re-run sort and search to keep tables up to date
    if (hasSort_) {
        runSort(lastSortKey_, lastSortAlgorithm_, false);
    } else {
        resultTable_->setRowCount(0);
        resultCount_->setText(tr("No sort run yet."));
    }
    if (hasSearch_) {
        runSearch(lastSearch_, false);
    } else {
        searchTable_->setRowCount(0);
        searchCount_->setText(tr("No search run yet."));
    }
}

void SearchPanel::fillPatientRows(
    QTableWidget* table,
    const std::vector<const model::PatientRecord*>& patients) {
    table->setRowCount(0);
    for (const model::PatientRecord* record : patients) {
        if (record == nullptr) continue;
        const model::PatientRecord& patient = *record;
        const int row = table->rowCount();
        table->insertRow(row);
        auto* idItem = readOnlyItem(QString::fromStdString(patient.id()));
        table->setItem(row, 0, idItem);
        table->setItem(row, 1, readOnlyItem(QString::fromStdString(patient.name())));
        table->setItem(row, 2, readOnlyItem(QString::number(patient.age())));
        table->setItem(row, 3, readOnlyItem(QString::fromStdString(
                                  model::categoryName(patient.category()))));
        auto* priorityItem = readOnlyItem(QString::number(model::patientPriority(patient)));
        table->setItem(row, 4, priorityItem);

        const QBrush categoryColor(theme::categoryColor(QString::fromStdString(
            model::categoryName(patient.category()))));
        for (int column = 0; column < table->columnCount(); ++column)
            table->item(row, column)->setForeground(categoryColor);
    }
}

}  // namespace scapms::gui
