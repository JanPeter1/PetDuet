#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef constrain
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif

inline long& mockRandomOverrideRef() {
  static long val = -1;
  return val;
}

inline void setMockRandomValue(long val) {
  mockRandomOverrideRef() = val;
}

inline long random(long howsmall, long howbig) {
  if (howsmall >= howbig) return howsmall;
  long ov = mockRandomOverrideRef();
  if (ov >= 0) {
    return howsmall + (ov % (howbig - howsmall));
  }
  return howsmall + (std::rand() % (howbig - howsmall));
}

class String {
private:
  std::string s_;

public:
  String() = default;
  String(const char* c) : s_(c ? c : "") {}
  String(const std::string& str) : s_(str) {}

  int length() const { return static_cast<int>(s_.size()); }

  int indexOf(char ch, int fromIndex = 0) const {
    if (fromIndex < 0) fromIndex = 0;
    size_t pos = s_.find(ch, static_cast<size_t>(fromIndex));
    return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
  }

  String substring(int beginIndex, int endIndex) const {
    if (beginIndex < 0) beginIndex = 0;
    if (endIndex > static_cast<int>(s_.size())) endIndex = static_cast<int>(s_.size());
    if (beginIndex >= endIndex) return String("");
    return String(s_.substr(static_cast<size_t>(beginIndex), static_cast<size_t>(endIndex - beginIndex)));
  }

  String substring(int beginIndex) const {
    return substring(beginIndex, static_cast<int>(s_.size()));
  }

  void trim() {
    size_t start = 0;
    while (start < s_.size() && (s_[start] == ' ' || s_[start] == '\t' || s_[start] == '\r' || s_[start] == '\n')) {
      start++;
    }
    size_t end = s_.size();
    while (end > start && (s_[end - 1] == ' ' || s_[end - 1] == '\t' || s_[end - 1] == '\r' || s_[end - 1] == '\n')) {
      end--;
    }
    s_ = s_.substr(start, end - start);
  }

  bool startsWith(const char* prefix) const {
    if (!prefix) return false;
    size_t len = std::strlen(prefix);
    return s_.size() >= len && s_.compare(0, len, prefix) == 0;
  }

  void replace(const char* target, const char* replacement) {
    if (!target || !replacement) return;
    std::string tgt(target);
    std::string rep(replacement);
    if (tgt.empty()) return;
    size_t pos = 0;
    while ((pos = s_.find(tgt, pos)) != std::string::npos) {
      s_.replace(pos, tgt.length(), rep);
      pos += rep.length();
    }
  }

  long toInt() const {
    if (s_.empty()) return 0;
    return std::strtol(s_.c_str(), nullptr, 10);
  }

  bool operator==(const char* rhs) const {
    return rhs && s_ == rhs;
  }

  bool operator==(const String& rhs) const {
    return s_ == rhs.s_;
  }

  const char* c_str() const { return s_.c_str(); }
};

#endif // MOCK_ARDUINO_H
