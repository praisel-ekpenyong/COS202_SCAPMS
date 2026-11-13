#include "controller/ClinicManager.h"

#include "model/Appointable.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/Exceptions.h"
#include "utils/FileHandler.h"
#include "utils/Validators.h"

#include <ctime>
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <vector>

namespace scapms::controller {
namespace {

using model::Appointment;
using model::AppointmentStatus;
using model::PatientCategory;
using model::PatientRecord;

// Cast PatientRecord to Appointable if supported
model::Appointable* appointableOf(PatientRecord* record) {
    return dynamic_cast<model::Appointable*>(record);
}

// Only Follow-Up patients can have a next-visit date
void ensureNextVisitDateApplicable(PatientCategory category,
                                   const std::string& nextVisitDate) {
    if (category != PatientCategory::FollowUp && !nextVisitDate.empty()) {
        throw utils::ValidationError("a next visit date applies only to Follow-Up patients");
    }
}

}  // namespace

std::unique_ptr<PatientRecord> ClinicManager::makePatientRecord(
    const std::string& id, const std::string& name, int age,
    const std::string& contact, PatientCategory category,
    const std::string& nextVisitDate) {
    ensureNextVisitDateApplicable(category, nextVisitDate);
    switch (category) {
        case PatientCategory::General:
            return std::make_unique<model::GeneralPatient>(id, name, age, contact);
        case PatientCategory::Emergency:
            return std::make_unique<model::EmergencyPatient>(id, name, age, contact);
        case PatientCategory::FollowUp:
            return std::make_unique<model::FollowUpPatient>(id, name, age, contact,
                                                            nextVisitDate);
    }
    throw utils::ValidationError("unknown patient category");
}

PatientRecord& ClinicManager::registerPatient(const std::string& name, int age,
                                              const std::string& contact,
                                              PatientCategory category,
                                              const std::string& nextVisitDate) {
    // Validate inputs before assigning ID
    utils::validateName(name);
    utils::validateAge(age);
    utils::validateContact(contact);
    ensureNextVisitDateApplicable(category, nextVisitDate);
    if (!nextVisitDate.empty()) {
        utils::validateDate(nextVisitDate);
    }
    const std::string id = idGenerator_.nextPatientId();
    PatientRecord& stored =
        addPatientRecord(makePatientRecord(id, name, age, contact, category, nextVisitDate));
    database_.touchRecent(&stored);
    return stored;
}

void ClinicManager::editPatient(const std::string& id, const std::string& name, int age,
                                const std::string& contact,
                                const std::string& nextVisitDate) {
    PatientRecord* record = database_.findPatient(id);
    if (record == nullptr) {
        throw utils::ValidationError("no patient with ID " + id);
    }
    utils::validateName(name);
    utils::validateAge(age);
    utils::validateContact(contact);
    ensureNextVisitDateApplicable(record->category(), nextVisitDate);
    if (!nextVisitDate.empty()) {
        utils::validateDate(nextVisitDate);
    }

    // Save previous state for undo
    const PatientSnapshot before = PatientSnapshot::of(*record);

    record->setName(name);
    record->setAge(age);
    record->setContact(contact);
    if (record->category() == PatientCategory::FollowUp) {
        static_cast<model::FollowUpPatient*>(record)->setNextVisitDate(nextVisitDate);
    }
    database_.touchRecent(record);
    ++revision_;
    undoHistory_.push(std::make_unique<EditPatientAction>(before, PatientSnapshot::of(*record)));
}

void ClinicManager::deletePatient(const std::string& id) {
    if (!database_.hasPatient(id)) {
        throw utils::ValidationError("no patient with ID " + id);
    }
    // Save patient, appointments, and queue state for undo
    auto action = DeletePatientAction::capture(database_, id);
    database_.forgetRecent(id);
    database_.takeAppointmentsOf(id);
    database_.takePatient(id);
    database_.removeFromQueue(id);
    ++revision_;
    undoHistory_.push(std::move(action));
}

bool ClinicManager::hasPatient(const std::string& id) const {
    return database_.hasPatient(id);
}

PatientRecord& ClinicManager::getPatient(const std::string& id) {
    return requirePatient(id);
}

PatientRecord& ClinicManager::requirePatient(const std::string& id) {
    PatientRecord* record = database_.findPatient(id);
    if (record == nullptr) {
        throw utils::ValidationError("no patient with ID " + id);
    }
    database_.touchRecent(record);
    return *record;
}

PatientRecord& ClinicManager::addPatientRecord(std::unique_ptr<PatientRecord> record) {
    if (record == nullptr) {
        throw utils::ValidationError("cannot store a null patient record");
    }
    if (database_.hasPatient(record->id())) {
        throw utils::DuplicateIdError("patient ID " + record->id() + " already exists");
    }
    PatientRecord& stored = *record;
    database_.addPatient(std::move(record));
    ++revision_;
    return stored;
}

Appointment& ClinicManager::scheduleAppointment(const std::string& patientId,
                                                const std::string& date,
                                                const std::string& time,
                                                const std::string& notes) {
    // Validate fields before generating ID
    utils::validateDate(date);
    utils::validateTime(time);
    requirePatient(patientId);
    const std::string id = idGenerator_.nextAppointmentId();
    auto appointment = std::make_unique<Appointment>(id, patientId, date, time,
                                                     AppointmentStatus::Pending, notes);
    return addAppointmentRecord(std::move(appointment));
}

Appointment& ClinicManager::addAppointmentRecord(std::unique_ptr<Appointment> appointment) {
    if (appointment == nullptr) {
        throw utils::ValidationError("cannot store a null appointment");
    }
    if (database_.findAppointment(appointment->id()) != nullptr) {
        throw utils::DuplicateIdError("appointment ID " + appointment->id() + " already exists");
    }
    PatientRecord* patient = database_.findPatient(appointment->patientId());
    if (patient == nullptr) {
        throw utils::ValidationError("no patient with ID " + appointment->patientId());
    }
    Appointment& stored = *appointment;
    database_.addAppointment(std::move(appointment));
    if (stored.status() == AppointmentStatus::Pending) {
        if (auto* appointable = appointableOf(patient)) {
            appointable->scheduleAppointment(stored.id());
        }
    }
    ++revision_;
    return stored;
}

void ClinicManager::cancelAppointment(const std::string& appointmentId) {
    Appointment* appointment = database_.findAppointment(appointmentId);
    if (appointment == nullptr) {
        throw utils::ValidationError("no appointment with ID " + appointmentId);
    }
    if (appointment->status() == AppointmentStatus::Cancelled) {
        throw utils::ValidationError("appointment " + appointmentId + " is already cancelled");
    }
    if (appointment->status() == AppointmentStatus::Completed) {
        throw utils::ValidationError("appointment " + appointmentId + " is already completed");
    }
    appointment->setStatus(AppointmentStatus::Cancelled);
    if (PatientRecord* patient = database_.findPatient(appointment->patientId())) {
        if (auto* appointable = appointableOf(patient)) {
            if (appointable->currentAppointmentId() == appointmentId) {
                appointable->cancelAppointment();
            }
        }
        database_.touchRecent(patient);
    }
    ++revision_;
}

void ClinicManager::removeAppointment(const std::string& appointmentId) {
    const model::Appointment* existing = database_.findAppointment(appointmentId);
    if (existing == nullptr) {
        throw utils::ValidationError("no appointment with ID " + appointmentId);
    }
    // Save appointment snapshot for undo
    auto action = RemoveAppointmentAction::capture(database_, appointmentId);
    std::unique_ptr<Appointment> appointment = database_.takeAppointment(appointmentId);
    if (PatientRecord* patient = database_.findPatient(appointment->patientId())) {
        if (auto* appointable = appointableOf(patient)) {
            if (appointable->currentAppointmentId() == appointmentId) {
                appointable->cancelAppointment();
            }
        }
        database_.touchRecent(patient);
    }
    ++revision_;
    undoHistory_.push(std::move(action));
}

void ClinicManager::addToQueue(const std::string& patientId, const std::string& arrivalTime) {
    PatientRecord* patient = database_.findPatient(patientId);
    if (patient == nullptr) {
        throw utils::ValidationError("no patient with ID " + patientId);
    }
    if (database_.isQueued(patientId)) {
        throw utils::ValidationError("patient " + patientId + " is already in the waiting queue");
    }
    const std::string arrival = arrivalTime.empty() ? currentTime() : arrivalTime;
    utils::validateTime(arrival);
    database_.enqueue(patientId, arrival);
    database_.touchRecent(patient);
}

model::PatientRecord* ClinicManager::callNext(const std::string& today) {
    if (database_.waitingQueue().empty()) {
        return nullptr;
    }
    const std::string day = today.empty() ? currentDate() : today;

    // Drain queue to inspect waiting patients and arrival times
    struct Waiting {
        std::string id;
        std::string arrival;
    };
    std::vector<Waiting> waiting;
    while (!database_.waitingQueue().empty()) {
        const std::string id = database_.waitingQueue().front();
        waiting.push_back({id, database_.queueArrivalTime(id)});
        database_.tryDequeueFront();
    }

    // Find highest priority patient (emergency first, FIFO for ties)
    std::size_t served = waiting.size();
    int servedPriority = -1;
    for (std::size_t i = 0; i < waiting.size(); ++i) {
        const PatientRecord* candidate = database_.findPatient(waiting[i].id);
        if (candidate != nullptr && candidate->priority() > servedPriority) {
            servedPriority = candidate->priority();
            served = i;
        }
    }

    // Re-queue remaining patients in arrival order
    for (std::size_t i = 0; i < waiting.size(); ++i) {
        if (i != served && database_.findPatient(waiting[i].id) != nullptr) {
            database_.enqueue(waiting[i].id, waiting[i].arrival);
        }
    }
    if (served == waiting.size()) {
        return nullptr;
    }
    PatientRecord* patient = database_.findPatient(waiting[served].id);

    // Complete today's pending appointment if scheduled
    model::Appointment* todays = nullptr;
    for (model::Appointment* appointment : database_.appointmentsFor(patient->id())) {
        if (appointment->status() == AppointmentStatus::Pending &&
            appointment->date() == day) {
            if (todays == nullptr || appointment->time() < todays->time()) {
                todays = appointment;
            }
        }
    }
    if (todays != nullptr) {
        todays->setStatus(AppointmentStatus::Completed);
        ++revision_;
    }
    database_.touchRecent(patient);
    return patient;
}

void ClinicManager::clear() {
    undoHistory_ = {};
    database_.clear();
    idGenerator_ = utils::IDGenerator{};
    ++revision_;
}

void ClinicManager::undo() {
    if (undoHistory_.empty()) {
        return;
    }
    std::unique_ptr<UndoableAction> action = std::move(undoHistory_.top());
    undoHistory_.pop();
    action->undo(*this);
    ++revision_;
}

std::size_t ClinicManager::undoDepth() const {
    return undoHistory_.size();
}

std::string ClinicManager::nextUndoDescription() const {
    return undoHistory_.empty() ? std::string() : undoHistory_.top()->describe();
}

unsigned ClinicManager::revision() const {
    return revision_;
}

std::string ClinicManager::currentDate() {
    char buffer[11];
    const std::time_t now = std::time(nullptr);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&now));
    return buffer;
}

