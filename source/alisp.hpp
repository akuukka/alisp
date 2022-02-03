#pragma once
#ifndef ALISP_SINGLE_HEADER
#define ALISP_INLINE  
#endif

constexpr bool ConvertParsedNamesToUpperCase = true;
constexpr const char* NilName = ConvertParsedNamesToUpperCase ? "NIL" : "nil";
constexpr const char* TName = ConvertParsedNamesToUpperCase ? "T" : "t";
