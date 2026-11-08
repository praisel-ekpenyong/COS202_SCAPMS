#pragma once
// The three patient categories: the kind of patient,
// which determines scheduling behaviour.
#include "utils/Exceptions.h"

#include <string>

namespace scapms::model {

enum class PatientCategory { General, Emergency, FollowUp };

// Category spelling shown in the GUI and stored in patients.csv.
inline std::string categoryName(PatientCategory category) {
    switch (category) {
        case PatientCategory::General:
            return "General";
        case PatientCategory::Emergency:
            return "Emergency";
        case PatientCategory::FollowUp:
            return "Follow-Up";
    }
    return "General";
}

// Parses the persisted/GUI spelling back into a category. Accepts the CSV
// variants "FollowUp"/"Followup" alongside "Follow-Up" so persistence
// can read either; throws scapms::utils::ValidationError on anything else.
inline PatientCategory patientCategoryFromString(const std::string& text) {
    if (text == "General") {
        return PatientCategory::General;
    }
    if (text == "Emergency") {
        return PatientCategory::Emergency;
    }
    if (text == "Follow-Up" || text == "FollowUp" || text == "Followup") {
        return PatientCategory::FollowUp;
    }
    throw scapms::utils::ValidationError("unknown patient category: \"" + text + "\"");
}

}  // namespace scapms::model
