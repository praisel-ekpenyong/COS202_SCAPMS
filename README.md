# Smart Clinic Appointment & Patient Management System (SCAPMS)

**Course:** COS 202: Computer Programming II  
**Institution:** Miva Open University  
**Faculty:** Faculty of Computing and Applied Sciences  
**Department:** Department of Cybersecurity  
**Student Name:** Praisel Ekpenyong  
**Matriculation Number:** 2025/A/CYB/0205  
**Submission Document:** [`SCAPMS_submission.pdf`](./SCAPMS_submission.pdf)

---

## Overview

The **Smart Clinic Appointment & Patient Management System (SCAPMS)** is a high-performance desktop clinic management application implemented in **C++17** and **Qt 6 Widgets**. It provides end-to-end management of clinic operations:

- **Polymorphic Patient Records:** Object-oriented hierarchy with `PatientRecord` base class and concrete specializations (`GeneralPatient`, `EmergencyPatient`, `FollowUpPatient`).
- **Scheduling Contract:** Segregation of concerns via the `Appointable` abstract interface.
- **Priority Waiting Queue:** Priority queue prioritizing emergency cases ahead of general appointments with a 15-minute wait-time threshold alert.
- **Algorithms:** Student-implemented Sorting (`Selection Sort`, `Merge Sort`) and Searching (`Linear Search`, `Binary Search`, `Recursive Search`) without reliance on STL algorithmic helpers.
- **Administrative Undo:** Multi-level undo stack capturing state mutations (patient deletion, edit, appointment removal).
- **Analytical Reporting:** Operational clinic reports (frequently attended, overdue pending appointments, category breakdown, daily clinic summary) with text export.
- **Data Persistence:** Full state persistence across application sessions using structured CSV files.

---

## Repository Structure

```
COS202_SCAPMS/
├── CMakeLists.txt              # Root CMake project configuration
├── README.md                   # Repository overview and build guide
├── SCAPMS_submission.pdf       # Primary assessment deliverable
│
├── model/                      # Domain models & database
│   ├── PatientRecord.h/.cpp    # Base patient record
│   ├── GeneralPatient.h/.cpp   # General outpatient category (Priority 1)
│   ├── EmergencyPatient.h/.cpp # Emergency triage category (Priority 3)
│   ├── FollowUpPatient.h/.cpp  # Follow-up patient category (Priority 2)
│   ├── Appointable.h           # Scheduling capability interface
│   ├── Appointment.h/.cpp      # Appointment entity
│   ├── AppointmentStatus.h     # Appointment status enum
│   ├── PatientCategory.h       # Patient category enum
│   └── ClinicDatabase.h/.cpp   # In-memory storage & LRU cache
│
├── controller/                 # Business logic & algorithms
│   ├── ClinicManager.h/.cpp    # Facade coordinator & undo stack
│   ├── SearchEngine.h/.cpp     # Custom Linear, Binary & Recursive search
│   ├── PatientSorter.h/.cpp    # Custom Selection & Merge sort routines
│   ├── SortAlgorithms.h        # Template sorting implementations
│   ├── ReportEngine.h/.cpp     # Clinic reports calculation & export
│   └── UndoableAction.h/.cpp   # Command-pattern undo action records
│
├── gui/                        # Qt 6 Widgets user interface
│   ├── MainWindow.h/.cpp       # Main window and tab container
│   ├── DashboardPanel.h/.cpp   # Overview dashboard & waiting alerts
│   ├── PatientPanel.h/.cpp     # Patient registration & table
│   ├── AppointmentPanel.h/.cpp # Booking & overdue indicators
│   ├── QueuePanel.h/.cpp       # Priority triage waiting queue
│   ├── SearchPanel.h/.cpp      # Interactive search & sort panel
│   ├── ReportPanel.h/.cpp      # Analytical reports viewer & export
│   ├── AdminPanel.h/.cpp       # Undo history ledger & log
│   ├── Theme.h/.cpp            # Clean healthcare visual theme
│   ├── main.cpp                # Application entry point
│   └── resources.qrc           # Application resource bundle
│
├── utils/                      # Helper utilities
│   ├── Validators.h/.cpp       # Regex form validation
│   ├── IDGenerator.h/.cpp      # Persistent auto-incrementing IDs
│   ├── FileHandler.h/.cpp      # CSV reading and writing engine
│   ├── Exceptions.h            # Custom exception classes
│   └── AppInfo.h/.cpp          # Application metadata
│
├── data/                       # CSV persistent datasets
│   ├── patients.csv            # Sample patient records
│   ├── appointments.csv        # Sample appointment records
│   └── counters.csv            # ID generator sequence counters
│
└── docs/                       # Assessment & documentation assets
    └── submission/             # Screenshots, UML diagram (SVG/PNG), build scripts
```

---

## Build & Execution Instructions

### Prerequisites
- C++17 compliant compiler (GCC 11+, Clang 13+, or MSVC 2019+)
- CMake 3.21 or newer
- Qt 6 (Widgets, Core, Gui)
- Ninja or Make

### Build Steps

```powershell
# 1. Configure the build environment
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 2. Compile the application
cmake --build build

# 3. Launch SCAPMS
./build/gui/SCAPMS.exe
```
