#include "utils/FileHandler.h"

#include "utils/Exceptions.h"

#include <fstream>
#include <sstream>

namespace scapms::utils {
namespace {

// True when a field must be quoted to survive a round trip (RFC 4180): any
// comma, double quote, or line break inside would otherwise split the field.
bool needsQuoting(const std::string& field) {
    return field.find_first_of(",\"\r\n") != std::string::npos;
}

// One CSV field, quoted when needed with embedded quotes doubled ("" for ").
std::string encodeField(const std::string& field) {
    if (!needsQuoting(field)) {
        return field;
    }
    std::string encoded = "\"";
    for (const char c : field) {
        if (c == '"') {
            encoded += "\"\"";
        } else {
            encoded += c;
        }
    }
    encoded += '"';
    return encoded;
}

}  // namespace

std::string FileHandler::dataFilePath(const std::string& dataDir,
                                      const std::string& fileName) {
    if (dataDir.empty() || dataDir.back() == '/' || dataDir.back() == '\\') {
        return dataDir + fileName;
    }
    return dataDir + '/' + fileName;
}

void FileHandler::writeCsv(const std::string& path, const std::vector<CsvRecord>& rows) {
    // Text mode: '\n' becomes the platform's native ending (CRLF on Windows).
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw FileError("cannot open for writing: " + path);
    }
    for (const CsvRecord& row : rows) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i > 0) {
                out << ',';
            }
            out << encodeField(row[i]);
        }
        out << '\n';
    }
    if (!out) {
        throw FileError("write failed midway: " + path);
    }
}

std::vector<CsvRecord> FileHandler::readCsv(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw FileError("cannot open: " + path);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    if (in.bad()) {
        throw FileError("cannot read: " + path);
    }
    std::string text = buffer.str();

    // A UTF-8 BOM would otherwise glue itself onto the first header field.
    if (text.compare(0, 3, "\xEF\xBB\xBF") == 0) {
        text.erase(0, 3);
    }

    // Character-at-a-time state machine. Records end at a line break outside
    // quotes (LF, CRLF, or lone CR); inside quotes a CRLF normalizes to LF so
    // a field round-trips byte-identically no matter how the file was saved.
    std::vector<CsvRecord> rows;
    CsvRecord record;
    std::string field;
    bool inQuotes = false;
    bool sawQuote = false;  // the current record contained a quoted section

    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < text.size() && text[i + 1] == '"') {
                    field += '"';  // doubled quote = one literal quote
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
                field += '\n';
                ++i;
            } else {
                field += c;
            }
        } else if (c == '"') {
            inQuotes = true;  // a quote anywhere begins a quoted section
            sawQuote = true;
        } else if (c == ',') {
            record.push_back(field);
            field.clear();
        } else if (c == '\r' || c == '\n') {
            if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;  // consume the LF of a CRLF pair
            }
            // A truly blank line separates records; it is not a data row.
            if (!field.empty() || !record.empty() || sawQuote) {
                record.push_back(field);
                rows.push_back(record);
            }
            record.clear();
            field.clear();
            sawQuote = false;
        } else {
            field += c;
        }
    }
    if (inQuotes) {
        throw FileError(path + ": quoted field is never closed");
    }
    // Flush the final record - the file need not end with a newline.
    if (!field.empty() || !record.empty() || sawQuote) {
        record.push_back(field);
        rows.push_back(record);
    }
    return rows;
}

}  // namespace scapms::utils
