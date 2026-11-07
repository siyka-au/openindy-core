#ifndef OI_VALUE_H_
#define OI_VALUE_H_

// A small dynamically typed value (bool, integer, number, text) for the few
// places where core and plugins exchange loosely typed data: transient
// attributes, a sensor's real-time reading, preview cells of an import.
#include <cstdint>
#include <map>
#include <string>
#include <variant>

#include "stringutil.h"

namespace oi {

class Value {
 public:
  Value() = default;  // invalid: holds nothing
  Value(bool v) : data(v) {}
  Value(int v) : data(static_cast<std::int64_t>(v)) {}
  Value(std::int64_t v) : data(v) {}
  Value(double v) : data(v) {}
  Value(std::string v) : data(std::move(v)) {}
  Value(const char* v) : data(std::string(v)) {}

  [[nodiscard]] bool isValid() const {
    return !std::holds_alternative<std::monostate>(this->data);
  }

  [[nodiscard]] bool isBool() const {
    return std::holds_alternative<bool>(this->data);
  }
  [[nodiscard]] bool isNumber() const {
    return std::holds_alternative<std::int64_t>(this->data) ||
           std::holds_alternative<double>(this->data);
  }
  [[nodiscard]] bool isString() const {
    return std::holds_alternative<std::string>(this->data);
  }

  //! false for an invalid value, zero, and the texts "", "0" and "false"
  [[nodiscard]] bool toBool() const {
    struct Visitor {
      bool operator()(std::monostate) const { return false; }
      bool operator()(bool v) const { return v; }
      bool operator()(std::int64_t v) const { return v != 0; }
      bool operator()(double v) const { return v != 0.0; }
      bool operator()(const std::string& v) const {
        return !v.empty() && v != "0" && toLower(v) != "false";
      }
    };
    return std::visit(Visitor{}, this->data);
  }

  //! 0 if the value is not a number or a text holding one
  [[nodiscard]] double toDouble() const {
    struct Visitor {
      double operator()(std::monostate) const { return 0.0; }
      double operator()(bool v) const { return v ? 1.0 : 0.0; }
      double operator()(std::int64_t v) const { return static_cast<double>(v); }
      double operator()(double v) const { return v; }
      double operator()(const std::string& v) const {
        return oi::toDouble(v).value_or(0.0);
      }
    };
    return std::visit(Visitor{}, this->data);
  }

  [[nodiscard]] int toInt() const {
    if (const auto* i = std::get_if<std::int64_t>(&this->data)) {
      return static_cast<int>(*i);
    }
    if (const auto* s = std::get_if<std::string>(&this->data)) {
      return oi::toInt(*s).value_or(0);
    }
    return static_cast<int>(this->toDouble());
  }

  //! the text; numbers as oi::number writes them, bool as "true"/"false"
  [[nodiscard]] std::string toString() const {
    struct Visitor {
      std::string operator()(std::monostate) const { return {}; }
      std::string operator()(bool v) const { return v ? "true" : "false"; }
      std::string operator()(std::int64_t v) const { return oi::number(v); }
      std::string operator()(double v) const { return oi::number(v); }
      std::string operator()(const std::string& v) const { return v; }
    };
    return std::visit(Visitor{}, this->data);
  }

  friend bool operator==(const Value&, const Value&) = default;

 private:
  std::variant<std::monostate, bool, std::int64_t, double, std::string> data;
};

//! Loosely typed named values: a sensor's real-time reading, and the
//! asynchronous sensor commands and their responses.
using ValueMap = std::map<std::string, Value>;

}  // namespace oi

#endif  // OI_VALUE_H_
