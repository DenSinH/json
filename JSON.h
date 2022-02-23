#pragma once


#include <string>
#include <sstream>
#include <optional>
#include <variant>
#include <map>
#include <array>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <fstream>


namespace JSON {

namespace Details {

static std::string escape(const std::string& source) {
    std::stringstream stream;
    for (auto c : source) {
        switch (c) {
            case '\t': stream << "\\t"; break;
            case '\n': stream << "\\n"; break;
            case '\r': stream << "\\r"; break;
            case '\"':
            case '\'':
            case '\\':
                stream << '\\';
            default:  // fallthrough
                stream << c;
                break;
        }
    }
    return stream.str();
}

}

struct JSON;
using Object = std::map<std::string, JSON>;
constexpr std::nullptr_t null = nullptr;
struct {
    std::vector<JSON> operator [](const std::vector<JSON>& json) const { return json; }
} Array;


struct JSON {

    constexpr JSON() {
        value = std::map<std::string, JSON>{};
    }

    template<typename T>
    constexpr JSON(const T& v) {
        value = v;
        if constexpr(std::is_same_v<T, Object>) {
            // make sure keys are valid
            for (const auto& it : v) {
                for (char c : it.first) {
                    if (!std::isalnum(c) && c != '_') [[unlikely]] {
                        throw std::runtime_error("Bad JSON object key: " + it.first);
                    }
                }
            }
        }
    }

    template<>
    constexpr JSON(const char* const& v) {
        // defaults to bool otherwise
        value = std::string{v};
    }

    template<typename ...Args>
    constexpr JSON(Args... args) {
        value = std::vector<JSON>{ args... };
    }

    JSON& operator[](size_t index) {
        if (std::holds_alternative<std::vector<JSON>>(value)) {
            return std::get<std::vector<JSON>>(value)[index];
        }
        throw std::runtime_error("Bad JSON array access");
    }

    JSON& operator[](const std::string& key) {
        if (std::holds_alternative<std::map<std::string, JSON>>(value)) {
            return std::get<Object>(value)[key];
        }
        throw std::runtime_error("Bad JSON map access");
    }

    const JSON& operator[](size_t index) const {
        if (std::holds_alternative<std::vector<JSON>>(value)) {
            return std::get<std::vector<JSON>>(value)[index];
        }
        throw std::runtime_error("Bad JSON array access");
    }

    const JSON& operator[](const std::string& key) const {
        if (std::holds_alternative<std::map<std::string, JSON>>(value)) {
            return std::get<Object>(value).at(key);
        }
        throw std::runtime_error("Bad JSON map access");
    }

    template<typename T>
    constexpr T get() const {
        if (std::holds_alternative<T>(value)) {
            return std::get<T>(value);
        }
        throw std::runtime_error("Bad JSON access");
    }
    template<> constexpr float get() const { return get<double>(); }

    // for sweeter syntax
    template<typename T>
    explicit operator T() const {
      return get<T>();
    }

    template<typename T>
    constexpr bool holds() const {
        return std::holds_alternative<T>(value);
    }
    template<> constexpr bool holds<float>() const { return holds<double>(); }

    constexpr bool is_null()   const { return holds<std::nullptr_t>(); }
    constexpr bool is_string() const { return holds<std::string>(); }
    constexpr bool is_int()    const { return holds<int>(); }
    constexpr bool is_float()  const { return holds<double>(); }
    constexpr bool is_bool()   const { return holds<bool>(); }
    constexpr bool is_object() const { return holds<Object>(); }
    constexpr bool is_array()  const { return holds<std::vector<JSON>>(); }

    template<int indent_amount = 0>
    std::string dump() const {
        std::stringstream stream;
        _dump<indent_amount>(stream, 0);
        return stream.str();
    }

    template<int indent_amount = 0>
    void dump_to(const std::filesystem::path& filename) const {
        std::ofstream file(filename, std::ios::trunc);
        if (!file.good()) {
            throw std::runtime_error("Could not open file");
        }

        file << dump<indent_amount>();
        file.close();
    }

    static JSON load(const char* string) {
        const char* c = string;
        return _load(c);
    }

    static JSON load(const std::string& string) { return load(string.c_str()); }

    static JSON load_from(const std::filesystem::path& filename) {
        std::ifstream file(filename);
        if (!file.good()) {
            throw std::runtime_error("Could not open file");
        }

        file.seekg(0, std::ios::beg);
        std::string content;
        content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return load(content);
    }

private:
    using underlying = std::variant<std::string, int, double, bool, Object, std::vector<JSON>, std::nullptr_t>;

    underlying value;

