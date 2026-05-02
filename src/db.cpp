#include "db.h"

#include <sqlite3.h>

bool ensure_database(const fs::path &db_path) {
    sqlite3 *db = nullptr;
    if (sqlite3_open(db_path.string().c_str(), &db) != SQLITE_OK) {
        return false;
    }

    const char *schema_sql =
        "CREATE TABLE IF NOT EXISTS test_cases ("
        "question_id TEXT NOT NULL,"
        "case_id INTEGER NOT NULL,"
        "input TEXT NOT NULL,"
        "expected_output TEXT NOT NULL,"
        "PRIMARY KEY(question_id, case_id)"
        ");";

    char *err = nullptr;
    if (sqlite3_exec(db, schema_sql, nullptr, nullptr, &err) != SQLITE_OK) {
        sqlite3_free(err);
        sqlite3_close(db);
        return false;
    }

    const char *count_sql =
        "SELECT COUNT(*) FROM test_cases WHERE question_id = 'sum1';";
    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, count_sql, -1, &stmt, nullptr);
    int count = 0;
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (count == 0) {
        const char *seed_sql =
            "INSERT INTO test_cases(question_id, case_id, input, expected_output) VALUES"
            "('sum1', 1, '1 2\n', '3\n'),"
            "('sum1', 2, '10 5\n', '15\n');";
        sqlite3_exec(db, seed_sql, nullptr, nullptr, &err);
        if (err) {
            sqlite3_free(err);
        }
    }

    sqlite3_close(db);
    return true;
}

std::vector<TestCase> load_test_cases(const fs::path &db_path, const std::string &question_id) {
    std::vector<TestCase> cases;
    sqlite3 *db = nullptr;
    if (sqlite3_open(db_path.string().c_str(), &db) != SQLITE_OK) {
        return cases;
    }

    const char *sql =
        "SELECT case_id, input, expected_output FROM test_cases "
        "WHERE question_id = ? ORDER BY case_id ASC;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return cases;
    }

    sqlite3_bind_text(stmt, 1, question_id.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TestCase tc;
        tc.case_id = sqlite3_column_int(stmt, 0);
        const unsigned char *input = sqlite3_column_text(stmt, 1);
        const unsigned char *expected = sqlite3_column_text(stmt, 2);
        tc.input = input ? reinterpret_cast<const char *>(input) : "";
        tc.expected = expected ? reinterpret_cast<const char *>(expected) : "";
        cases.push_back(std::move(tc));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return cases;
}
