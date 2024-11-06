#ifndef HELPERS_H
#define HELPERS_H

#include <expected>
#include <print>
#include <format>
#include <source_location>
#include <utility>
#include <concepts>
#include <fstream>
#include <ostream>


#define FWD(x) std::forward<decltype(x)>(x)
#define restrict __restrict__




template <typename... Args>
[[noreturn]] void 
error(std::format_string<Args...> fmt, Args&&... args) {
    std::println(stderr, fmt, std::forward<Args>(args)...);

    exit(EXIT_FAILURE);
}

template <typename T>
[[noreturn]] void 
error(T&& str) {
    error("{}", std::forward<T>(str));
}


template <typename... Args>
[[noreturn]] inline void 
debug_error(std::format_string<Args...> fmt, Args&&... args, const std::source_location& loc = std::source_location::current()) {
    error("Error in {0} at line {1} [{2}]:\n{3}", 
    loc.file_name(), 
    loc.line(), 
    loc.function_name(), 
    std::format(fmt, std::forward<Args>(args)...));
}

template <typename T>
[[noreturn]] inline void 
debug_error(T&& str, const std::source_location& loc = std::source_location::current()) {
    error("Error in {0} at line {1} [{2}]:\n{3}", 
    loc.file_name(), 
    loc.line(), 
    loc.function_name(), 
    std::forward<T>(str));
}




// Assign the expected value to val or do something with the error value.
template <typename T, typename E, typename F>
void assign_or_else(const std::expected<T, E>& res, T& val, const F& fn) {
    if (res) {
        val = *res;
    } else {
        fn(res.error());
    }
}

template <typename T, typename E, typename F>
void assign_or_else(std::expected<T, E>&& res, T& val, const F& fn) {
    if (res) {
        val = *std::move(res);
    } else {
        fn(std::move(res).error());
    }
}


// Return expected value or do something with error value and return the result.
template <typename T, typename E, typename F>
T value_or_else(const std::expected<T, E>& res, const F& fn) {
    if (res) {
        return *res;
    } else {
        return fn(res.error());
    }
}

template <typename T, typename E, typename F>
T value_or_else(std::expected<T, E>&& res, const F& fn) {
    if (res) {
        return *std::move(res);
    } else {
        return fn(std::move(res).error());
    }
}





std::string readfile(const std::string& name) {
    std::ifstream f {name};
    std::ostringstream buf;
    buf << f.rdbuf();

    return buf.str();
}




#endif