    template<int indent_amount = 0>
    void _dump(std::stringstream& stream, int indent_level, bool first_line = false) const {
        std::array<char, indent_amount + 1> indent = {};
        std::fill(indent.begin(), indent.end() - 1, ' ');

        if constexpr(indent_amount != 0) {
            if (!first_line)
                for (int i = 0; i < indent_level; i++) stream << indent.data();
        }

        if      (is_null())   { stream << "null"; }
        else if (is_string()) { stream << '\"' << Details::escape(get<std::string>()) << '\"'; }
        else if (is_int())    { stream << get<int>(); }
        else if (is_float())  { stream << get<double>(); }
        else if (is_bool())   { stream << (get<bool>() ? "true" : "false"); }
        else if (is_array()) {
            const std::vector<JSON>& arr = get<std::vector<JSON>>();
            stream << '[';
            for (int i = 0; i < arr.size(); i++) {
                const auto& elem = arr[i];
                if constexpr(indent_amount != 0) {
                    stream << '\n';
                }

                elem._dump<indent_amount>(stream, indent_level + 1);

                if (i != arr.size() - 1) {
                    stream << ',';
                    if constexpr(indent_amount == 0) { stream << ' '; }
                }
            }

            if constexpr(indent_amount != 0) {
                stream << '\n';
                for (int i = 0; i < indent_level; i++) stream << indent.data();
            }
            stream << ']';
        }
        else if (is_object()) {
            const Object& obj = get<Object>();
            stream << '{';
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                if constexpr(indent_amount != 0) {
                    stream << '\n';
                    for (int i = 0; i < indent_level + 1; i++) stream << indent.data();
                }
                stream << '\"' << it->first << '\"';
                stream << ':' << ' ';
                it->second._dump<indent_amount>(stream, indent_level + 1, true);
                if (it != --obj.end()) {
                    stream << ',';
                    if constexpr(indent_amount == 0) { stream << ' '; }
                }
            }
            if constexpr(indent_amount != 0) {
                stream << '\n';
                for (int i = 0; i < indent_level; i++) stream << indent.data();
            }
            stream << '}';
        }
    }

    static JSON _load(const char*& c) {
        auto skip_whitespace = [](const char*& c) {
            while (*c && std::isspace(*c)) { c++; }
        };
        auto expect = [&c](const char e) {
            if (*c++ != e) {
                std::stringstream err_stream{};
                err_stream << "Invalid JSON: expected " << e << " got " << *c;
                std::runtime_error(err_stream.str());
            }
        };

        skip_whitespace(c);
        switch (*c++) {
            case '\0':
                break;
            case 't': {
                expect('r'); expect('u'); expect('e');
                return JSON(true);
            }
            case 'f': {
                expect('a'); expect('l'); expect('s'); expect('e');
                return JSON(false);
            }
            case 'n': {
                expect('u'); expect('l'); expect('l');
                return JSON(nullptr);
            }
            case '\"': {
                std::stringstream stream{};
                while (*c && *c != '\"') {
                    if (*c == '\\') {
                        switch (*(c + 1)) {
                            case 't': stream << '\t'; c++; break;
                            case 'n': stream << '\n'; c++; break;
                            case 'r': stream << '\r'; c++; break;
                            case '\"':
                            case '\'':
                                stream << *(c + 1);
                                c++;
                                break;
                            case '\\':
                                stream << '\\';
                                c++;
                                break;
                            default:
                                stream << '\\';
                                break;
                        }
                    }
                    else {
                        stream << *c;
                    }
                    c++;
                }
                expect('\"');
                return JSON(stream.str());
            }
            case '[': {
                std::vector<JSON> arr{};
                skip_whitespace(c);
                if (*c == ']') {
                    c++;
                    return JSON(arr);
                }
                while (true) {
                    skip_whitespace(c);
                    arr.emplace_back(_load(c));
                    skip_whitespace(c);
                    if (*c != ',') {
                        expect(']');
                        return JSON(arr);
                    }
                    c++;
                }
            }
            case '{': {
                skip_whitespace(c);
                if (*c == '}') {
                    c++;
                    return JSON();
                }

                Object obj{};
                while (true) {
                    std::stringstream key{};
                    skip_whitespace(c);
                    expect('\"');
                    while (*c && (std::isalnum(*c) || *c == '_')) { key << *c++; }
                    expect('\"');
                    skip_whitespace(c);
                    expect(':');
                    skip_whitespace(c);
                    JSON value = _load(c);
                    obj[key.str()] = value;
                    skip_whitespace(c);
                    if (*c != ',') {
                        expect('}');
                        return obj;
                    }
                    c++;
                }
            }
            default: {
                c--;
                if (std::isdigit(*c) || *c == '+' || *c == '-' || *c == '.') {
                    bool is_negative = false;
                    if (*c == '+') { c++; }
                    else if (*c == '-') { c++; is_negative = true; }

                    bool is_float = false;
                    int length = 0;
                    std::stringstream num_stream{};
                    while (std::isdigit(*c) || *c == '.') {
                        if (*c == '.') {
                            if (is_float) {
                                throw std::runtime_error("Bad JSON float value");
                            }
                            if (length == 0) {
                                throw std::runtime_error("No digits before JSON float decimal point");
                            }
                            num_stream << '.';
                            is_float = true;
                            length = 0;
                        }
                        else {
                            num_stream << *c;
                        }

                        c++;
                        length++;
                    }

                    if (is_float) {
                        if (length == 0) {
                            throw std::runtime_error("No digits after JSON float decimal point");
                        }

                        double value = std::stod(num_stream.str());
                        if (is_negative) {
                            value = -value;
                        }
                        return JSON(value);
                    }
                    else {
                        if (length == 0) {
                            throw std::runtime_error("Zero length integer literal");
                        }
                        int value = std::stoi(num_stream.str());
                        if (is_negative) {
                            value = -value;
                        }
                        return JSON(value);
                    }
                }
                std::stringstream err_stream{};
                err_stream << "Bad JSON: got " << *c;
                throw std::runtime_error(err_stream.str());
            }
        }
        throw std::runtime_error("End of stream before JSON was completed");
    }
};

}