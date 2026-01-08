#pragma once

#include <algorithm>
#include <utility>
#include <concepts>     // For C++20 concepts
#include "logging/log.h"

// https://github.com/CppCon/CppCon2015/blob/master/Presentations/Declarative%20Control%20Flow/Declarative%20Control%20Flow%20-%20Andrei%20Alexandrescu%20-%20CppCon%202015.pdf
/*
 * usage : SCOPE_EXIT { cleanup(); };
 */
namespace detail
{
  template <typename T> concept NoexceptInvocable = requires(T t)
  {
    {
      t()
    } noexcept;
  };
  // checks if the function is noexcept and can be called with no arguments (the lambda that will encase the { ON CLEANUP HERE }
  template <NoexceptInvocable Func>
  class ScopeExitGuard; // forward declaration
  struct ScopeExitHelper
  {};

  template <NoexceptInvocable Func>
  ScopeExitGuard<Func> operator+(ScopeExitHelper, Func&& fn) noexcept
  {
    return ScopeExitGuard<Func>(std::forward<Func>(fn));
  } //overloaded + operator to allow the syntax in the macro to work.
  // constructs the ScopeExitGuard by calling the private constructor (friend) by passing the lambda function to the class, making sure that it has no parameters and is noexcept
  template <NoexceptInvocable Func>
  class ScopeExitGuard
  {
    Func exitFunction;

    explicit ScopeExitGuard(Func&& func) noexcept : exitFunction(std::move(func)) {}

    template <NoexceptInvocable F>
    friend ScopeExitGuard<F> operator+(ScopeExitHelper, F&& fn) noexcept;

    public:
      ~ScopeExitGuard() noexcept { exitFunction(); }
  };
} // namespace detail

// These macros work together to create a unique variable name for the ScopeExitGuard object.
// This avoids name collisions if SCOPE_EXIT is used multiple times in the same scope.

// Step 2: The ## operator pastes s1 and s2 together into a single token (e.g., scopeExitGuard123).
// This happens *after* s1 and s2 have been potentially expanded by the CONCAT macro.
#define SCOPE_EXIT_CONCAT_IMPL(s1, s2) s1##s2

// Step 1: This macro forces the full expansion of its arguments (s1, s2) *before* passing them to CONCAT_IMPL.
// This is crucial if s2 is __LINE__ or __COUNTER__, ensuring the number is used, not the literal token name.
#define SCOPE_EXIT_CONCAT(s1, s2) SCOPE_EXIT_CONCAT_IMPL(s1, s2)

// Selects __COUNTER__ if available (preferred for uniqueness), otherwise falls back to __LINE__.
#ifdef __COUNTER__
#define SCOPE_EXIT_ANONYMOUS_VARIABLE(str) SCOPE_EXIT_CONCAT(str, __COUNTER__)
#else
#define SCOPE_EXIT_ANONYMOUS_VARIABLE(str) SCOPE_EXIT_CONCAT(str, __LINE__)
#endif
#define SCOPE_EXIT \
    auto SCOPE_EXIT_ANONYMOUS_VARIABLE(scopeExitGuard) \
    = ::detail::ScopeExitHelper() + [&]() noexcept -> void

