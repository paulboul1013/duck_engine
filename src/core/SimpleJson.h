#pragma once
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace duck {

class JsonValue {
public:
    enum class Type { Null, Number, String, Object, Array, Bool };

    using Object = std::unordered_map<std::string, JsonValue>;
    using Array = std::vector<JsonValue>;

    JsonValue() = default;

    static JsonValue makeNumber(double value) {
        JsonValue json;
        json.m_type = Type::Number;
        json.m_number = value;
        return json;
    }

    static JsonValue makeString(std::string value) {
        JsonValue json;
        json.m_type = Type::String;
        json.m_string = std::move(value);
        return json;
    }

    static JsonValue makeObject(Object value) {
        JsonValue json;
        json.m_type = Type::Object;
        json.m_object = std::move(value);
        return json;
    }

    static JsonValue makeArray(Array value) {
        JsonValue json;
        json.m_type = Type::Array;
        json.m_array = std::move(value);
        return json;
    }

    static JsonValue makeBool(bool value) {
        JsonValue json;
        json.m_type = Type::Bool;
        json.m_bool = value;
        return json;
    }

    Type type() const { return m_type; }
    bool isObject() const { return m_type == Type::Object; }
    bool isArray() const { return m_type == Type::Array; }

    const Object& asObject() const {
        if (m_type != Type::Object) throw std::runtime_error("JSON value is not an object");
        return m_object;
    }

    const Array& asArray() const {
        if (m_type != Type::Array) throw std::runtime_error("JSON value is not an array");
        return m_array;
    }

    double asNumber() const {
        if (m_type != Type::Number) throw std::runtime_error("JSON value is not a number");
        return m_number;
    }

    const std::string& asString() const {
        if (m_type != Type::String) throw std::runtime_error("JSON value is not a string");
        return m_string;
    }

    bool asBool() const {
        if (m_type != Type::Bool) throw std::runtime_error("JSON value is not a bool");
        return m_bool;
    }

    const JsonValue& at(const std::string& key) const {
        const auto& object = asObject();
        auto it = object.find(key);
        if (it == object.end()) throw std::runtime_error("Missing JSON key: " + key);
        return it->second;
    }

    bool contains(const std::string& key) const {
        if (m_type != Type::Object) return false;
        return m_object.find(key) != m_object.end();
    }

    template <typename T>
    T value(const std::string& key, T fallback) const;

private:
    Type m_type = Type::Null;
    double m_number = 0.0;
    std::string m_string;
    Object m_object;
    Array m_array;
    bool m_bool = false;
};

template <>
inline float JsonValue::value<float>(const std::string& key, float fallback) const {
    if (!contains(key)) return fallback;
    return static_cast<float>(at(key).asNumber());
}

template <>
inline int JsonValue::value<int>(const std::string& key, int fallback) const {
    if (!contains(key)) return fallback;
    return static_cast<int>(at(key).asNumber());
}

template <>
inline std::string JsonValue::value<std::string>(const std::string& key, std::string fallback) const {
    if (!contains(key)) return fallback;
    return at(key).asString();
}

class JsonParser {
public:
    explicit JsonParser(std::string text)
        : m_text(std::move(text)) {}

    JsonValue parse() {
        skipWhitespace();
        JsonValue value = parseValue();
        skipWhitespace();
        if (!eof()) throw std::runtime_error("Unexpected trailing JSON content");
        return value;
    }

private:
    JsonValue parseValue() {
        skipWhitespace();
        if (eof()) throw std::runtime_error("Unexpected end of JSON");

        char c = peek();
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') return JsonValue::makeString(parseString());
        if (c == 't') return parseTrue();
        if (c == 'f') return parseFalse();
        if (c == 'n') return parseNull();
        return JsonValue::makeNumber(parseNumber());
    }

    JsonValue parseObject() {
        expect('{');
        JsonValue::Object object;
        skipWhitespace();
        if (peek() == '}') {
            advance();
            return JsonValue::makeObject(std::move(object));
        }

        while (true) {
            skipWhitespace();
            std::string key = parseString();
            skipWhitespace();
            expect(':');
            skipWhitespace();
            object[key] = parseValue();
            skipWhitespace();
            if (peek() == '}') {
                advance();
                break;
            }
            expect(',');
        }
        return JsonValue::makeObject(std::move(object));
    }

    JsonValue parseArray() {
        expect('[');
        JsonValue::Array array;
        skipWhitespace();
        if (peek() == ']') {
            advance();
            return JsonValue::makeArray(std::move(array));
        }

        while (true) {
            skipWhitespace();
            array.push_back(parseValue());
            skipWhitespace();
            if (peek() == ']') {
                advance();
                break;
            }
            expect(',');
        }
        return JsonValue::makeArray(std::move(array));
    }

    std::string parseString() {
        expect('"');
        std::string out;
        while (!eof()) {
            char c = advance();
            if (c == '"') return out;
            if (c == '\\') {
                if (eof()) throw std::runtime_error("Invalid JSON escape");
                char escaped = advance();
                switch (escaped) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    default: throw std::runtime_error("Unsupported JSON escape");
                }
            } else {
                out.push_back(c);
            }
        }
        throw std::runtime_error("Unterminated JSON string");
    }

    double parseNumber() {
        const char* start = m_text.c_str() + m_pos;
        char* end = nullptr;
        double value = std::strtod(start, &end);
        if (end == start) throw std::runtime_error("Invalid JSON number");
        m_pos = static_cast<size_t>(end - m_text.c_str());
        return value;
    }

    JsonValue parseTrue() {
        expectSequence("true");
        return JsonValue::makeBool(true);
    }

    JsonValue parseFalse() {
        expectSequence("false");
        return JsonValue::makeBool(false);
    }

    JsonValue parseNull() {
        expectSequence("null");
        return JsonValue();
    }

    void skipWhitespace() {
        while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) {
            ++m_pos;
        }
    }

    void expect(char c) {
        if (eof() || peek() != c) {
            throw std::runtime_error(std::string("Expected JSON character: ") + c);
        }
        advance();
    }

    void expectSequence(const char* sequence) {
        while (*sequence) {
            expect(*sequence);
            ++sequence;
        }
    }

    char peek() const {
        return m_text[m_pos];
    }

    char advance() {
        return m_text[m_pos++];
    }

    bool eof() const {
        return m_pos >= m_text.size();
    }

    std::string m_text;
    size_t m_pos = 0;
};

} // namespace duck
