#pragma once
// Build error if `s` is not a string literal.
#define STRING_LITERAL(s) "" s ""

// Makes a unique variable name for use in macros to prevent shadowing.
#define M_VAR(name) CONCAT2(MACRO_VAR_##name##_, __LINE__)

#define CONCAT2_(_1, _2) _1##_2
#define CONCAT2(_1, _2) CONCAT2_(_1, _2)
#define CONCAT3(_1, ...) CONCAT2(_1, CONCAT2(__VA_ARGS__))
#define CONCAT4(_1, ...) CONCAT2(_1, CONCAT3(__VA_ARGS__))
#define CONCAT5(_1, ...) CONCAT2(_1, CONCAT4(__VA_ARGS__))
#define CONCAT6(_1, ...) CONCAT2(_1, CONCAT5(__VA_ARGS__))
#define CONCAT7(_1, ...) CONCAT2(_1, CONCAT6(__VA_ARGS__))
#define CONCAT8(_1, ...) CONCAT2(_1, CONCAT7(__VA_ARGS__))
#define CONCAT9(_1, ...) CONCAT2(_1, CONCAT8(__VA_ARGS__))
