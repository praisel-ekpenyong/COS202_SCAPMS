#include "model/Appointment.h"

#include "utils/IDGenerator.h"
#include "utils/Validators.h"

namespace scapms::model {

Appointment::Appointment(std::string id, std::string patientId,
                         std::string date, std::string time,
                         AppointmentStatus status, std::string notes)
    : id_(std::move(id)),
      patientId_(std::move(patientId)),
      date_(std::move(date)),
      time_(std::move(time)),
      status_(status),
      notes_(std::move(notes)) {
    if (!scapms::utils::isAppointmentId(id_)) {
        throw scapms::utils::ValidationError(
            "appointment ID must have format A-0001, got \"" + id_ + "\"");
    }
    if (!scapms::utils::isPatientId(patientId_)) {
        throw scapms::utils::ValidationError(
            "patient ID must have format P-0001, got \"" + patientId_ + "\"");
    }
    scapms::utils::validateDate(date_);
    scapms::utils::validateTime(time_);
}

const std::string& Appointment::id() const {
    return id_;
}

const std::string& Appointment::patientId() const {
    return patientId_;
}

const std::string& Appointment::date() const {
    return date_;
}

const std::string& Appointment::time() const {
    return time_;
}

AppointmentStatus Appointment::status() const {
    return status_;
}

const std::string& Appointment::notes() const {
    return notes_;
}

void Appointment::setStatus(AppointmentStatus status) {
    status_ = status;
}

void Appointment::setDate(const std::string& date) {
    scapms::utils::validateDate(date);
    date_ = date;
}

void Appointment::setTime(const std::string& time) {
    scapms::utils::validateTime(time);
    time_ = time;
}

void Appointment::setNotes(const std::string& notes) {
    notes_ = notes;
}

bool Appointment::isOverdue(const std::string& today) const {
    return status_ == AppointmentStatus::Pending && date_ < today;
}

std::string Appointment::describe() const {
    std::string text = id_ + " for " + patientId_ + " on " + date_ + " " +
                       time_ + " [" + appointmentStatusName(status_) + "]";
    if (!notes_.empty()) {
        text += " (" + notes_ + ")";
    }
    return text;
}

}  // namespace scapms::model
