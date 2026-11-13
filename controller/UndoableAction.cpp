#include "controller/UndoableAction.h"

#include "controller/ClinicManager.h"

#include "model/Appointment.h"
#include "model/ClinicDatabase.h"
#include "model/FollowUpPatient.h"
#include "model/PatientRecord.h"

#include "utils/Exceptions.h"

#include <utility>

namespace scapms::controller {

using model::Appointment;
using model::PatientCategory;
using model::PatientRecord;

PatientSnapshot PatientSnapshot::of(const PatientRecord& record) {
    PatientSnapshot snapshot;
    snapshot.id = record.id();
    snapshot.name = record.name();
    snapshot.age = record.age();
    snapshot.contact = record.contact();
    snapshot.category = record.category();
    if (snapshot.category == PatientCategory::FollowUp) {
        if (const auto* followUp = dynamic_cast<const model::FollowUpPatient*>(&record)) {
            snapshot.nextVisitDate = followUp->nextVisitDate();
        }
    }
    return snapshot;
}

std::unique_ptr<PatientRecord> PatientSnapshot::rebuild() const {
    return ClinicManager::makePatientRecord(id, name, age, contact, category,
                                            nextVisitDate);
}

AppointmentSnapshot AppointmentSnapshot::of(const Appointment& appointment) {
    AppointmentSnapshot snapshot;
    snapshot.id = appointment.id();
    snapshot.patientId = appointment.patientId();
    snapshot.date = appointment.date();
    snapshot.time = appointment.time();
    snapshot.status = appointment.status();
    snapshot.notes = appointment.notes();
    return snapshot;
}

std::unique_ptr<Appointment> AppointmentSnapshot::rebuild() const {
    return std::make_unique<Appointment>(id, patientId, date, time, status, notes);
}

std::unique_ptr<DeletePatientAction> DeletePatientAction::capture(
    const model::ClinicDatabase& database, const std::string& patientId) {
    const PatientRecord* record = database.findPatient(patientId);
    if (record == nullptr) {
        throw utils::ValidationError("no patient with ID " + patientId);
    }
    auto action = std::unique_ptr<DeletePatientAction>(new DeletePatientAction());
    action->patient_ = PatientSnapshot::of(*record);
    for (const Appointment* appointment : database.appointmentsFor(patientId)) {
        action->appointments_.push_back(AppointmentSnapshot::of(*appointment));
    }
    action->queueArrival_ = database.queueArrivalTime(patientId);
    return action;
}

void DeletePatientAction::undo(ClinicManager& manager) {
    // Restore patient record first, then appointments
    manager.addPatientRecord(patient_.rebuild());
    for (const AppointmentSnapshot& appointment : appointments_) {
        manager.addAppointmentRecord(appointment.rebuild());
    }
    if (!queueArrival_.empty()) {
        manager.addToQueue(patient_.id, queueArrival_);
    }
}

std::string DeletePatientAction::describe() const {
    return "Delete patient " + patient_.id + " (" + patient_.name + ")";
}

std::unique_ptr<RemoveAppointmentAction> RemoveAppointmentAction::capture(
    const model::ClinicDatabase& database, const std::string& appointmentId) {
    const Appointment* appointment = database.findAppointment(appointmentId);
    if (appointment == nullptr) {
        throw utils::ValidationError("no appointment with ID " + appointmentId);
    }
    auto action = std::unique_ptr<RemoveAppointmentAction>(new RemoveAppointmentAction());
    action->appointment_ = AppointmentSnapshot::of(*appointment);
    return action;
}

void RemoveAppointmentAction::undo(ClinicManager& manager) {
    manager.addAppointmentRecord(appointment_.rebuild());
}

std::string RemoveAppointmentAction::describe() const {
    return "Remove appointment " + appointment_.id;
}

EditPatientAction::EditPatientAction(PatientSnapshot before, PatientSnapshot after)
    : before_(std::move(before)), after_(std::move(after)) {}

void EditPatientAction::undo(ClinicManager& manager) {
    // Restore previous field values
    PatientRecord& record = manager.getPatient(before_.id);
    record.setName(before_.name);
    record.setAge(before_.age);
    record.setContact(before_.contact);
    if (record.category() == PatientCategory::FollowUp) {
        dynamic_cast<model::FollowUpPatient&>(record).setNextVisitDate(before_.nextVisitDate);
    }
}

std::string EditPatientAction::describe() const {
    return "Edit patient " + after_.id + " (" + after_.name + ")";
}

}  // namespace scapms::controller
