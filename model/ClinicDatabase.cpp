#include "model/ClinicDatabase.h"

namespace scapms::model {

void ClinicDatabase::addPatient(std::unique_ptr<PatientRecord> patient) {
    patients_.push_back(std::move(patient));
}

std::unique_ptr<PatientRecord> ClinicDatabase::takePatient(const std::string& id) {
    for (std::size_t i = 0; i < patients_.size(); ++i) {
        if (patients_[i]->id() == id) {
            std::unique_ptr<PatientRecord> taken = std::move(patients_[i]);
            patients_.erase(patients_.begin() + static_cast<std::ptrdiff_t>(i));
            return taken;
        }
    }
    return nullptr;
}

void ClinicDatabase::addAppointment(std::unique_ptr<Appointment> appointment) {
    appointments_.push_back(std::move(appointment));
}

std::unique_ptr<Appointment> ClinicDatabase::takeAppointment(const std::string& id) {
    for (std::size_t i = 0; i < appointments_.size(); ++i) {
        if (appointments_[i]->id() == id) {
            std::unique_ptr<Appointment> taken = std::move(appointments_[i]);
            appointments_.erase(appointments_.begin() + static_cast<std::ptrdiff_t>(i));
            return taken;
        }
    }
    return nullptr;
}

std::vector<std::unique_ptr<Appointment>> ClinicDatabase::takeAppointmentsOf(
    const std::string& patientId) {
    std::vector<std::unique_ptr<Appointment>> taken;
    for (std::size_t i = 0; i < appointments_.size();) {
        if (appointments_[i]->patientId() == patientId) {
            taken.push_back(std::move(appointments_[i]));
            appointments_.erase(appointments_.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }
    return taken;
}

bool ClinicDatabase::hasPatient(const std::string& id) const {
    return findPatient(id) != nullptr;
}

PatientRecord* ClinicDatabase::findPatient(const std::string& id) {
    for (const auto& patient : patients_) {
        if (patient->id() == id) {
            return patient.get();
        }
    }
    return nullptr;
}

const PatientRecord* ClinicDatabase::findPatient(const std::string& id) const {
    for (const auto& patient : patients_) {
        if (patient->id() == id) {
            return patient.get();
        }
    }
    return nullptr;
}

Appointment* ClinicDatabase::findAppointment(const std::string& id) {
    for (const auto& appointment : appointments_) {
        if (appointment->id() == id) {
            return appointment.get();
        }
    }
    return nullptr;
}

const Appointment* ClinicDatabase::findAppointment(const std::string& id) const {
    for (const auto& appointment : appointments_) {
        if (appointment->id() == id) {
            return appointment.get();
        }
    }
    return nullptr;
}

std::vector<Appointment*> ClinicDatabase::appointmentsFor(const std::string& patientId) {
    std::vector<Appointment*> found;
    for (const auto& appointment : appointments_) {
        if (appointment->patientId() == patientId) {
            found.push_back(appointment.get());
        }
    }
    return found;
}

std::vector<const Appointment*> ClinicDatabase::appointmentsFor(
    const std::string& patientId) const {
    std::vector<const Appointment*> found;
    for (const auto& appointment : appointments_) {
        if (appointment->patientId() == patientId) {
            found.push_back(appointment.get());
        }
    }
    return found;
}

const std::vector<std::unique_ptr<PatientRecord>>& ClinicDatabase::patients() const {
    return patients_;
}

const std::vector<std::unique_ptr<Appointment>>& ClinicDatabase::appointments() const {
    return appointments_;
}

void ClinicDatabase::enqueue(const std::string& patientId, const std::string& arrivalTime) {
    waitingQueue_.push(patientId);
    queueArrival_[patientId] = arrivalTime;
}

bool ClinicDatabase::tryDequeueFront() {
    if (waitingQueue_.empty()) {
        return false;
    }
    queueArrival_.erase(waitingQueue_.front());
    waitingQueue_.pop();
    return true;
}

bool ClinicDatabase::removeFromQueue(const std::string& patientId) {
    if (!isQueued(patientId)) {
        return false;
    }
    // Rebuild queue without the removed patient ID
    std::size_t remaining = waitingQueue_.size();
    for (std::size_t i = 0; i < remaining; ++i) {
        const std::string id = waitingQueue_.front();
        const std::string arrival = queueArrivalTime(id);
        tryDequeueFront();
        if (id != patientId) {
            enqueue(id, arrival);
        }
    }
    return true;
}

bool ClinicDatabase::isQueued(const std::string& id) const {
    for (std::queue<std::string> copy = waitingQueue_; !copy.empty(); copy.pop()) {
        if (copy.front() == id) {
            return true;
        }
    }
    return false;
}

std::string ClinicDatabase::queueArrivalTime(const std::string& patientId) const {
    const auto it = queueArrival_.find(patientId);
    return it == queueArrival_.end() ? std::string{} : it->second;
}

const std::queue<std::string>& ClinicDatabase::waitingQueue() const {
    return waitingQueue_;
}

void ClinicDatabase::touchRecent(PatientRecord* patient) {
    if (patient == nullptr) {
        return;
    }
    const std::size_t count = recentCount();
    std::size_t existing = 0;
    bool found = false;
    for (std::size_t i = 0; i < count; ++i) {
        if (recentPatients_[i] == patient) {
            existing = i;
            found = true;
            break;
        }
    }
    if (found && existing == 0) {
        return;
    }
    // Shift elements right and insert patient at index 0
    const std::size_t last = found ? existing
                           : (count == kRecentCacheSize ? count - 1 : count);
    for (std::size_t i = last; i > 0; --i) {
        recentPatients_[i] = recentPatients_[i - 1];
    }
    recentPatients_[0] = patient;
}

void ClinicDatabase::forgetRecent(const std::string& patientId) {
    for (std::size_t i = 0; i < kRecentCacheSize; ++i) {
        if (recentPatients_[i] != nullptr && recentPatients_[i]->id() == patientId) {
            for (std::size_t j = i; j + 1 < kRecentCacheSize; ++j) {
                recentPatients_[j] = recentPatients_[j + 1];
            }
            recentPatients_[kRecentCacheSize - 1] = nullptr;
            return;
        }
    }
}

const PatientRecord* ClinicDatabase::recentAt(std::size_t index) const {
    if (index >= kRecentCacheSize) {
        return nullptr;
    }
    return recentPatients_[index];
}

std::size_t ClinicDatabase::recentCount() const {
    std::size_t count = 0;
    while (count < kRecentCacheSize && recentPatients_[count] != nullptr) {
        ++count;
    }
    return count;
}

void ClinicDatabase::clear() {
    patients_.clear();
    appointments_.clear();
    waitingQueue_ = std::queue<std::string>{};
    queueArrival_.clear();
    for (std::size_t i = 0; i < kRecentCacheSize; ++i) {
        recentPatients_[i] = nullptr;
    }
}

}  // namespace scapms::model
