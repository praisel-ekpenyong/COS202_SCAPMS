#pragma once
// Base class for patient records.
#include "model/PatientCategory.h"

#include <string>

namespace scapms::model {

class PatientRecord {
public:
    // Validates fields on creation.
    PatientRecord(std::string id, std::string name, int age,
                  std::string contact, PatientCategory category);
    virtual ~PatientRecord() = default;

    // Non-copyable; managed via unique_ptr.
    PatientRecord(const PatientRecord&) = delete;
    PatientRecord& operator=(const PatientRecord&) = delete;

    const std::string& id() const;
    const std::string& name() const;
    int age() const;
    const std::string& contact() const;
    PatientCategory category() const;

    // Field setters.
    void setName(const std::string& name);
    void setAge(int age);
    void setContact(const std::string& contact);

    // Category description.
    virtual std::string describe() const = 0;

    // Waiting queue priority (higher = served earlier).
    virtual int priority() const = 0;

private:
    std::string id_;
    std::string name_;
    int age_;
    std::string contact_;
    PatientCategory category_;
};

}  // namespace scapms::model
