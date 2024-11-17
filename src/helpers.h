#ifndef HELPERS_H
#define HELPERS_H

#include <expected>
#include <print>
#include <format>
#include <source_location>
#include <utility>
#include <type_traits>
#include <concepts>
#include <fstream>
#include <ostream>


#define FWD(x) std::forward<decltype(x)>(x)


// Helper for variants.
template<class... Fns>
struct overload : Fns... {using Fns::operator()...;};



// Helpers for detecting specializations.

template <typename T, template <typename...> class U>
struct is_specialization_of : std::false_type {};

template <typename... Ts, template <typename...> class U>
struct is_specialization_of<U<Ts...>, U> : std::true_type {};

template <typename T, template <typename...> class U>
constexpr inline bool is_specialization_of_v = is_specialization_of<T, U>::value;

template<typename T, template <typename...> class U>
concept specializes = is_specialization_of_v<T, U>;







template <typename T, typename... Us>
concept is_any_of = (std::same_as<T, Us> || ...);





// Call a lambda with certain arguments on each type.
template <typename... Ts>
struct for_each_type {
    template <typename Fn, typename... Args>
    static constexpr inline void run(Fn&& fn, Args&&... args) {
        (fn.template operator()<Ts>(std::forward<Args>(args)...), ...);
    }
};




template <typename... Ts>
struct type_tuple {
    template <std::size_t Index>
    using get = std::tuple_element_t<Index, std::tuple<Ts...>>;
};


template <typename... Ts, typename Fn, typename... Args>
constexpr inline void fett_helper_(type_tuple<Ts...> tup, Fn&& fn, Args&&... args) {
    fn.template operator()<Ts...>(std::forward<Args>(args)...);
}


template <typename... Tups>
requires (specializes<Tups, type_tuple> && ...)
struct for_each_type_tuple {
    template <typename Fn, typename... Args>
    static constexpr inline void run(Fn&& fn, Args&&... args) {
        (fett_helper_(Tups{}, fn, std::forward<Args>(args)...), ...);
    }
};









// Generic string-like type (char*, std::string, std::string_view, ...)
template<typename T>
concept StringLike = requires {
    std::is_convertible_v<T, std::string_view>;
};





template <typename... Args>
[[noreturn]] inline void 
error(std::format_string<Args...> fmt, Args&&... args) {
    std::println(stderr, fmt, std::forward<Args>(args)...);

    exit(EXIT_FAILURE);
}



template <typename... Args>
[[noreturn]] inline void 
debug_error_helper(const std::source_location& loc, std::format_string<Args...> fmt, Args&&... args) {
    error("Error in {0} at line {1} [{2}]:\n{3}", 
    loc.file_name(), 
    loc.line(), 
    loc.function_name(), 
    std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
[[noreturn]] inline void 
debug_error(std::format_string<Args...> fmt, Args&&... args, const std::source_location& loc = std::source_location::current()) {
    debug_error_helper(loc, fmt, std::forward<Args>(args)...);
}



template <typename... Args>
inline void
debug_warning_helper(const std::source_location& loc, std::format_string<Args...> fmt, Args&&... args) {
    std::println(stderr, "Warning: {0}\nIn {1}, line {2} [{3}]", 
    std::format(fmt, std::forward<Args>(args)...),
    loc.file_name(),
    loc.line(),
    loc.function_name());
}

template <typename... Args>
inline void
debug_warning(std::format_string<Args...> fmt, Args&&... args, const std::source_location& loc = std::source_location::current()) {
    debug_warning_helper(loc, fmt, std::forward<Args>(args)...);
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