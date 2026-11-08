#pragma once
// Appointment model: tracks patient, date/time, status and notes.
#include "model/AppointmentStatus.h"

#include <string>

namespace scapms::model {

class Appointment {
public:
    // Creates appointment with field validation.
    Appointment(std::string id, std::string patientId, std::string date,
                std::string time,
                AppointmentStatus status = AppointmentStatus::Pending,
                std::string notes = "");

    const std::string& id() const;
    const std::string& patientId() const;
    const std::string& date() const;  // "YYYY-MM-DD"
    const std::string& time() const;  // "HH:MM"
    AppointmentStatus status() const;
    const std::string& notes() const;

    void setStatus(AppointmentStatus status);

    // Setters with validation.
    void setDate(const std::string& date);
    void setTime(const std::string& time);
    void setNotes(const std::string& notes);

    // Checks if pending visit is before the given date (YYYY-MM-DD).
    bool isOverdue(const std::string& today) const;

    // Summary string for display.
    std::string describe() const;

private:
    std::string id_;
    std::string patientId_;
    std::string date_;
    std::string time_;
    AppointmentStatus status_;
    std::string notes_;
};

}  // namespace scapms::model
