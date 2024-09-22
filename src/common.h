#ifndef COMMON_H
#define COMMON_H


#define fallthrough __attribute__((fallthrough))


#define noreturn __attribute__((noreturn))


#define ctz __builtin_ctz


#define expected(cond) __builtin_expect((cond), 1)
#define unexpected(cond) __builtin_expect((cond), 0)


#define haltandcatchfire __builtin_trap()

#define unreachable __builtin_unreachable()





#endif