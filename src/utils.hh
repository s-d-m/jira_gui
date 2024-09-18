#pragma once

#include <string>
#include <string_view>
#include <vector>

bool is_issue_before(const std::string& a, const std::string& b);
auto base64_decode(const std::string_view& input) -> std::vector<std::uint8_t>;
