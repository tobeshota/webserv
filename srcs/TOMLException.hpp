#pragma once

#include <stdexcept>
#include <string>

class TOMLException : public std::runtime_error {
 public:
  explicit TOMLException(const std::string& message, int line = -1)
      : std::runtime_error(createMessage(message, line)),
        m_message(message),
        m_line(line) {}

  virtual ~TOMLException() throw() {}

  const std::string& getMessage() const { return m_message; }
  int getLine() const { return m_line; }

 private:
  std::string createMessage(const std::string& message, int line) const {
    if (line >= 0) {
      return "TOML Error at line " + intToString(line) + ": " + message;
    }
    return "TOML Error: " + message;
  }

  std::string intToString(int n) const {
    std::string result;
    bool negative = false;

    if (n < 0) {
      negative = true;
      n = -n;
    } else if (n == 0) {
      return "0";
    }

    while (n > 0) {
      result = static_cast<char>('0' + (n % 10)) + result;
      n /= 10;
    }

    if (negative) {
      result = "-" + result;
    }
    return result;
  }

  std::string m_message;
  int m_line;
};
