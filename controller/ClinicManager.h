#pragma once
// Coordinates operations across patients, appointments, waiting queue, and undo.
#include "controller/UndoableAction.h"
#include "model/Appointment.h"
#include "model/ClinicDatabase.h"
#include "model/PatientCategory.h"
#include "model/PatientRecord.h"
#include "utils/IDGenerator.h"

#include <cstddef>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <vector>

namespace scapms::controller {

class ClinicManager {
public:
    ClinicManager() = default;

    // Size of the recent patients cache.
    static constexpr std::size_t kRecentCacheSize = model::ClinicDatabase::kRecentCacheSize;

    // Factory helper to construct PatientRecord instances by category.
    static std::unique_ptr<model::PatientRecord> makePatientRecord(
        const std::string& id, const std::string& name, int age,
        const std::string& contact, model::PatientCategory category,
        const std::string& nextVisitDate = "");

    // Patient methods

    // Validates inputs, assigns P-xxxx ID, and registers patient.
    model::PatientRecord& registerPatient(const std::string& name, int age,
                                          const std::string& contact,
                                          model::PatientCategory category,
                                          const std::string& nextVisitDate = "");

    // Updates patient record fields and records an undo action.
    void editPatient(const std::string& id, const std::string& name, int age,
                     const std::string& contact, const std::string& nextVisitDate = "");

    // Deletes patient and associated appointments (can be undone).
    void deletePatient(const std::string& id);

    // Checks whether a patient exists in database.
    bool hasPatient(const std::string& id) const;

    // Retrieves patient and updates recent cache.
    model::PatientRecord& getPatient(const std::string& id);

    // Inserts pre-built record (used by load and undo).
    model::PatientRecord& addPatientRecord(std::unique_ptr<model::PatientRecord> record);

    // Appointment methods

    // Schedules an appointment with A-xxxx ID and links to patient.
    model::Appointment& scheduleAppointment(const std::string& patientId,
                                            const std::string& date,
                                            const std::string& time,
                                            const std::string& notes = "");

    // Cancels appointment if pending.
    void cancelAppointment(const std::string& appointmentId);

    // Deletes appointment record from database (undoable).
    void removeAppointment(const std::string& appointmentId);

    // Inserts pre-built appointment (used by load and undo).
    model::Appointment& addAppointmentRecord(std::unique_ptr<model::Appointment> appointment);

    // Queue methods

    // Adds patient to waiting queue with arrival time.
    void addToQueue(const std::string& patientId, const std::string& arrivalTime = "");

    // Calls next patient by priority (Emergency first, then arrival time).
    model::PatientRecord* callNext(const std::string& today = "");

    // Undo methods

    // Reverses most recent administrative action (delete, remove, edit).
    void undo();

    // Current size of undo stack.
    std::size_t undoDepth() const;

    // Description of the action that would be undone.
    std::string nextUndoDescription() const;

    // Clears all stored data and resets counters.
    void clear();

    // Tracks data changes so search knows if sorted view needs updating.
    unsigned revision() const;

    // Date and time string helpers.
    static std::string currentDate();
    static std::string currentTime();

    // Current ID generator counter values.
    unsigned patientIdCounter() const;
    unsigned appointmentIdCounter() const;

    // Restores ID counter values when loading files.
    void restoreIdCounters(unsigned highestPatientNumber,
                           unsigned highestAppointmentNumber);

    // Read-only access to database.
    const model::ClinicDatabase& database() const noexcept;

    // Persistence

    // Result summary for file loading.
    struct LoadSummary {
        unsigned patientsLoaded = 0;
        unsigned appointmentsLoaded = 0;
        unsigned patientsSkipped = 0;
        unsigned appointmentsSkipped = 0;
        // Non-fatal issues or skipped rows during load.
        std::vector<std::string> issues;
    };

    // Saves database to patients.csv, appointments.csv, and counters.csv.
    void saveClinic(const std::string& dataDir) const;

    // Loads clinic data from CSV files.
    LoadSummary loadClinic(const std::string& dataDir);

private:
    // Helper to find patient or throw ValidationError.
    model::PatientRecord& requirePatient(const std::string& id);

    model::ClinicDatabase database_;
    utils::IDGenerator idGenerator_;
    unsigned revision_ = 0;

    // Administrative undo stack.
    std::stack<std::unique_ptr<UndoableAction>> undoHistory_;
};

}  // namespace scapms::controller
