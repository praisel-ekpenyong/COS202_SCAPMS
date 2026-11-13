#pragma once
// In-memory storage for patients, appointments, waiting queue, and recent cache.
#include "model/Appointment.h"
#include "model/PatientRecord.h"

#include <cstddef>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace scapms::model {

class ClinicDatabase {
public:
    // Cache capacity for recently viewed patients.
    static constexpr std::size_t kRecentCacheSize = 10;

    // Patients

    // Stores a patient record.
    void addPatient(std::unique_ptr<PatientRecord> patient);

    // Removes patient from database.
    std::unique_ptr<PatientRecord> takePatient(const std::string& id);

    // Appointments

    // Stores appointment record.
    void addAppointment(std::unique_ptr<Appointment> appointment);

    // Removes appointment by ID.
    std::unique_ptr<Appointment> takeAppointment(const std::string& id);

    // Removes all appointments associated with a patient ID.
    std::vector<std::unique_ptr<Appointment>> takeAppointmentsOf(const std::string& patientId);

    // Queries

    bool hasPatient(const std::string& id) const;
    PatientRecord* findPatient(const std::string& id);
    const PatientRecord* findPatient(const std::string& id) const;
    Appointment* findAppointment(const std::string& id);
    const Appointment* findAppointment(const std::string& id) const;
    std::vector<Appointment*> appointmentsFor(const std::string& patientId);
    std::vector<const Appointment*> appointmentsFor(const std::string& patientId) const;
    const std::vector<std::unique_ptr<PatientRecord>>& patients() const;
    const std::vector<std::unique_ptr<Appointment>>& appointments() const;

    // Waiting queue

    // Enqueues patient ID with arrival timestamp.
    void enqueue(const std::string& patientId, const std::string& arrivalTime);

    // Dequeues front patient from waiting queue.
    bool tryDequeueFront();

    // Removes patient from waiting queue if present.
    bool removeFromQueue(const std::string& patientId);

    bool isQueued(const std::string& id) const;
    std::string queueArrivalTime(const std::string& patientId) const;
    const std::queue<std::string>& waitingQueue() const;

    // Recent cache

    // Moves patient to front of recent cache.
    void touchRecent(PatientRecord* patient);

    // Clears deleted patient from recent cache.
    void forgetRecent(const std::string& patientId);

    // Returns cached patient at index.
    const PatientRecord* recentAt(std::size_t index) const;
    std::size_t recentCount() const;

    // Clear data

    // Clears all stored records and queues.
    void clear();

private:
    std::vector<std::unique_ptr<PatientRecord>> patients_;
    std::vector<std::unique_ptr<Appointment>> appointments_;
    std::queue<std::string> waitingQueue_;
    // Stores arrival time for each queued patient.
    std::map<std::string, std::string> queueArrival_;
    // Recent patients array
    PatientRecord* recentPatients_[kRecentCacheSize] = {};

};

}  // namespace scapms::model