std::string ClinicManager::currentTime() {
    char buffer[6];
    const std::time_t now = std::time(nullptr);
    std::strftime(buffer, sizeof(buffer), "%H:%M", std::localtime(&now));
    return buffer;
}

unsigned ClinicManager::patientIdCounter() const {
    return idGenerator_.patientCounter();
}

unsigned ClinicManager::appointmentIdCounter() const {
    return idGenerator_.appointmentCounter();
}

void ClinicManager::restoreIdCounters(unsigned highestPatientNumber,
                                      unsigned highestAppointmentNumber) {
    idGenerator_.restorePatientCounter(highestPatientNumber);
    idGenerator_.restoreAppointmentCounter(highestAppointmentNumber);
}

const model::ClinicDatabase& ClinicManager::database() const noexcept {
    return database_;
}

// CSV loading and saving helpers
namespace {

// Helper to parse unsigned integer from string.
unsigned parseDigits(const std::string& text, const std::string& what) {
    if (text.empty()) {
        throw utils::ValidationError(what + " is empty, expected a whole number");
    }
    for (const char c : text) {
        if (c < '0' || c > '9') {
            throw utils::ValidationError(what + " \"" + text + "\" is not a whole number");
        }
    }
    try {
        const unsigned long long parsed = std::stoull(text);
        if (parsed > 4294967295ull) {
            throw std::out_of_range("counter overflow");
        }
        return static_cast<unsigned>(parsed);
    } catch (const std::out_of_range&) {
        throw utils::ValidationError(what + " \"" + text + "\" is out of range");
    }
}

// Parse and validate age from CSV field (0-120).
int parseAgeFromCsv(const std::string& text) {
    const unsigned value = parseDigits(text, "age");
    if (value > 120u) {
        throw utils::ValidationError("age " + text + " is outside 0-120");
    }
    return static_cast<int>(value);
}

// Returns highest numeric ID seen in a column (e.g. "P-0007" -> 7).
unsigned highestNumberIn(const std::vector<utils::CsvRecord>& file, char prefix) {
    unsigned highest = 0;
    for (std::size_t i = 1; i < file.size(); ++i) {
        const utils::CsvRecord& row = file[i];
        const std::string& id = row.empty() ? std::string() : row[0];
        if (id.size() > 2 && id[0] == prefix && id[1] == '-') {
            try {
                const unsigned number = parseDigits(id.substr(2), "ID number");
                if (number > highest) {
                    highest = number;
                }
            } catch (const std::exception&) {
                // skip malformed ID
            }
        }
    }
    return highest;
}

unsigned largerOf(unsigned a, unsigned b) {
    return a > b ? a : b;
}

// Verify expected CSV header format.
void requireHeader(const std::vector<utils::CsvRecord>& file, const char* expected,
                   const std::string& fileName) {
    if (file.empty() || file[0].size() != 6) {
        throw utils::FileError(fileName + ": missing or malformed header row (expected " +
                               expected + ")");
    }
    std::string joined;
    for (std::size_t i = 0; i < file[0].size(); ++i) {
        if (i > 0) {
            joined += ',';
        }
        joined += file[0][i];
    }
    if (joined != expected) {
        throw utils::FileError(fileName + ": unexpected header \"" + joined + "\" (expected " +
                               expected + ")");
    }
}

}  // namespace

