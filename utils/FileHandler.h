#pragma once
// FileHandler: helper for reading and writing CSV files.
#include <string>
#include <vector>

namespace scapms::utils {

using CsvRecord = std::vector<std::string>;

class FileHandler {
public:
    static constexpr const char* kPatientsFile = "patients.csv";
    static constexpr const char* kAppointmentsFile = "appointments.csv";
    static constexpr const char* kCountersFile = "counters.csv";

    // Combines directory path and file name.
    static std::string dataFilePath(const std::string& dataDir,
                                    const std::string& fileName);

    // Writes rows to CSV file, handling quotes for fields with commas.
    static void writeCsv(const std::string& path, const std::vector<CsvRecord>& rows);

    // Reads rows from CSV file, handling quoted multi-line fields.
    static std::vector<CsvRecord> readCsv(const std::string& path);
};

}  // namespace scapms::utils
