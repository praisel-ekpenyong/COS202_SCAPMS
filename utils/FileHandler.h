#pragma once
// FileHandler: raw CSV file I/O for persistence.
// Deliberately format-blind - it parses and writes plain
// string records only. It must not include model/ or controller/ types
// (model already depends on utils, so a model include here would cycle);
// controller::ClinicManager converts the string records into domain records
// through its own seam (makePatientRecord / add*Record).
#include <string>
#include <vector>

namespace scapms::utils {

// One parsed CSV record: the raw string fields of a single row.
using CsvRecord = std::vector<std::string>;

class FileHandler {
public:
    // The fixed file names inside a clinic data directory.
    // counters.csv carries the persisted ID counters (counters are never
    // reused, so they are stored, not re-derived from the highest row).
    static constexpr const char* kPatientsFile = "patients.csv";
    static constexpr const char* kAppointmentsFile = "appointments.csv";
    static constexpr const char* kCountersFile = "counters.csv";

    // Joins a data directory with one of the file names above; tolerates a
    // trailing slash (forward and back).
    static std::string dataFilePath(const std::string& dataDir,
                                    const std::string& fileName);

    // Writes every row, header row first (the caller owns the schema). A
    // field containing a comma, double quote or newline is quoted (RFC 4180)
    // with embedded quotes doubled, so notes with commas survive a round
    // trip. Text mode: the platform's native line endings. Throws FileError
    // when the file cannot be opened or a write fails midway.
    static void writeCsv(const std::string& path, const std::vector<CsvRecord>& rows);

    // Reads every row of a CSV file, header row included. Tolerates LF and
    // CRLF endings (either may appear mixed), a UTF-8 BOM, blank lines, and
    // quoted fields spanning lines (RFC 4180); a double quote anywhere begins
    // a quoted section. An empty file yields an empty vector - deciding
    // whether that is fatal is the caller's (the loader requires a header).
    // Throws FileError when the file cannot be opened or a quoted field is
    // never closed (row boundaries are then unknowable, so the whole file is
    // rejected rather than mis-parsed from that point on).
    static std::vector<CsvRecord> readCsv(const std::string& path);
};

}  // namespace scapms::utils