void ClinicManager::saveClinic(const std::string& dataDir) const {
    try {
        std::filesystem::create_directories(dataDir);
    } catch (const std::filesystem::filesystem_error& error) {
        throw utils::FileError("cannot create data directory " + dataDir + ": " +
                               error.what());
    }

    std::vector<utils::CsvRecord> patientFile;
    patientFile.push_back({"id", "name", "age", "contact", "category", "followUpDate"});
    for (const std::unique_ptr<model::PatientRecord>& record : database_.patients()) {
        std::string followUpDate;
        if (record->category() == model::PatientCategory::FollowUp) {
            followUpDate =
                static_cast<const model::FollowUpPatient*>(record.get())->nextVisitDate();
        }
        patientFile.push_back({record->id(), record->name(), std::to_string(record->age()),
                               record->contact(), model::categoryName(record->category()),
                               followUpDate});
    }

    std::vector<utils::CsvRecord> appointmentFile;
    appointmentFile.push_back({"id", "patientId", "date", "time", "status", "notes"});
    for (const std::unique_ptr<model::Appointment>& appointment : database_.appointments()) {
        appointmentFile.push_back(
            {appointment->id(), appointment->patientId(), appointment->date(),
             appointment->time(), model::appointmentStatusName(appointment->status()),
             appointment->notes()});
    }

    const std::vector<utils::CsvRecord> counterFile = {
        {"patientCounter", "appointmentCounter"},
        {std::to_string(patientIdCounter()), std::to_string(appointmentIdCounter())},
    };

    utils::FileHandler::writeCsv(
        utils::FileHandler::dataFilePath(dataDir, utils::FileHandler::kPatientsFile), patientFile);
    utils::FileHandler::writeCsv(
        utils::FileHandler::dataFilePath(dataDir, utils::FileHandler::kAppointmentsFile),
        appointmentFile);
    utils::FileHandler::writeCsv(
        utils::FileHandler::dataFilePath(dataDir, utils::FileHandler::kCountersFile), counterFile);
}

