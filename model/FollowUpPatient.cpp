#include "model/FollowUpPatient.h"

#include "utils/IDGenerator.h"
#include "utils/Validators.h"

namespace scapms::model {

FollowUpPatient::FollowUpPatient(std::string id, std::string name, int age,
                                 std::string contact,
                                 const std::string& nextVisitDate)
    : PatientRecord(std::move(id), std::move(name), age, std::move(contact),
                    PatientCategory::FollowUp) {
    setNextVisitDate(nextVisitDate);  // validates unless empty ("not yet set")
}

std::string FollowUpPatient::describe() const {
    std::string text = id() + ": " + name() + ", " + std::to_string(age()) +
                       ", " + categoryName(category());
    if (!nextVisitDate_.empty()) {
        text += ", next visit " + nextVisitDate_;
    }
    return text;
}

int FollowUpPatient::priority() const {
    return 2;
}

const std::string& FollowUpPatient::nextVisitDate() const {
    return nextVisitDate_;
}

void FollowUpPatient::setNextVisitDate(const std::string& nextVisitDate) {
    if (!nextVisitDate.empty()) {
        scapms::utils::validateDate(nextVisitDate);
    }
    nextVisitDate_ = nextVisitDate;
}

void FollowUpPatient::scheduleAppointment(const std::string& appointmentId) {
    if (!scapms::utils::isAppointmentId(appointmentId)) {
        throw scapms::utils::ValidationError(
            "appointment ID must have format A-0001, got \"" + appointmentId + "\"");
    }
    scheduledAppointmentId_ = appointmentId;
}

void FollowUpPatient::cancelAppointment() {
    scheduledAppointmentId_.clear();
}

bool FollowUpPatient::isScheduled() const {
    return !scheduledAppointmentId_.empty();
}

const std::string& FollowUpPatient::currentAppointmentId() const {
    return scheduledAppointmentId_;
}

}  // namespace scapms::model
