#include <algorithm>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <format>

#include "utils.hh"

bool is_issue_before(const std::string& a, const std::string& b) {
    bool is_before;
    const auto a_dash = std::find(a.cbegin(), a.cend(), '-');
    const auto b_dash = std::find(b.cbegin(), b.cend(), '-');
    if ((a_dash == a.cend()) || (b_dash == b.cend())
        || (std::string(a.cbegin(), a_dash) != std::string(b.cbegin(), b_dash))) {
        is_before = a < b;
    } else {
        try {
            const auto num_a = std::stol(std::string(std::next(a_dash), a.cend()));
            const auto num_b = std::stol(std::string(std::next(b_dash), b.cend()));
            is_before = num_a < num_b;
        } catch (...) {
            is_before = a < b;
        }
    }
    return is_before;
}

namespace {
    auto base64_decode_c(const unsigned char c) -> std::uint8_t {
        if (('A' <= c) && (c <= 'Z')) {
            return static_cast<std::uint8_t>(c - 'A');
        }
        if (('a' <= c) && (c <= 'z')) {
            return 26 + static_cast<std::uint8_t>(c - 'a');
        }
        if (('0' <= c) && (c <= '9')) {
            return 52 + static_cast<std::uint8_t>(c - '0');
        }
        if (c == '+') {
            return 62;
        }
        if (c == '/') {
            return 63;
        }
        throw std::invalid_argument(std::format("Can't decode character {} as base64", c));
    }
}

#include <iostream>
auto base64_decode(const std::string_view& input) -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> res;
    const auto nr_chars = input.size();
    if (nr_chars == 0) {
        return {}; // empty string means empty data
    }
    if (nr_chars < 2) {
        throw std::invalid_argument(std::format("Not enough characters to do base64 decoding. At least 2 required"));
    }
    size_t nr_padding = 0;
    if (input[nr_chars - 1] == '=') {
        nr_padding += 1;
    }
    if (input[nr_chars - 2] == '=') {
        nr_padding += 1;
    }

    const auto nr_chars_without_padding = nr_chars - nr_padding;
    res.reserve((nr_chars_without_padding + 3) / 4); // +3  for ceiling

    unsigned i;
    for (i = 0; i + 3 < nr_chars_without_padding; i += 4) {
        const auto a = static_cast<unsigned char>(input[i]);
        const auto b = static_cast<unsigned char>(input[ i + 1 ]);
        const auto c = static_cast<unsigned char>(input[ i + 2 ]);
        const auto d = static_cast<unsigned char>(input[ i + 3 ]);

        const auto decoded_a = base64_decode_c(a);
        const auto decoded_b = base64_decode_c(b);
        const auto decoded_c = base64_decode_c(c);
        const auto decoded_d = base64_decode_c(d);

        const auto byte_1 = static_cast<uint8_t>((decoded_a << 2) | (decoded_b >> 4));
        const auto byte_2 = static_cast<uint8_t>(((decoded_b & 0x0F) << 4) | (decoded_c >> 2));
        const auto byte_3 = static_cast<uint8_t>(((decoded_c & 0x03) << 6) | decoded_d);

        res.push_back(byte_1);
        res.push_back(byte_2);
        res.push_back(byte_3);
    }
    switch (nr_padding) {
        case 0:
            break;
        case 1: {
            // two more bytes
            const auto a = static_cast<unsigned char>(input[i]);
            const auto b = static_cast<unsigned char>(input[ i + 1 ]);
            const auto c = static_cast<unsigned char>(input[ i + 2 ]);

            const auto decoded_a = base64_decode_c(a);
            const auto decoded_b = base64_decode_c(b);
            const auto decoded_c = base64_decode_c(c);

            const auto byte_1 = static_cast<uint8_t>((decoded_a << 2) | (decoded_b >> 4));
            const auto byte_2 = static_cast<uint8_t>(((decoded_b & 0x0F) << 4) | (decoded_c >> 2));
            res.push_back(byte_1);
            res.push_back(byte_2);
            break;
        };
        case 2: {
            // one more byte
            const auto a = static_cast<unsigned char>(input[i]);
            const auto b = static_cast<unsigned char>(input[ i + 1 ]);

            const auto decoded_a = base64_decode_c(a);
            const auto decoded_b = base64_decode_c(b);

            const auto byte_1 = static_cast<uint8_t>((decoded_a << 2) | (decoded_b >> 4));
            res.push_back(byte_1);
            break;
        };
        default:
            __builtin_unreachable();
    }
    return res;
}
