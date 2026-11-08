#pragma once
// Interface for patients that can schedule appointments.
#include <string>

namespace scapms::model {

class Appointable {
public:
    virtual ~Appointable() = default;

    // Associates an appointment ID with this patient.
    virtual void scheduleAppointment(const std::string& appointmentId) = 0;

    // Clears the active appointment.
    virtual void cancelAppointment() = 0;

    // True while an appointment is held.
    virtual bool isScheduled() const = 0;

    // ID of the held appointment; empty string when none.
    virtual const std::string& currentAppointmentId() const = 0;
};

}  // namespace scapms::model
