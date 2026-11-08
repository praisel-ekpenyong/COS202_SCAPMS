#include "model/PatientRecord.h"

#include "utils/IDGenerator.h"
#include "utils/Validators.h"

namespace scapms::model {

PatientRecord::PatientRecord(std::string id, std::string name, int age,
                             std::string contact, PatientCategory category)
    : id_(std::move(id)),
      name_(std::move(name)),
      age_(age),
      contact_(std::move(contact)),
      category_(category) {
    if (!scapms::utils::isPatientId(id_)) {
        throw scapms::utils::ValidationError(
            "patient ID must have format P-0001, got \"" + id_ + "\"");
    }
    scapms::utils::validateName(name_);
    scapms::utils::validateAge(age_);
    scapms::utils::validateContact(contact_);
}

const std::string& PatientRecord::id() const {
    return id_;
}

const std::string& PatientRecord::name() const {
    return name_;
}

int PatientRecord::age() const {
    return age_;
}

const std::string& PatientRecord::contact() const {
    return contact_;
}

PatientCategory PatientRecord::category() const {
    return category_;
}

void PatientRecord::setName(const std::string& name) {
    scapms::utils::validateName(name);
    name_ = name;
}

void PatientRecord::setAge(int age) {
    scapms::utils::validateAge(age);
    age_ = age;
}

void PatientRecord::setContact(const std::string& contact) {
    scapms::utils::validateContact(contact);
    contact_ = contact;
}

}  // namespace scapms::model
