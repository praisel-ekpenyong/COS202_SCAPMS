#include "model/GeneralPatient.h"

#include "utils/IDGenerator.h"

namespace scapms::model {

GeneralPatient::GeneralPatient(std::string id, std::string name, int age,
                               std::string contact)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact),
                    PatientCategory::General) {}

std::string GeneralPatient::describe() const {
    return id() + ": " + name() + ", " + std::to_string(age()) + ", " +
           categoryName(category());
}

int GeneralPatient::priority() const {
    return 1;
}

void GeneralPatient::scheduleAppointment(const std::string& appointmentId) {
    if (!scapms::utils::isAppointmentId(appointmentId)) {
        throw scapms::utils::ValidationError(
            "appointment ID must have format A-0001, got \"" + appointmentId + "\"");
    }
    scheduledAppointmentId_ = appointmentId;
}

void GeneralPatient::cancelAppointment() {
    scheduledAppointmentId_.clear();
}

bool GeneralPatient::isScheduled() const {
    return !scheduledAppointmentId_.empty();
}

const std::string& GeneralPatient::currentAppointmentId() const {
    return scheduledAppointmentId_;
}

}  // namespace scapms::model
