#ifndef EXEC_SERVICE_TYPES_H
#define EXEC_SERVICE_TYPES_H

#include <string>

struct TestCase {
    int case_id = 0;
    std::string input;
    std::string expected;
};

struct CaseResult {
    int case_id = 0;
    bool passed = false;
    std::string output;
    std::string expected;
    std::string error;
};

struct ExecResult {
    int exit_code = -1;
    bool timed_out = false;
    bool signaled = false;
    std::string stdout_data;
    std::string stderr_data;
};

#endif
