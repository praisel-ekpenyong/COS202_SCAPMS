#pragma once
// Undo action classes for reversing deletions, edits, and cancellations.
#include "model/AppointmentStatus.h"
#include "model/PatientCategory.h"

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {
class Appointment;
class ClinicDatabase;
class PatientRecord;
}  // namespace scapms::model

namespace scapms::controller {

class ClinicManager;

// Base class for undoable operations.
class UndoableAction {
public:
    UndoableAction() = default;
    virtual ~UndoableAction() = default;

    UndoableAction(const UndoableAction&) = delete;
    UndoableAction& operator=(const UndoableAction&) = delete;

    // Undo operation
    virtual void undo(ClinicManager& manager) = 0;

    // Action description
    virtual std::string describe() const = 0;
};

// Snapshot of patient record fields.
struct PatientSnapshot {
    std::string id;
    std::string name;
    int age = 0;
    std::string contact;
    model::PatientCategory category = model::PatientCategory::General;
    std::string nextVisitDate;  // for Follow-Up patients, empty otherwise

    // Captures snapshot from record.
    static PatientSnapshot of(const model::PatientRecord& record);

    // Reconstructs record from snapshot.
    std::unique_ptr<model::PatientRecord> rebuild() const;
};

// Snapshot of appointment record fields.
struct AppointmentSnapshot {
    std::string id;
    std::string patientId;
    std::string date;
    std::string time;
    model::AppointmentStatus status = model::AppointmentStatus::Pending;
    std::string notes;

    static AppointmentSnapshot of(const model::Appointment& appointment);

    // Rebuilds appointment from snapshot.
    std::unique_ptr<model::Appointment> rebuild() const;
};

// Restores deleted patient, appointments, and queue state.
class DeletePatientAction final : public UndoableAction {
public:
    // Captures patient data and associated appointments before deletion.
    static std::unique_ptr<DeletePatientAction> capture(
        const model::ClinicDatabase& database, const std::string& patientId);

    void undo(ClinicManager& manager) override;
    std::string describe() const override;

private:
    DeletePatientAction() = default;

    PatientSnapshot patient_;
    std::vector<AppointmentSnapshot> appointments_;
    std::string queueArrival_;  // arrival time if patient was in queue
};

// Undo action for appointment removal.
class RemoveAppointmentAction final : public UndoableAction {
public:
    // Captures appointment before removal.
    static std::unique_ptr<RemoveAppointmentAction> capture(
        const model::ClinicDatabase& database, const std::string& appointmentId);

    void undo(ClinicManager& manager) override;
    std::string describe() const override;

private:
    RemoveAppointmentAction() = default;

    AppointmentSnapshot appointment_;
};

// Undo action for patient edit: restores previous field values.
class EditPatientAction final : public UndoableAction {
public:
    // Takes snapshots of the record before and after editing.
    EditPatientAction(PatientSnapshot before, PatientSnapshot after);

    void undo(ClinicManager& manager) override;
    std::string describe() const override;

private:
    PatientSnapshot before_;
    PatientSnapshot after_;
};

}  // namespace scapms::controller
