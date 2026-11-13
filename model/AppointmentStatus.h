#pragma once
// Appointment statuses: pending, completed, or cancelled.
// appointments.csv stores these spellings in lowercase.
#include "utils/Exceptions.h"

#include <string>

namespace scapms::model {

enum class AppointmentStatus { Pending, Completed, Cancelled };

// CSV/GUI spelling of a status.
inline std::string appointmentStatusName(AppointmentStatus status) {
    switch (status) {
        case AppointmentStatus::Pending:
            return "pending";
        case AppointmentStatus::Completed:
            return "completed";
        case AppointmentStatus::Cancelled:
            return "cancelled";
    }
    return "pending";
}

// Parses string into AppointmentStatus
inline AppointmentStatus appointmentStatusFromString(const std::string& text) {
    if (text == "pending") {
        return AppointmentStatus::Pending;
    }
    if (text == "completed") {
        return AppointmentStatus::Completed;
    }
    if (text == "cancelled") {
        return AppointmentStatus::Cancelled;
    }
    throw scapms::utils::ValidationError("unknown appointment status: \"" + text + "\"");
}

}  // namespace scapms::model
