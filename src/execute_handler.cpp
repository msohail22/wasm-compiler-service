#include "execute_handler.h"

#include "db.h"
#include "exec.h"
#include "json_utils.h"
#include "types.h"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::string build_response(const std::string &status,
                                  int passed,
                                  int total,
                                  const std::vector<CaseResult> &cases,
                                  const std::string &error) {
    std::ostringstream out;
    out << "{";
    out << "\"status\":\"" << json_escape(status) << "\",";
    out << "\"passed\":" << passed << ",";
    out << "\"total\":" << total;
    if (!error.empty()) {
        out << ",\"error\":\"" << json_escape(error) << "\"";
    }
    out << ",\"cases\":[";
    for (size_t i = 0; i < cases.size(); ++i) {
        const auto &c = cases[i];
        out << "{";
        out << "\"case_id\":" << c.case_id << ",";
        out << "\"passed\":" << (c.passed ? "true" : "false") << ",";
        out << "\"output\":\"" << json_escape(c.output) << "\",";
        out << "\"expected\":\"" << json_escape(c.expected) << "\",";
        out << "\"error\":\"" << json_escape(c.error) << "\"";
        out << "}";
        if (i + 1 < cases.size()) {
            out << ",";
        }
    }
    out << "]}";
    return out.str();
}

std::string handle_execute(const std::string &body, const fs::path &db_path) {
    auto code = json_get_string(body, "code");
    auto language = json_get_string(body, "language");
    auto question_id = json_get_string(body, "question_id");

    if (!code || !language || !question_id) {
        return build_response("runtime_error", 0, 0, {}, "missing_fields");
    }

    if (*language != "cpp") {
        return build_response("runtime_error", 0, 0, {}, "unsupported_language");
    }

    std::vector<TestCase> cases = load_test_cases(db_path, *question_id);
    if (cases.empty()) {
        return build_response("runtime_error", 0, 0, {}, "no_test_cases");
    }

    std::string temp_pattern = "/tmp/execsvcXXXXXX";
    std::vector<char> tmpl(temp_pattern.begin(), temp_pattern.end());
    tmpl.push_back('\0');
    char *dir = mkdtemp(tmpl.data());
    if (!dir) {
        return build_response("runtime_error", 0, 0, {}, "tempdir_failed");
    }

    fs::path workdir(dir);
    fs::path src_path = workdir / "main.cpp";
    fs::path bin_path = workdir / "program";

    if (!write_file(src_path, *code)) {
        fs::remove_all(workdir);
        return build_response("runtime_error", 0, 0, {}, "write_failed");
    }

    std::vector<std::string> compile_args = {
        "g++", "-std=c++17", "-O2", "-pipe", src_path.string(), "-o", bin_path.string()
    };

    ExecResult compile_result = run_process(compile_args, "", 10, 512);
    if (compile_result.exit_code != 0 || compile_result.signaled || compile_result.timed_out) {
        fs::remove_all(workdir);
        return build_response("compilation_error", 0, static_cast<int>(cases.size()), {}, compile_result.stderr_data);
    }

    std::vector<CaseResult> results;
    results.reserve(cases.size());
    int passed = 0;
    bool runtime_error = false;

    for (const auto &tc : cases) {
        CaseResult cr;
        cr.case_id = tc.case_id;
        cr.expected = tc.expected;

        std::vector<std::string> run_args = { bin_path.string() };
        ExecResult run_result = run_process(run_args, tc.input, 2, 256);

        if (run_result.timed_out) {
            cr.error = "timeout";
            runtime_error = true;
        } else if (run_result.signaled || run_result.exit_code != 0) {
            cr.error = "nonzero_exit";
            runtime_error = true;
        } else {
            cr.output = run_result.stdout_data;
            cr.passed = (cr.output == tc.expected);
            if (cr.passed) {
                passed++;
            }
        }

        if (!run_result.stderr_data.empty() && cr.error.empty()) {
            cr.error = run_result.stderr_data;
        }

        results.push_back(std::move(cr));
    }

    fs::remove_all(workdir);

    std::string status = runtime_error ? "runtime_error" : "success";
    return build_response(status, passed, static_cast<int>(cases.size()), results, "");
}
