#ifndef EXEC_SERVICE_JSON_UTILS_H
#define EXEC_SERVICE_JSON_UTILS_H

#include <optional>
#include <string>

std::string trim_left(const std::string &s);
std::string json_escape(const std::string &s);
std::optional<std::string> json_get_string(const std::string &body, const std::string &key);

#endif
