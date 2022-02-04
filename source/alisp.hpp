#pragma once
#ifndef ALISP_SINGLE_HEADER
#define ALISP_INLINE  
#endif

constexpr bool ConvertParsedNamesToUpperCase = false;
constexpr const char* NilName = ConvertParsedNamesToUpperCase ? "NIL" : "nil";
constexpr const char* TName = ConvertParsedNamesToUpperCase ? "T" : "t";
