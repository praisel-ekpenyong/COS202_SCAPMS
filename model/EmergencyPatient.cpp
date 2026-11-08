#include "model/EmergencyPatient.h"

#include "utils/IDGenerator.h"

namespace scapms::model {

EmergencyPatient::EmergencyPatient(std::string id, std::string name, int age,
                                   std::string contact)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact),
                    PatientCategory::Emergency) {}

std::string EmergencyPatient::describe() const {
    return id() + ": " + name() + ", " + std::to_string(age()) + ", " +
           categoryName(category()) + " (served first)";
}

int EmergencyPatient::priority() const {
    return 3;
}

void EmergencyPatient::scheduleAppointment(const std::string& appointmentId) {
    if (!scapms::utils::isAppointmentId(appointmentId)) {
        throw scapms::utils::ValidationError(
            "appointment ID must have format A-0001, got \"" + appointmentId + "\"");
    }
    scheduledAppointmentId_ = appointmentId;
}

void EmergencyPatient::cancelAppointment() {
    scheduledAppointmentId_.clear();
}

bool EmergencyPatient::isScheduled() const {
    return !scheduledAppointmentId_.empty();
}

const std::string& EmergencyPatient::currentAppointmentId() const {
    return scheduledAppointmentId_;
}

}  // namespace scapms::model