#if !defined(NDEBUG) && (defined(DEBUG) || defined(_DEBUG) || defined(__DEBUG))
#define VERIFY(cond) \
  ( (cond) ? true : ([&]() -> bool { LOG_RUNTIME_METADATA(EngineLogLevel::Critical, __FILE__, __LINE__, __func__, "Assertion Failed: {}", #cond); assert(false); return false; }()) )
#define ASSERT(cond) \
  ( (cond) ? (void)0 : ([&]() { LOG_RUNTIME_METADATA(EngineLogLevel::Critical, __FILE__, __LINE__, __func__, "Assertion Failed: {}", #cond); assert(false); }()) )
#define ASSERT_MSG(cond, fmt, ...) \
  ( (cond) ? (void)0 : ([&]() { LOG_RUNTIME_METADATA(EngineLogLevel::Critical, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); assert(false); }()) )
#else
#define VERIFY(cond) (cond)
#define ASSERT(cond) ((void)0)
#define ASSERT_MSG(cond, fmt, ...) ((void)0)
#endif // End of conditional compilation

inline bool endsWith(const char* s, const char* part)
{
  const size_t sLength = strlen(s);
  const size_t partLength = strlen(part);
  return sLength < partLength ? false : strcmp(s + sLength - partLength, part) == 0;
}

inline std::string getBaseFilename(const char* full_path_cstr)
{
  const char* last_slash = std::strrchr(full_path_cstr, '/');
  const char* last_backslash = std::strrchr(full_path_cstr, '\\');
  const char* sep_ptr = nullptr;
  if (last_slash) { sep_ptr = last_slash; }
  if (last_backslash && last_backslash > sep_ptr) { sep_ptr = last_backslash; }
  const char* filename_start_ptr = sep_ptr ? sep_ptr + 1 : full_path_cstr;
  const char* last_dot_ptr = std::strrchr(full_path_cstr, '.');
  size_t length = last_dot_ptr - filename_start_ptr;
  return std::string(filename_start_ptr, length);
}

template <typename T>
void mergeVectors(std::vector<T>& v1, const std::vector<T>& v2) { v1.insert(v1.end(), v2.begin(), v2.end()); }

// From https://stackoverflow.com/a/64152990/1182653
// Delete a list of items from std::vector with indices in 'selection'
// e.g., eraseSelected({1, 2, 3, 4, 5}, {1, 3})  ->   {1, 3, 5}
//                         ^     ^    2 and 4 get deleted
template <typename T, typename Index = size_t>
void eraseSelected(std::vector<T>& v, const std::vector<Index>& selection)
{
  // cut off the elements moved to the end of the vector by std::stable_partition
  v.resize(std::distance(v.begin(),
                         // std::stable_partition moves any element whose index is in 'selection' to the end
                         std::stable_partition(v.begin(),
                                               v.end(),
                                               [&selection, &v](const T& item)
                                               {
                                                 return !std::binary_search(selection.begin(), selection.end(), static_cast<size_t>(static_cast<const T*>(&item) - &v[0]));
                                               })));
}

void inline saveStringList(FILE* f, const std::vector<std::string>& lines)
{
  uint32_t sz = (uint32_t)lines.size();
  fwrite(&sz, sizeof(uint32_t), 1, f);
  for (const std::string& s : lines)
  {
    sz = (uint32_t)s.length();
    fwrite(&sz, sizeof(uint32_t), 1, f);
    fwrite(s.c_str(), sz + 1, 1, f);
  }
}

void inline loadStringList(FILE* f, std::vector<std::string> lines)
{
  {
    uint32_t sz = 0;
    fread(&sz, sizeof(uint32_t), 1, f);
    lines.resize(sz);
  }
  std::vector<char> inBytes;
  for (std::string& s : lines)
  {
    uint32_t sz = 0;
    fread(&sz, sizeof(uint32_t), 1, f);
    inBytes.resize(sz + 1);
    fread(inBytes.data(), sz + 1, 1, f);
    s = std::string(inBytes.data());
  }
}

std::string inline lowercaseString(const std::string& string)
{
  std::string out(string.length(), ' ');
  std::transform(string.begin(), string.end(), out.begin(), tolower);
  return out;
}

inline std::string replaceAll(const std::string& str, const std::string& oldSubStr, const std::string& newSubStr)
{
  std::string result = str;

  for (size_t p = result.find(oldSubStr); p != std::string::npos; p = result.find(oldSubStr))
    result.replace(p, oldSubStr.length(), newSubStr);

  return result;
}

inline int addUnique(std::vector<std::string>& files, const std::string& file)
{
  if (file.empty())
    return -1;

  if (auto i = std::find(files.begin(), files.end(), file); i != files.end())
    return static_cast<int>(std::distance(files.begin(), i));

  files.push_back(file);
  return static_cast<int>(files.size()) - 1;
}