ClinicManager::LoadSummary ClinicManager::loadClinic(const std::string& dataDir) {
    // Read and verify headers before modifying database
    const std::vector<utils::CsvRecord> patientFile = utils::FileHandler::readCsv(
        utils::FileHandler::dataFilePath(dataDir, utils::FileHandler::kPatientsFile));
    const std::vector<utils::CsvRecord> appointmentFile = utils::FileHandler::readCsv(
        utils::FileHandler::dataFilePath(dataDir, utils::FileHandler::kAppointmentsFile));
    requireHeader(patientFile, "id,name,age,contact,category,followUpDate",
                  utils::FileHandler::kPatientsFile);
    requireHeader(appointmentFile, "id,patientId,date,time,status,notes",
                  utils::FileHandler::kAppointmentsFile);

    // Load persisted counters if present
    unsigned persistedPatients = 0;
    unsigned persistedAppointments = 0;
    std::string counterNote;
    try {
        const std::vector<utils::CsvRecord> counterFile = utils::FileHandler::readCsv(
            utils::FileHandler::dataFilePath(dataDir, utils::FileHandler::kCountersFile));
        if (counterFile.size() >= 2 && counterFile[1].size() >= 2) {
            persistedPatients = parseDigits(counterFile[1][0], "patient counter");
            persistedAppointments = parseDigits(counterFile[1][1], "appointment counter");
        } else {
            counterNote = "counters.csv has no counter row; "
                          "counters derived from the highest IDs in the data files";
        }
    } catch (const std::exception& error) {
        counterNote = std::string("counters.csv ignored (") + error.what() +
                      "); counters derived from the highest IDs in the data files";
    }

    clear();

    restoreIdCounters(largerOf(persistedPatients, highestNumberIn(patientFile, 'P')),
                      largerOf(persistedAppointments, highestNumberIn(appointmentFile, 'A')));

    LoadSummary summary;
    for (std::size_t i = 1; i < patientFile.size(); ++i) {
        const utils::CsvRecord& row = patientFile[i];
        // CSV record number, 1-based with the header as row 1.
        const std::size_t fileRow = i + 1;
        try {
            if (row.size() != 6) {
                throw utils::ValidationError(
                    "expected 6 fields (id,name,age,contact,category,followUpDate), got " +
                    std::to_string(row.size()));
            }
            addPatientRecord(makePatientRecord(row[0], row[1], parseAgeFromCsv(row[2]), row[3],
                                                model::patientCategoryFromString(row[4]),
                                                row[5]));
            ++summary.patientsLoaded;
        } catch (const std::exception& error) {
            ++summary.patientsSkipped;
            summary.issues.push_back(std::string(utils::FileHandler::kPatientsFile) + " row " +
                                     std::to_string(fileRow) + " skipped: " + error.what());
        }
    }

    for (std::size_t i = 1; i < appointmentFile.size(); ++i) {
        const utils::CsvRecord& row = appointmentFile[i];
        const std::size_t fileRow = i + 1;
        try {
            if (row.size() != 6) {
                throw utils::ValidationError(
                    "expected 6 fields (id,patientId,date,time,status,notes), got " +
                    std::to_string(row.size()));
            }
            addAppointmentRecord(std::make_unique<model::Appointment>(
                row[0], row[1], row[2], row[3], model::appointmentStatusFromString(row[4]),
                row[5]));
            ++summary.appointmentsLoaded;
        } catch (const std::exception& error) {
            ++summary.appointmentsSkipped;
            summary.issues.push_back(std::string(utils::FileHandler::kAppointmentsFile) +
                                     " row " + std::to_string(fileRow) + " skipped: " +
                                     error.what());
        }
    }

    if (!counterNote.empty()) {
        summary.issues.push_back("note: " + counterNote);
    }
    return summary;
}

}  // namespace scapms::controller
