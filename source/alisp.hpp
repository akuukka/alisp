#pragma once
#ifndef ALISP_SINGLE_HEADER
#define ALISP_INLINE  
#define ALISP_STATIC static
#else
#define ALISP_STATIC inline
#endif

constexpr bool ConvertParsedNamesToUpperCase = false;
constexpr const char* NilName = ConvertParsedNamesToUpperCase ? "NIL" : "nil";
constexpr const char* TName = ConvertParsedNamesToUpperCase ? "T" : "t";
constexpr const char* RestName = ConvertParsedNamesToUpperCase ? "&REST" : "&rest";
constexpr const char* OptionalName = ConvertParsedNamesToUpperCase ? "&OPTIONAL" : "&optional";
constexpr const char* MacroName = ConvertParsedNamesToUpperCase ? "MACRO" : "macro";
constexpr const char* LambdaName = ConvertParsedNamesToUpperCase ? "LAMBDA" : "lambda";
constexpr const char* ListpName = ConvertParsedNamesToUpperCase ? "LISTP" : "listp";
