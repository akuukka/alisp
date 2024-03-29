#define ENABLE_DEBUG_REFCOUNTING
#include <exception>
#include <typeinfo>
#include <chrono>
#include "Error.hpp"
#include "StreamObject.hpp"
#include <variant>
#include "UTF8.hpp"
#include "alisp.hpp"
#include <sstream>
#include <fstream>
#include <ios>
#ifdef ALISP_SINGLE_HEADER
#include "ALisp_SingleHeader.hpp"
#else
#include "Machine.hpp"
#endif
#include <cassert>
#include <cstdint>
#include <set>
#include <string>
#include "ValueObject.hpp"
#include "ConsCellObject.hpp"

using namespace alisp;

void ASSERT_EQ(std::string a, std::string b)
{
    std::string oa, ob;
    oa = a;
    ob = b;
    if (ConvertParsedNamesToUpperCase) {
        a = utf8::toUpper(a);
        b = utf8::toUpper(b);
    }

    if (a == b) {
        return;
    }
    std::cerr << "Was expecting " << ob << " but got " << oa << std::endl;
    assert(false);
}

void ASSERT_EQ(std::string a, const char* b)
{
    ASSERT_EQ(a, std::string(b));
}

void ASSERT_EQ(String a, const char* b)
{
    if (a == b) {
        return;
    }
    std::cerr << "Was expecting " << b << " but got " << a << std::endl;
    assert(false);
}

void ASSERT_EQ(const std::unique_ptr<alisp::Object>& a, std::string b)
{
    ASSERT_EQ(a->toString(), b);
}

void ASSERT_OUTPUT_EQ(alisp::Machine& m, const char* expr, std::string res)
{
    try {
        std::string out = m.evaluate(expr)->toString();
        std::string oout, ores;
        oout = out;
        ores = res;
        if (ConvertParsedNamesToUpperCase) {
            out = utf8::toUpper(out);
            res = utf8::toUpper(res);
        }
        if (out != res) {
            std::cerr << "Expected '" << expr << "' to output '" << ores << "' but ";
            std::cerr << " got '" << oout << "' instead.\n";
            exit(1);
        }
    }
    catch (std::runtime_error& ex) {
        throw;
    }
}

void ASSERT_OUTPUT_CONTAINS(alisp::Machine& m, const char* expr, std::string res)
{
    std::string out = m.evaluate(expr)->toString();
    if (ConvertParsedNamesToUpperCase) {
        out = utf8::toUpper(out);
        res = utf8::toUpper(res);
    }
    if (out.find(res) == std::string::npos) {
        std::cerr << "Expected the output of '" << expr << "' to contain '" << res << "' but ";
        std::cerr << " it didn't. The output was '" << out << "'.\n";
        exit(1);
    }
}

#define ASSERT_EXCEPTION(m, expr, exception)                            \
{                                                                       \
    bool ok = false;                                                    \
    try {                                                               \
        (m).evaluate((expr));                                           \
    }                                                                   \
    catch (exception & ex) {                                            \
        ok = true;                                                      \
    }                                                                   \
    if (!ok) {                                                           \
        std::cerr << "Expected '" << expr << "' to throw ";             \
        std::cerr << #exception << "\n";                                \
        exit(1);                                                        \
    }                                                                   \
}

void TEST_CODE(Machine& m, std::string code)
{
    while (code.size() && (code.front() == '\n' || code.front() == ' ')) {
        code = code.substr(1);
    }
    for (;;) {
        auto it = code.find("=>");
        if (it == std::string::npos) {
            m.evaluate(code.c_str());
            break;
        }
        auto expr = code.substr(0, it - 1);
        auto newline = code.find('\n', it + 1);
        std::string res;
        if (newline == std::string::npos) {
            res = code.substr(it + 2);
        }
        else {
            res = code.substr(it + 2, newline - (it + 2));
        }
        while (res[0] == ' ') {
            res = res.substr(1);
        }
        while (res.size() && res.back() == ' ') {
            res.pop_back();
        }
        ASSERT_OUTPUT_EQ(m, expr.c_str(), res);
        if (newline == std::string::npos) {
            break;
        }
        code = code.substr(newline + 1);
    }
}

void testRemqFunction()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "(remq 'a nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(remq 'a (list 'a))", "nil");
    ASSERT_OUTPUT_EQ(m, "(setq sample-list (list 'a 'b 'c 'a 'b 'c))", "(a b c a b c)");
    ASSERT_OUTPUT_EQ(m, "(remq 'a sample-list)", "(b c b c)");
    ASSERT_OUTPUT_EQ(m, "(remq 1 '(1 2 3))", "(2 3)");
    ASSERT_EXCEPTION(m, "(remq 1 '(1 2 . 3))", exceptions::WrongTypeArgument);
}

void testDelqFunction()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "(setq sample-list (list 'a 'b 'c '(4)))", "(a b c (4))");
    ASSERT_OUTPUT_EQ(m, "(delq 'a sample-list)", "(b c (4))");
    ASSERT_OUTPUT_EQ(m, "sample-list", "(a b c (4))");
    ASSERT_OUTPUT_EQ(m, "(delq 'c sample-list)", "(a b (4))");
    ASSERT_OUTPUT_EQ(m, "sample-list", "(a b (4))");

    ASSERT_OUTPUT_EQ(m, "(setq sample-list2 (list 'a 'a 'a 'a 'b 'c '(4)))", "(a a a a b c (4))");
    ASSERT_OUTPUT_EQ(m, "(delq 'a sample-list2)", "(b c (4))");
    ASSERT_OUTPUT_EQ(m, "sample-list2", "(a a a a b c (4))");

    ASSERT_OUTPUT_EQ(m, "(setq sample-list3 (list 1 2 1 3 1 1))", "(1 2 1 3 1 1)");
    ASSERT_OUTPUT_EQ(m, "(delq 1 sample-list3)", "(2 3)");
    ASSERT_OUTPUT_EQ(m, "sample-list3", "(1 2 3)");

    // A dotted list causes an error, but not pre-emptively:
    ASSERT_OUTPUT_EQ(m, "(setq sample-list4 (cons 3 (cons 4 (cons 1 2)))  )", "(3 4 1 . 2)");
    ASSERT_EXCEPTION(m, "(delq 4 sample-list4)", exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "sample-list4", "(3 1 . 2)");

    ASSERT_OUTPUT_EQ(m, "(delq 1 '(1 1 1 1))", "nil");
}

void testListBasics()
{
    Machine m;
    ListBuilder builder(m);
    builder.append(m.makeTrue());
    builder.append(m.makeNil());
    ASSERT_EQ(builder.get()->toString(), "(t nil)");
    
    ASSERT_OUTPUT_EQ(m, "(memq 2 nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(memq 2 '(1 2 3 4 . 5))", "(2 3 4 . 5)");
    ASSERT_EXCEPTION(m, "(memq 6 '(1 2 3 4 . 5))", exceptions::WrongTypeArgument);
    ASSERT_EXCEPTION(m, "(memq 6 123)", exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(append '(1 2) '(3 4 . 5))", "(1 2 3 4 . 5)");
    ASSERT_OUTPUT_EQ(m, "(append '(1 2 3 4) '(5 6 7 8))", "(1 2 3 4 5 6 7 8)");
    ASSERT_OUTPUT_EQ(m, "(make-list 3 'pigs)", "(pigs pigs pigs)");
    ASSERT_OUTPUT_EQ(m, "(make-list 0 'pigs)", "nil");
    ASSERT_OUTPUT_EQ(m, "(setq l (make-list 3 '(a b)))", "((a b) (a b) (a b))");
    ASSERT_OUTPUT_EQ(m, "(eq (car l) (cadr l))", "t");
    ASSERT_OUTPUT_EQ(m, "(listp (quote nil))", "t");
    ASSERT_OUTPUT_EQ(m, "(listp nil)", "t");
    ASSERT_OUTPUT_EQ(m, "(listp 'nil)", "t");
    ASSERT_OUTPUT_EQ(m, "'(1 2 . 3)", "(1 2 . 3)");
    ASSERT_OUTPUT_EQ(m, "()", "nil");
    ASSERT_OUTPUT_EQ(m, "'(1)", "(1)"); 
    ASSERT_OUTPUT_EQ(m, "'(1 2 3)", "(1 2 3)"); 
    ASSERT_OUTPUT_EQ(m, "(consp '(1 2))", "t"); 
    ASSERT_OUTPUT_EQ(m, "(consp 1)", "nil"); 
    ASSERT_OUTPUT_EQ(m, "(consp nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(atom '(1 2))", "nil"); 
    ASSERT_OUTPUT_EQ(m, "(atom 1)", "t"); 
    ASSERT_OUTPUT_EQ(m, "(atom nil)", "t");
    ASSERT_OUTPUT_EQ(m, "(listp '(1 2))", "t"); 
    ASSERT_OUTPUT_EQ(m, "(listp 1)", "nil"); 
    ASSERT_OUTPUT_EQ(m, "(listp '())", "t");
    ASSERT_OUTPUT_EQ(m, "(listp ())", "t");
    ASSERT_OUTPUT_EQ(m, "(nlistp 1)", "t"); 
    ASSERT_OUTPUT_EQ(m, "(nlistp nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(nlistp '())", "nil");
    ASSERT_OUTPUT_EQ(m, "(nlistp ())", "nil");
    ASSERT_OUTPUT_EQ(m, "'(a . b)", "(a . b)");
    ASSERT_OUTPUT_EQ(m, "(car '(a . b))", "a");
    ASSERT_OUTPUT_EQ(m, "(cdr '(a . b))", "b");
    ASSERT_OUTPUT_EQ(m, "(consp '(a . b))", "t");
    ASSERT_OUTPUT_EQ(m, "(car '(a b . c))", "a");
    ASSERT_OUTPUT_EQ(m, "(cdr '(a b . c))", "(b . c)");
    ASSERT_OUTPUT_EQ(m, "(proper-list-p 1)", "nil");
    ASSERT_OUTPUT_EQ(m, "(proper-list-p nil)", "0");
    ASSERT_OUTPUT_EQ(m, "(proper-list-p '(1 2 3 4))", "4");
    ASSERT_OUTPUT_EQ(m, "(proper-list-p '(a b . c))", "nil");
    ASSERT_OUTPUT_EQ(m, "(progn (setq x '(\"a\" \"b\")) (setq y (cons x x))"
                     "(eq (car (car y)) (car (cdr y))))", "t");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 'y)", "((\"a\" \"b\") \"a\" \"b\")");
    ASSERT_OUTPUT_EQ(m, "(length '(1 2 3 4))", "4");
    ASSERT_OUTPUT_EQ(m, "(length '(1))", "1");
    ASSERT_OUTPUT_EQ(m, "(length nil)", "0");
    ASSERT_OUTPUT_EQ(m, "(length ())", "0");
    ASSERT_OUTPUT_EQ(m, "(setq x1 (list 'a 'b 'c))", "(a b c)");
    ASSERT_OUTPUT_EQ(m, "(setq x2 (cons 'z (cdr x1)))", "(z b c)");
    ASSERT_OUTPUT_EQ(m, "(setcar (cdr x1) 'foo)", "foo");
    ASSERT_OUTPUT_EQ(m, "x1", "(a foo c)");
    ASSERT_OUTPUT_EQ(m, "x2", "(z foo c)");
    ASSERT_OUTPUT_EQ(m, "(prog2 1 2 (setq p3 3))", "2");
    ASSERT_OUTPUT_EQ(m, "(prog2 (setq p1 4) 2 (setq p3 3))", "2");
    ASSERT_OUTPUT_EQ(m, "(= (+ p1 p3) 7)", "t");
    ASSERT_EXCEPTION(m, "(setcar nil 4)", exceptions::WrongTypeArgument);

    ASSERT_OUTPUT_EQ(m, "(setq *some-list* (list* 'one 'two 'three 'four))",
                     "(one two three . four)");
    ASSERT_OUTPUT_EQ(m, "(rplaca *some-list* 'uno)", "(uno two three . four)");
    ASSERT_OUTPUT_EQ(m, "(rplacd (last *some-list*) (list 'iv))", "(three iv)");
    ASSERT_OUTPUT_EQ(m, "*some-list*", "(uno two three iv)");

    TEST_CODE(m, R"code((nconc) => nil
(nconc nil nil) => nil
(nconc '(1 2 3)) => (1 2 3)
(nconc nil '(1 2 3)) => (1 2 3)
(nconc "can be anything") => "can be anything"
(setq x '(a b c)) => (a b c)
(setq y '(d e f)) => (d e f)
(nconc x y) =>  (a b c d e f)
x =>  (a b c d e f)

(setq foo (list 'a 'b 'c 'd 'e)
       bar (list 'f 'g 'h 'i 'j)
       baz (list 'k 'l 'm)) =>  (k l m)
 (setq foo (nconc foo bar baz)) => (a b c d e f g h i j k l m)
 foo => (a b c d e f g h i j k l m)
 bar => (f g h i j k l m)
 baz => (k l m)

 (setq foo (list 'a 'b 'c 'd 'e)
       bar (list 'f 'g 'h 'i 'j)
       baz (list 'k 'l 'm)) =>  (k l m)
 (setq foo (nconc nil foo bar nil baz)) => (a b c d e f g h i j k l m) 
 foo => (a b c d e f g h i j k l m)
 bar => (f g h i j k l m)
 baz => (k l m)
)code");

    TEST_CODE(m, R"code(
(list-length '(a b c d)) =>  4
(list-length '(a (b c) d)) =>  3
(list-length '()) =>  0
(list-length nil) =>  0
(defun circular-list (&rest elems)
 (let ((cycle (copy-list elems))) 
   (nconc cycle cycle)))
(list-length (circular-list 'a 'b)) => nil
(list-length (circular-list 'a)) => nil
(circular-list)
(list-length (circular-list)) => 0
)code");
}

template <typename E>
bool expect(std::function<void(void)> code)
{
    try {
        code();
    }
    catch (E&) {
        return true;
    }
    return false;
}

void testNullFunction()
{
    Machine m;
    assert(m.evaluate("(null nil)")->toString() == m.parsedSymbolName("t"));
    assert(m.evaluate("(null ())")->toString() == m.parsedSymbolName("t"));
    assert(expect<alisp::exceptions::VoidFunction>([&]() {
        m.evaluate("(null (test))");
    }));
    assert(expect<alisp::exceptions::WrongNumberOfArguments>([&]() { m.evaluate("(null)"); }));
    assert(expect<alisp::exceptions::WrongNumberOfArguments>([&]() { m.evaluate("(null 1 2)"); }));
    assert(m.evaluate("(null '(1))")->toString() == NilName);
    assert(m.evaluate("(null '())")->toString() == TName);

    assert(m.evaluate("(null (null t))")->toString() == TName);
    assert(m.evaluate("(null (null (null nil)))")->toString() == TName);
}

void testQuote()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "'()", "nil");
    ASSERT_OUTPUT_EQ(m, "'(1 2 3)", "(1 2 3)");
    ASSERT_OUTPUT_EQ(m, "(quote (+ 1 2))", "(+ 1 2)");
    ASSERT_OUTPUT_EQ(m, "(quote foo)", "foo");
    ASSERT_OUTPUT_EQ(m, "'foo", "foo");
    ASSERT_OUTPUT_EQ(m, "''foo", "'foo");
    ASSERT_OUTPUT_EQ(m, "'(quote foo)", "'foo");
    ASSERT_OUTPUT_EQ(m, "'(a ,b,c)", "(a (, b) (, c))");
    ASSERT_OUTPUT_EQ(m, "`(a b)", "(a b)");
    ASSERT_OUTPUT_EQ(m, "`(a ,(+ 1 2))", "(a 3)");
    ASSERT_OUTPUT_EQ(m, "`(1 2 (3 ,(+ 4 5)))", "(1 2 (3 9))");
    ASSERT_OUTPUT_EQ(m, "(progn (setq some-list '(2 3)) `(1 ,@some-list 4 ,@some-list) )",
                     "(1 2 3 4 2 3)");
    ASSERT_OUTPUT_EQ(m, "`(1 2 ,@() 3)", "(1 2 3)");
    ASSERT_OUTPUT_EQ(m, "`(1 2 ,() 3)", "(1 2 nil 3)");
}

void testCarFunction()
{
    Machine m;
    ASSERT_EXCEPTION(m, "(car 1)", exceptions::WrongTypeArgument);
    ASSERT_EXCEPTION(m, "(car (+ 1 1))", exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(car nil)", NilName);
    ASSERT_OUTPUT_EQ(m, "(car ())",NilName);
    ASSERT_OUTPUT_EQ(m, "(car '())",NilName);
    ASSERT_OUTPUT_EQ(m, "(car '(1 2))","1");
    ASSERT_EXCEPTION(m, "(car (1 2))", exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(car '(1 2))","1");
    ASSERT_OUTPUT_EQ(m, "(car '((1 2)))","(1 2)");
    ASSERT_OUTPUT_EQ(m, "(setq test (list 'a 'b' c))", "(a b c)");
    ASSERT_OUTPUT_EQ(m, "(setcar test 'd)", "d");
    ASSERT_OUTPUT_EQ(m, "test", "(d b c)");
}

void testSyntaxErrorDetection()
{
    Machine m;
    assert(expect<alisp::exceptions::SyntaxError>([&]() { m.evaluate("(car"); }));
}

void testStrings()
{
    Machine m;
    for (auto ch : String("")) {
        assert(false && "Iterating empty string!");
    }
    String s1("abba");
    ASSERT_EQ(std::string("").substr(0), "");
    ASSERT_EQ(String("").substr(0, 0), "");
    ASSERT_OUTPUT_EQ(m, "(characterp (max-char))", "t");
    ASSERT_OUTPUT_EQ(m, "(characterp (1+ (max-char)))", "nil");
    ASSERT_EQ(s1.substr(2), "ba");
    ASSERT_EQ(String("AbbaBaba").substr(4), "Baba");
    ASSERT_EQ(String("アブラカダブラ").substr(0), "アブラカダブラ");
    ASSERT_EQ(String("アブラカダブラ").substr(2), "ラカダブラ");
    ASSERT_EQ(String("アブラカダブラ").substr(3, 2), "カダ");
    std::set<std::uint32_t> chars = {49,50,51,12472};
    for (auto ch : String("12ジ3")) {
        chars.erase(ch);
    }
    assert(chars.empty());
    
    ASSERT_OUTPUT_EQ(m, "(substring \"abcdefg\" 2)", "\"cdefg\"");
    ASSERT_EXCEPTION(m, "(substring \"abcdefg\" 2.0)", alisp::exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(substring \"abcdefg\" 0 3)", "\"abc\"");
    ASSERT_OUTPUT_EQ(m, "(substring \"abcdefg\")", "\"abcdefg\"");
    ASSERT_OUTPUT_EQ(m, "(substring \"abcdefg\" -3 -1)", "\"ef\"");
    ASSERT_OUTPUT_EQ(m, "(substring \"abcdefg\" -3 nil)", "\"efg\"");
    ASSERT_OUTPUT_EQ(m, "(concat \"ab\" \"cd\")", "\"abcd\"");
    ASSERT_OUTPUT_EQ(m, "(length \"abc\")", "3");
    ASSERT_OUTPUT_EQ(m, "(char-or-string-p (elt \"abc\" 0))", "t");
    ASSERT_OUTPUT_EQ(m, "(char-or-string-p \"abc\")", "t");
    ASSERT_OUTPUT_EQ(m, "(char-or-string-p 1)", "t");
    ASSERT_OUTPUT_EQ(m, "(char-or-string-p 100000000)", "nil");
    ASSERT_OUTPUT_EQ(m, "(string ?a ?b ?c)", "\"abc\"");
    ASSERT_OUTPUT_EQ(m, "(string)", "\"\"");
    ASSERT_OUTPUT_EQ(m, "(split-string \"  two words \")", "(\"two\" \"words\")");
    ASSERT_OUTPUT_EQ(m, "(split-string \"  two words \" \"[ ]+\")",
                     "(\"\" \"two\" \"words\" \"\")");
    ASSERT_OUTPUT_EQ(m, "(split-string \"Soup is good food\" \"o\")",
                     R"code(("S" "up is g" "" "d f" "" "d"))code");
    ASSERT_OUTPUT_EQ(m, "(split-string \"Soup is good food\" \"o\" t)",
                     R"code(("S" "up is g" "d f" "d"))code");
    ASSERT_OUTPUT_EQ(m, "(split-string \"Soup is good food\" \"o+\")",
                     R"code(("S" "up is g" "d f" "d"))code");
    ASSERT_OUTPUT_EQ(m,
                     R"code((split-string "aooob" "o*"))code",
                     R"code(("" "a" "" "b" ""))code");
    ASSERT_OUTPUT_EQ(m,
                     R"code((split-string "ooaboo" "o*"))code",
                     R"code(("" "" "a" "b" ""))code");
    ASSERT_OUTPUT_EQ(m,
                     R"code((split-string "" ""))code",
                     R"code((""))code");
    ASSERT_OUTPUT_EQ(m,
                     R"code((split-string "Soup is good food" "o*" t))code",
                     R"code(("S" "u" "p" " " "i" "s" " " "g" "d" " " "f" "d"))code");
    ASSERT_OUTPUT_EQ(m,
                     R"code((split-string "Nice doggy!" "" t))code",
                     R"code(("N" "i" "c" "e" " " "d" "o" "g" "g" "y" "!"))code");
    ASSERT_OUTPUT_EQ(m,
                     R"code((split-string "" "" t))code",
                     R"code(nil)code");
    ASSERT_OUTPUT_EQ(m,
                     R"code((split-string "ooo" "o*" t))code",
                     R"code(nil)code");
    ASSERT_OUTPUT_EQ(m, "(stringp (car '(\"a\")))", "t");
    ASSERT_OUTPUT_EQ(m, "(stringp \"abc\")", "t");
    ASSERT_OUTPUT_EQ(m, "(stringp 1)", "nil");
    ASSERT_OUTPUT_EQ(m, "(stringp ())", "nil");
    ASSERT_OUTPUT_EQ(m, R"code((format "%c" 65))code", R"code("A")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%c" 12472))code", R"code("ジ")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "test"))code", R"code("test")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "a%%b"))code", R"code("a%b")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%d" 15))code", R"code("15")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%5d" 15))code", R"code("   15")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%015d" 30))code", R"code("000000000000030")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%0000015d" 30))code", R"code("000000000000030")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%05d" -30))code", R"code("-0030")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%5d" -30))code", R"code("  -30")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%s" "cabra"))code", R"code("cabra")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%S" "cabra"))code", R"code(""cabra"")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "num: %d.%%" 50))code", R"code("num: 50.%")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%+d" 15))code", R"code("+15")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%+d" -15))code", R"code("-15")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%+d" 0))code", R"code("+0")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%++++d" 15))code", R"code("+15")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%+05d" 15))code", R"code("+0015")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%+6d" 15))code", R"code("   +15")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%+2d" 155))code", R"code("+155")code");
    ASSERT_OUTPUT_EQ(m,
                     R"code(
(progn
 (setq str "abc")
 (store-substring str 0 "A"))
)code",
                     R"code("Abc")code");
    ASSERT_OUTPUT_EQ(m, R"code((store-substring str 1 "B"))code", R"code("ABc")code");
    ASSERT_OUTPUT_EQ(m, R"code((store-substring str 2 "C"))code", R"code("ABC")code");
    ASSERT_EXCEPTION(m, R"code((store-substring str 3 "D"))code", alisp::exceptions::Error);
    ASSERT_EXCEPTION(m, R"code((store-substring str 2 ?ジ))code", alisp::exceptions::Error);
    ASSERT_EXCEPTION(m, R"code((store-substring str 1 ?ジ))code", alisp::exceptions::Error);
    ASSERT_OUTPUT_EQ(m, R"code((store-substring str 0 ?ジ))code", R"code("ジ")code");
    ASSERT_EXCEPTION(m, R"code((store-substring str 4 ?d))code", alisp::exceptions::Error);
    ASSERT_EXCEPTION(m, R"code((store-substring str -1 ?d))code", alisp::exceptions::Error);
    ASSERT_EXCEPTION(m, R"code((store-substring str -1 "abc"))code", alisp::exceptions::Error);
    ASSERT_OUTPUT_EQ(m, R"code((char-equal 65 ?A))code", R"code(t)code");
    ASSERT_OUTPUT_EQ(m, R"code((char-equal ?x ?b))code", R"code(nil)code");
    ASSERT_OUTPUT_EQ(m, R"code((char-to-string 12472))code", R"code("ジ")code");
    ASSERT_OUTPUT_EQ(m, R"code((char-equal 12472 ?ジ))code", R"code(t)code");
    ASSERT_OUTPUT_EQ(m, R"code((length "aジb"))code", R"code(3)code");
    ASSERT_OUTPUT_EQ(m, R"code((string-bytes "aジb"))code", R"code(5)code");
    ASSERT_OUTPUT_EQ(m, R"code((elt "aジb" 2))code", R"code(98)code");
    ASSERT_OUTPUT_EQ(m, R"code((reverse "ABCDジEFG"))code", R"code("GFEジDCBA")code");
    ASSERT_OUTPUT_EQ(m, R"code((elt "aジb" 1))code", R"code(12472)code");
    ASSERT_EXCEPTION(m, R"code((elt "aジb" 3))code", exceptions::Error);
    ASSERT_EXCEPTION(m, R"code((elt "" 0))code", exceptions::Error);
    ASSERT_OUTPUT_EQ(m, R"code((make-string 5 (elt "aジb" 1)))code", R"code("ジジジジジ")code");
    ASSERT_OUTPUT_EQ(m, R"code((make-string 2 ?\n))code", "\"\n\n\"");
    ASSERT_OUTPUT_EQ(m, R"code((make-string 4 ?\s))code", R"code("    ")code");
    ASSERT_OUTPUT_EQ(m, R"code((make-string 4 ?\\))code", R"code("\\\\")code");
    /*
    ASSERT_OUTPUT_EQ(m, R"code((format t "format t"))code", R"code(nil)code");
    ASSERT_OUTPUT_EQ(m, R"code((format nil "format nil"))code", R"code("format nil")code");
    ASSERT_OUTPUT_EQ(m, R"code((format *standard-output* "format stdout"))code", R"code(nil)code");
    */
    ASSERT_OUTPUT_EQ(m, R"code((force-output *standard-output*))code", "nil");
    ASSERT_OUTPUT_EQ(m, R"code((force-output *query-io*))code", "nil");
    ASSERT_OUTPUT_EQ(m, R"code((parse-integer "1"))code", "1");
    ASSERT_OUTPUT_EQ(m, R"code((parse-integer "123"))code", "123");
    ASSERT_OUTPUT_EQ(m, R"code((string-to-number "3"))code", "3");
    ASSERT_OUTPUT_CONTAINS(m, R"code((string-to-number "3.5"))code", "3.5");
    ASSERT_OUTPUT_EQ(m, R"code((string-to-number "X256"))code", "0");
    ASSERT_OUTPUT_EQ(m, R"code("2\"5\"6")code", R"code("2"5"6")code");
}

void testDivision()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "(/ 10 2)", "5");
    // Same as emacs: one float arg means that also preceding integer divisions
    // are handled as floating point operations.
    assert(std::abs(m.evaluate("(/ 10 3 3.0)")->value<double>() - 1.11111111) < 0.001);
    ASSERT_EXCEPTION(m, "(/ 1 0)", exceptions::Error);
}

void testCdrFunction()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "(cdr '(a b c))", "(b c)");
    ASSERT_OUTPUT_EQ(m, "(cdr '(a))", "nil");
    ASSERT_OUTPUT_EQ(m, "(cdr '())", "nil");
    ASSERT_OUTPUT_EQ(m, "(cdr (cdr '(a b c)))", "(c)");
    ASSERT_EXCEPTION(m, "(cdr 1)", exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(cdr-safe '(a b c))", "(b c)");
    ASSERT_OUTPUT_EQ(m, "(nthcdr 2 '(pine fir oak maple))", "(oak maple)");
    ASSERT_OUTPUT_EQ(m, "(nthcdr 20 '(pine fir oak maple))", "nil");
    ASSERT_OUTPUT_EQ(m, "(nthcdr 20 nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(nthcdr 0 '(pine fir oak maple))", "(pine fir oak maple)");
    ASSERT_OUTPUT_EQ(m, "(nthcdr 0 (cons 1 2))", "(1 . 2)");
    ASSERT_OUTPUT_EQ(m, "(nthcdr 1 (cons 1 2))", "2");
    ASSERT_EXCEPTION(m, "(nthcdr 2 (cons 1 2))", exceptions::WrongTypeArgument);
}

void testVariables()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "(eq (+ most-positive-fixnum 1) most-negative-fixnum)", "t");
    ASSERT_OUTPUT_EQ(m, "(eql 1 1)", "t");
    ASSERT_OUTPUT_EQ(m, "(eql 1.0 1)", "nil");
    ASSERT_OUTPUT_EQ(m, "(eql 'crab 'crab)", "t");
    ASSERT_OUTPUT_EQ(m, "(eql () ())", "t");
    ASSERT_OUTPUT_EQ(m, "'(;comment\n1)", "(1)");
    ASSERT_OUTPUT_CONTAINS(m, "1e5", "100000.0");
    ASSERT_OUTPUT_EQ(m, "(boundp 'abracadabra)", "nil");
    ASSERT_OUTPUT_EQ(m, "(let ((abracadabra 5))(boundp 'abracadabra))", "t");
    ASSERT_OUTPUT_EQ(m, "(boundp 'abracadabra)", "nil");
    ASSERT_OUTPUT_EQ(m, "(setq abracadabra 5)", "5");
    ASSERT_OUTPUT_EQ(m, "(boundp 'abracadabra)", "t");
    ASSERT_OUTPUT_EQ(m, "(boundp nil)", "t");
    ASSERT_OUTPUT_EQ(m, "(numberp 1)", "t");
    ASSERT_OUTPUT_EQ(m, "(numberp 1.0)", "t");
    ASSERT_OUTPUT_EQ(m, "(numberp nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(numberp \"A\")", "nil");
    ASSERT_EXCEPTION(m, "(setq nil t)", exceptions::SettingConstant);
    ASSERT_OUTPUT_EQ(m, "(set 'y 15)", "15");
    ASSERT_OUTPUT_EQ(m, "(progn (setq x 1) (let (x z) (setq x 2) "
                     "(setq z 3) (setq y x)) (list x y))", "(1 2)");
    ASSERT_OUTPUT_EQ(m, "(setq x 1) ; Put a value in the global binding.", "1");
    ASSERT_EXCEPTION(m, "(let ((x 2)) (makunbound 'x) x)", exceptions::VoidVariable);
    ASSERT_OUTPUT_EQ(m, "x ; The global binding is unchanged.", "1");
    
    ASSERT_EXCEPTION(m, R"code(
(let ((x 2))             ; Locally bind it.
  (let ((x 3))           ; And again.
    (makunbound 'x)      ; Void the innermost-local binding.
    x))                  ; And refer: it’s void.
)code", exceptions::VoidVariable);

    ASSERT_OUTPUT_EQ(m, R"code(
(let ((x 2))
  (let ((x 3))
    (makunbound 'x))     ; Void inner binding, then remove it.
  x)                     ; Now outer let binding is visible.
)code", "2");
    
    ASSERT_OUTPUT_EQ(m, R"code(
(setq x -99)  ; x receives an initial value of -99.
(defun getx ()
  x)            ; x is used free in this function.
(let ((x 1))    ; x is dynamically bound.
  (getx))
)code", "1");
    
    ASSERT_OUTPUT_EQ(m, R"code(
(setq x -99)      ; x receives an initial value of -99.
(defun addx ()
  (setq x (1+ x)))  ; Add 1 to x and return its new value.
(let ((x 1))
  (addx)
  (addx))
)code", "3");
    ASSERT_OUTPUT_EQ(m, "(addx)", "-98");
    ASSERT_OUTPUT_EQ(m, "(defvar var1 50)", "var1");
    ASSERT_OUTPUT_EQ(m, "var1", "50");
    ASSERT_OUTPUT_EQ(m, "(defvar var1 60)", "var1");
    ASSERT_OUTPUT_EQ(m, "var1", "50");
    ASSERT_OUTPUT_EQ(
        m,
        ConvertParsedNamesToUpperCase ? "(intern-soft \"VAR2\")" : "(intern-soft \"var2\")",
        "nil");
    ASSERT_OUTPUT_EQ(m, "(defvar var2)", "var2");
    ASSERT_OUTPUT_EQ(
        m,
        ConvertParsedNamesToUpperCase ? "(intern-soft \"VAR2\")" : "(intern-soft \"var2\")",
        "var2");
}

void testSymbols()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "(symbol-value nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(listp (symbol-plist 'cbdc))", "t");
    ASSERT_OUTPUT_EQ(m, "(listp (symbol-plist nil))", "t");
    ASSERT_OUTPUT_EQ(m, "(symbol-plist :akeyword)", "nil");
    ASSERT_OUTPUT_EQ(m, "(get 'some-symbol 'some-nonexisting-property)", "nil");
    ASSERT_OUTPUT_EQ(m, "(put 'object :id 345)", "345");
    ASSERT_OUTPUT_EQ(m, "(symbol-plist 'object)", "(:id 345)");
    ASSERT_OUTPUT_EQ(m, "(put 'object :id 346)", "346");
    ASSERT_OUTPUT_EQ(m, "(symbol-plist 'object)", "(:id 346)");
    ASSERT_OUTPUT_EQ(m, "(get 'object :id)", "346");
    ASSERT_OUTPUT_EQ(m, "(put 'object :guid 532512542)", "532512542");
    ASSERT_OUTPUT_EQ(m, "(symbol-plist 'object)", "(:id 346 :guid 532512542)");
    ASSERT_OUTPUT_EQ(m, "(setcdr (cdddr (symbol-plist 'object)) (cons 'odd nil))", "(odd)");
    ASSERT_OUTPUT_EQ(m, "(symbol-plist 'object)", "(:id 346 :guid 532512542 odd)");
    ASSERT_OUTPUT_EQ(m, "(get 'object 'odd)", "nil");
    ASSERT_EXCEPTION(m, "(put 'object :rating 8)", exceptions::Error); // Can't put to improper plists
    ASSERT_OUTPUT_EQ(m, "(put 'object :id 347)", "347"); // But can modify existing values

    ASSERT_OUTPUT_EQ(m, "'('a 'b)", "('a 'b)");
    ASSERT_OUTPUT_EQ(m, "'('a'b)", "('a 'b)");
    ASSERT_OUTPUT_EQ(m, "(symbolp 'abc)", "t");
    ASSERT_OUTPUT_EQ(m, "(symbol-name 'abc)", "\"abc\"");
    assert(expect<exceptions::VoidVariable>([&]() {
        m.evaluate("(symbolp abc)");
    }));
    assert(expect<exceptions::WrongTypeArgument>([&]() {
        m.evaluate("(symbol-name 2)");
    }));
    ASSERT_OUTPUT_EQ(m, "(make-symbol \"test\")", "test");
    ASSERT_OUTPUT_EQ(m, "(symbolp (make-symbol \"test\"))", "t");
    ASSERT_EXCEPTION(m, "(+ 1 (make-symbol \"newint\"))", exceptions::WrongTypeArgument);
    ASSERT_EQ(m.evaluate("(progn (setq sym (make-symbol \"foo\"))(symbol-name sym))"), "\"foo\"");
    ASSERT_EQ(m.evaluate("(eq sym 'foo)"), "nil");
    ASSERT_EQ(m.evaluate("'t"), "t");
    assert(expect<exceptions::VoidVariable>([&]() {
        m.evaluate("(eq 'a a)");
    }));
    ASSERT_EQ(m.evaluate("(symbolp (car (list 'a)))"), "t");
    ASSERT_EXCEPTION(m, "(progn (setq testint (make-symbol \"abracadabra\"))"
                     "(+ 1 (eval testint)))", exceptions::VoidVariable);

    // Unintern should't unintern if the symbol passed is actually an uninterned symbol with
    // same name:
    ASSERT_OUTPUT_EQ(m, "(setq interned 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(unintern (make-symbol \"interned\"))", "nil");
    ASSERT_OUTPUT_EQ(m, "(intern-soft \"interned\")", "interned");
    ASSERT_OUTPUT_EQ(m, "(unintern (intern-soft \"interned\"))", "t");
    ASSERT_OUTPUT_EQ(m, "(intern-soft \"interned\")", "nil");
}

void testKeywords()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, ":keyword1", ":keyword1");
    ASSERT_EXCEPTION(m, "(set :keyword2 1)", exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(getf (list :a 1 :b 2 :c 3) :a)", "1");
}

void testEqFunction()
{
    alisp::Machine m;
    ASSERT_EQ(m.evaluate("(progn (setq x \"a\")(eq x x))"), "t");
    ASSERT_EQ(m.evaluate("(progn (setq y 1)(eq y y))"), "t");
    ASSERT_EQ(m.evaluate("(eq \"a\" \"a\")"), "nil");
    ASSERT_EQ(m.evaluate("(eq 'a 'a)"), "t");
    ASSERT_EQ(m.evaluate("(eq 1 1)"), "t");
    ASSERT_EQ(m.evaluate("(eq 1 1.0)"), "nil");
    ASSERT_EQ(m.evaluate("(eq 1.0 1.0)"), "t");
    ASSERT_EQ(m.evaluate("(eq nil nil)"), "t");
    ASSERT_EQ(m.evaluate("(eq () nil)"), "t");
    ASSERT_EQ(m.evaluate("(eq '() nil)"), "t");
    ASSERT_EQ(m.evaluate("(progn (setq l '(a b))(eq l l))"), "t");
    ASSERT_OUTPUT_EQ(m, "(equal 'foo 'foo)", "t");
    ASSERT_OUTPUT_EQ(m, "(equal 456 456)", "t");
    ASSERT_OUTPUT_EQ(m, "(equal \"asdf\" \"asdf\")", "t");
    ASSERT_OUTPUT_EQ(m, "(equal '(1 (2 (3))) '(1 (2 (3))))", "t");
    ASSERT_OUTPUT_EQ(m, "(eq '(1 (2 (3))) '(1 (2 (3))))", "nil");
    /*
    ASSERT_OUTPUT_EQ(m, "(setq cy1 (progn (set 'z (list 1 2 3))(setcdr (cdr (cdr z)) (cdr z)) z))",
                     "(1 2 3 2 . #2)");
    ASSERT_OUTPUT_EQ(m, "(setq cy2 (progn (set 'z (list 1 2 3))(setcdr (cdr (cdr z)) (cdr z)) z))",
                     "(1 2 3 2 . #2)");
    std::cout << m.evaluate("cy1")->toString() << std::endl;
    std::cout << m.evaluate("cy1")->asList() << std::endl;
    auto cc = m.evaluate("cy1");
    std::cout << "Is " << cc->toString() << std::flush << " cyclical? " << cc->asList()->cc->isCyclical() << std::endl;
    ASSERT_EXCEPTION(m, "(equal cy1 cy2)", exceptions::CircularList);
    */
}

void testInternFunction()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(intern \"\")", "##");
    ASSERT_OUTPUT_EQ(m, "(eq (intern \"TT\") 'TT)", "t");
    ASSERT_OUTPUT_EQ(m, "(setq sym (intern \"FOO\"))", "FOO");
    ASSERT_OUTPUT_EQ(m, "(eq sym 'FOO)", "t");
    ASSERT_OUTPUT_EQ(m, "(intern-soft \"FRAZZLE\")", "nil");
    ASSERT_OUTPUT_EQ(m, "(setq sym (intern \"FRAZZLE\"))", "FRAZZLE");
    ASSERT_OUTPUT_EQ(m, "(intern-soft \"FRAZZLE\")", "FRAZZLE");
    ASSERT_OUTPUT_EQ(m, "(eq sym 'FRAZZLE)", "t");
    ASSERT_OUTPUT_EQ(m, "(intern-soft \"abc\")", "nil");
    ASSERT_OUTPUT_EQ(m, "(setq sym (intern \"abc\"))", "abc");
    ASSERT_OUTPUT_EQ(m, "(intern-soft \"abc\")", "abc");
    ASSERT_OUTPUT_EQ(m, "(unintern sym)", "t");
    ASSERT_OUTPUT_EQ(m, "(intern-soft \"abc\")", "nil");

    // Ensure same functionality as emacs: if sym refers to abra and then abra is uninterned,
    // after that describe-variable still can show what sym pointed to.
    ASSERT_OUTPUT_EQ(m, "(setq sym (intern \"ABRA\"))", "ABRA");
    ASSERT_OUTPUT_EQ(m, "(setq ABRA 500)", "500");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 'ABRA)", "ABRA's value is 500");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable sym)", "ABRA's value is 500");
    ASSERT_OUTPUT_EQ(m, "(format \"%d\" ABRA)", "\"500\"");
    ASSERT_EXCEPTION(m, "(message \"%d\" sym)", exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(unintern sym)", "t"); // this removes abra from objarray
    ASSERT_EXCEPTION(m, "(message \"%d\" ABRA)", exceptions::VoidVariable);
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable sym)", "ABRA's value is 500");
}

void testDescribeVariableFunction()
{
    Machine m;
    ASSERT_EXCEPTION(m, "(describe-variable a)", exceptions::VoidVariable);
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 'a)", "a is void as a variable");
    m.evaluate("(setq a 12345)");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 'a)", "12345");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable nil)", "nil's value is nil");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable t)", "t's value is t");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 't)", "t's value is t");
}

void testBasicArithmetic()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "(ash 1 0)", "1");
    ASSERT_OUTPUT_EQ(m, "(ash 1 1)", "2");
    ASSERT_OUTPUT_EQ(m, "(ash 1 2)", "4");
    ASSERT_OUTPUT_EQ(m, "(ash 8 -1)", "4");
    ASSERT_OUTPUT_EQ(m, "(logxor 12 5 7)", "14");
    ASSERT_OUTPUT_EQ(m, "(lognot 5)", "-6");
    ASSERT_OUTPUT_EQ(m, "(logcount 43)", "4");
    ASSERT_OUTPUT_EQ(m, "(logcount -43)", "3");
    ASSERT_OUTPUT_EQ(m, "-1", "-1");
    ASSERT_OUTPUT_EQ(m, "(zerop 0.0)", "t");
    ASSERT_OUTPUT_EQ(m, "(zerop 0)", "t");
    ASSERT_OUTPUT_EQ(m, "(zerop 1)", "nil");
    ASSERT_EXCEPTION(m, "(isnan (/ 0 0))", exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(isnan (/ 0.0 0.0))", "t");
    ASSERT_OUTPUT_EQ(m, "(isnan (/ 0 0.0))", "t");
    ASSERT_OUTPUT_EQ(m, "(isnan (/ 0.0 0))", "t");
    ASSERT_OUTPUT_EQ(m, "(isnan 0.0e+NaN)", "t");
    ASSERT_OUTPUT_EQ(m, "(isnan -0.0e+NaN)", "t");
    ASSERT_OUTPUT_EQ(m, "(<= 2.1 2)", "nil");
    ASSERT_OUTPUT_EQ(m, "(<= 1 2)", "t");
    ASSERT_OUTPUT_EQ(m, "(<= 2 2)", "t");
    ASSERT_OUTPUT_EQ(m, "(< 2 2)", "nil");
    ASSERT_OUTPUT_EQ(m, "(<= 1 2 3)", "t");
    ASSERT_OUTPUT_EQ(m, "(<= 1 2 3 4.0)", "t");
    ASSERT_OUTPUT_EQ(m, "(<= 1 2 3 4.0 3)", "nil");
    ASSERT_OUTPUT_EQ(m, "(% 5 2)", "1");
    ASSERT_EXCEPTION(m, "(% 5 2.0)", exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(+ 1 1)", "2");
    ASSERT_OUTPUT_EQ(m, "(+)", "0");
    ASSERT_OUTPUT_EQ(m, "(* 3 4)", "12");
    ASSERT_OUTPUT_EQ(m, "(*)", "1");
    ASSERT_OUTPUT_EQ(m, "(+ 1 -1)", "0");
    ASSERT_OUTPUT_EQ(m, "(1+ 0)", "1");
    ASSERT_OUTPUT_CONTAINS(m, "(1+ 0.0)", "1.0");
    ASSERT_EXCEPTION(m, "(1+ \"a\")", exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_CONTAINS(m, "(+ +.1 -0.1)", "0.0");
    ASSERT_OUTPUT_EQ(m, "(= 1 1)", "t");
    ASSERT_OUTPUT_EQ(m, "(= 1.0 1)", "t");
    ASSERT_OUTPUT_EQ(m, "(= 1 1.0)", "t");
    ASSERT_OUTPUT_EQ(m, "(= 1 1.0 1.0)", "t");
    ASSERT_OUTPUT_EQ(m, "(= 1 1.0 1.0 1.0)", "t");
    ASSERT_OUTPUT_EQ(m, "(= 1 2)", "nil");
    ASSERT_EXCEPTION(m, "(truncate 1 0)", exceptions::ArithError);
    ASSERT_OUTPUT_EQ(m, "(truncate 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(truncate 1.1)", "1");
    ASSERT_OUTPUT_EQ(m, "(truncate -1.2)", "-1");
    ASSERT_OUTPUT_EQ(m, "(truncate 19.5 3.2)", "6");
    ASSERT_OUTPUT_EQ(m, "(truncate 5.999 nil)", "5");
    ASSERT_OUTPUT_EQ(m, "(ceiling -1.5)", "-1");
    ASSERT_OUTPUT_EQ(m, "(floor -1.5)", "-2");
    ASSERT_OUTPUT_EQ(m, "(floor 1.5)", "1");
    ASSERT_OUTPUT_EQ(m, "(ceiling 2)", "2");
    ASSERT_OUTPUT_EQ(m, "(abs -1.5)", "1.500000");
    ASSERT_OUTPUT_EQ(m, "(abs -4)", "4");
}

void testNthFunction()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(setq x '(\"a\" \"b\"))", "(\"a\" \"b\")");
    ASSERT_OUTPUT_EQ(m, "(nth 0 x)", "\"a\"");
    ASSERT_OUTPUT_EQ(m, "(nth 1 x)", "\"b\"");
    ASSERT_OUTPUT_EQ(m, "(nth 2 x)", "nil");
    ASSERT_OUTPUT_EQ(m, "(eq (nth 1 x) (nth 1 x))", "t");
    ASSERT_OUTPUT_EQ(m, "(setq y (cons \"c\" x))", "(\"c\" \"a\" \"b\")");
    ASSERT_OUTPUT_EQ(m, "(nth 1 x)", "\"b\"");
    ASSERT_OUTPUT_EQ(m, "(nth 2 y)", "\"b\"");
    ASSERT_OUTPUT_EQ(m, "(eq (nth 1 x) (nth 2 y) )", "t");
}

void testConsFunction()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(cons 1 '(2 3))", "(1 2 3)");
    ASSERT_OUTPUT_EQ(m, "(cons 1 '())", "(1)");
    ASSERT_OUTPUT_EQ(m, "(cons 1 2)", "(1 . 2)");
}

void testListFunction()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(list 'a 'b)", "(a b)");
    ASSERT_OUTPUT_EQ(m, "(list 1 2 3 4 5)", "(1 2 3 4 5)");
    ASSERT_OUTPUT_EQ(m, "(list 1 2 '(3 4 5) 'foo)", "(1 2 (3 4 5) foo)");
    ASSERT_OUTPUT_EQ(m, "(list)", "nil");
    ASSERT_OUTPUT_EQ(m, "(cdr (list 'a 'b 'c))", "(b c)");
    ASSERT_OUTPUT_EQ(m, "(last '(1)) ; (3) ", "(1)");
    ASSERT_OUTPUT_EQ(m, "(last '(1 2 3)) ; (3) ", "(3)");
    ASSERT_OUTPUT_EQ(m, "(last '(1 2 . 3))", "(2 . 3)");
    ASSERT_OUTPUT_EQ(m, "(last nil)", "nil");
}

void testEvalFunction()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "(eval 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(setq foo 'bar)", "bar");
    ASSERT_OUTPUT_EQ(m, "(setq bar 'baz)", "baz");
    ASSERT_OUTPUT_EQ(m, "(eval 'foo)", "bar");
    ASSERT_OUTPUT_EQ(m, "(eval foo)", "baz");
}

void testMacros()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "((lambda (x) (+ x 1)) 1)", "2");
    ASSERT_OUTPUT_EQ(m, "(defmacro test-macro (a) a)", "test-macro");
    ASSERT_OUTPUT_EQ(m, "(symbol-function 'test-macro)", "(macro lambda (a) a)");
    ASSERT_OUTPUT_EQ(m, "(macroexpand 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(macroexpand nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(macroexpand '(test-macro 123))", "123");
    ASSERT_OUTPUT_EQ(m, "(test-macro 123)", "123");
    ASSERT_OUTPUT_EQ(m, "(when t)", "nil");
    ASSERT_OUTPUT_EQ(m, "(when nil t)", "nil");
    ASSERT_OUTPUT_EQ(m, "(when t nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(when t 1 2 3)", "3");
    ASSERT_OUTPUT_EQ(m, "(when (= 1 2) 1 2 3)", "nil");
    ASSERT_EXCEPTION(m, "(pop nil)", exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(setq l '(a b))", "(a b)");
    ASSERT_OUTPUT_EQ(m, "(push 'c l)", "(c a b)");
    ASSERT_OUTPUT_EQ(m, "(push 'd l)", "(d c a b)");
    ASSERT_OUTPUT_EQ(m, "(defmacro inc (var) (list 'setq var (list '1+ var)))", "inc");
    ASSERT_OUTPUT_EQ(m, "(setq x 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(inc x)", "2");
    ASSERT_EXCEPTION(m, "(inc 1)", exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(setq li '(1 2 3))", "(1 2 3)");
    ASSERT_OUTPUT_EQ(m, "(pop li)", "1");
    ASSERT_OUTPUT_EQ(m, "li", "(2 3)");
    ASSERT_EXCEPTION(m, "(pop 1)", exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(defmacro asetf (var value) (setq ty value) (list 2 3))", "asetf");
    ASSERT_OUTPUT_EQ(m, "(macroexpand '(asetf 1 2))", "(2 3)");
    ASSERT_OUTPUT_EQ(m, "ty", "2");
    ASSERT_OUTPUT_EQ(m, "(macroexpand '(inc r))", "(setq r (1+ r))");
    ASSERT_OUTPUT_EQ(m, "(setf x (list 1 2))", "(1 2)");
    ASSERT_OUTPUT_EQ(m, "(setf (car x) 3)", "3");
    ASSERT_OUTPUT_EQ(m, "x", "(3 2)");
    ASSERT_OUTPUT_EQ(m, "(setf (cadr x) 4)", "4");
    ASSERT_OUTPUT_EQ(m, "x", "(3 4)");
    ASSERT_OUTPUT_CONTAINS(m, R"code(
(defmacro inc2 (var1 var2)
  (list 'progn (list 'inc var1) (list 'inc var2)))
(macroexpand '(inc2 r s))
)code", "(progn (inc r) (inc s))");
    TEST_CODE(m, R"code(
(defmacro mirror (x)
  1
  2
  x)
(mirror 12) => 12
)code");
}

void testDeepCopy()
{
    Machine m;
    // Cloning
    auto storage = m.evaluate("'(1 2 3 4)");
    auto list = dynamic_cast<alisp::ConsCellObject*>(storage.get());
    assert(list);
    auto cloned = list->clone();
    list->cc->car = alisp::makeInt(5);
    ASSERT_EQ(list->toString(), "(5 2 3 4)");
    ASSERT_EQ(cloned->toString(), "(5 2 3 4)");
    // Deepcopying
    auto copied = list->deepCopy();
    list->cc->car = alisp::makeInt(1);
    ASSERT_EQ(list->toString(), "(1 2 3 4)");
    ASSERT_EQ(copied->toString(), "(5 2 3 4)");
    // Should work dotted pairs as well
    storage = m.evaluate("'(1 2 . 3)");
    list = dynamic_cast<alisp::ConsCellObject*>(storage.get());
    assert(list);
    ASSERT_EQ(list->toString(), "(1 2 . 3)");
    copied = list->deepCopy();
    ASSERT_EQ(copied->toString(), "(1 2 . 3)");
}

void testFunctions()
{
    Machine m;
    std::stringstream ss;
    m.setVariable("debugstream", std::make_unique<OStreamObject>(&ss));
    ASSERT_OUTPUT_EQ(m, "(apply 'cons '((+ 2 3) 4))", "((+ 2 3) . 4)");
    ASSERT_OUTPUT_EQ(m, "(defun tempfunc () nil)"
                     "(let (tempfunc)(setq tempfunc 5) (fboundp 'tempfunc))", "t");
    ASSERT_OUTPUT_EQ(m, "(fset 'minus '-)", "-");
    ASSERT_OUTPUT_EQ(m, "(fboundp 'minus)", "t");
    ASSERT_OUTPUT_EQ(m, "(fset 'minus 1)" , "1");
    ASSERT_OUTPUT_EQ(m, "(fboundp 'minus)", "t");
    ASSERT_OUTPUT_EQ(m, "(fset 'minus nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(fboundp 'minus)", "nil");
    ASSERT_EXCEPTION(m, "(fboundp 5)", exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(fboundp '+)", "t");
    ASSERT_OUTPUT_EQ(m, "(fboundp '++++)", "nil");
    ASSERT_OUTPUT_EQ(m, "(progn (defun gms(y) (+ 1 y)) (symbol-function 'gms))",
                     "(lambda (y) (+ 1 y))");
    ASSERT_OUTPUT_EQ(m, "#'test", "test");
    ASSERT_OUTPUT_EQ(m, "(apply 'set (list 'foo 5))", "5");
    ASSERT_OUTPUT_EQ(m, "(apply 'set '(foo 5))", "5");
    ASSERT_OUTPUT_EQ(m, "(apply '+ '(3 4))", "7");
    ASSERT_EXCEPTION(m, "(apply '+)", exceptions::Error);
    ASSERT_EXCEPTION(m, "(apply '+ 7)", exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(apply '+ 1 2 '(3 4))", "10");
    ASSERT_OUTPUT_EQ(m, "(apply '* ())", "1");
    ASSERT_EXCEPTION(m, "(func-arity +)", exceptions::VoidVariable);
    ASSERT_EXCEPTION(m, "(func-arity 1)", exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(func-arity '%)", "(2 . 2)");
    ASSERT_OUTPUT_EQ(m, "(func-arity (lambda (x y &optional z) (* x y z)))", "(2 . 3)");
    ASSERT_OUTPUT_EQ(m, "(func-arity (symbol-function '%))", "(2 . 2)");
    ASSERT_OUTPUT_EQ(m, "(progn (setq plus '+)(setq plus2 plus)(setq plus3 plus)"
                     "(indirect-function plus2))", "#<subr +>");
    ASSERT_EXCEPTION(m, "(y 1 1)", exceptions::VoidFunction);
    ASSERT_EXCEPTION(m, "('y 1 1)", exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(symbol-function '+)", "#<subr +>");
    ASSERT_OUTPUT_EQ(m, "(funcall (symbol-function '+) 1 2)", "3");
    ASSERT_OUTPUT_EQ(m, "(functionp 5)", "nil");
    ASSERT_EXCEPTION(m, "(functionp abbu)", exceptions::VoidVariable);
    ASSERT_OUTPUT_EQ(m, "(functionp 'set)", "t");
    ASSERT_OUTPUT_EQ(m, "(functionp 'setq)", "nil");
    ASSERT_OUTPUT_EQ(m, "(functionp nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(lambda (x) (* x x))", "(lambda (x) (* x x))");
    ASSERT_OUTPUT_EQ(m, "((lambda (x) (+ x 1)) 1)", "2");
    ASSERT_OUTPUT_EQ(m, "(function (lambda (x) (* x x)))", "(lambda (x) (* x x))");
    ASSERT_OUTPUT_EQ(m, "(macroexpand '(lambda (x) (+ x 1)))", "#'(lambda (x) (+ x 1))");
    ASSERT_OUTPUT_EQ(m, "(functionp (lambda (x) (+ x 1)))", "t");
    ASSERT_OUTPUT_EQ(m, "(listp (function (lambda (x) (* x x))))", "t");
    ASSERT_EXCEPTION(m, "(funcall + 1 2)", exceptions::VoidVariable);
    ASSERT_EXCEPTION(m, "(funcall 1 2)", exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(funcall '+ 1 2)", "3");
    ASSERT_OUTPUT_EQ(m, "(car (car nil))", "nil");
    ASSERT_OUTPUT_EQ(m, "(car (car 'nil))", "nil");
    ASSERT_OUTPUT_EQ(m, "(caar 'nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(defun foo () (princ \"foo\" debugstream) 5)", "foo");
    ASSERT_OUTPUT_EQ(m, "(defun foo2 (msg) (princ msg debugstream) msg)", "foo2");
    ASSERT_OUTPUT_EQ(m, "(symbol-function nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(listp (symbol-function 'foo2))", "t");
    ASSERT_OUTPUT_CONTAINS(m, "(symbol-function 'foo2)", "(lambda (msg) (princ msg debugstream) msg)");
    ASSERT_EXCEPTION(m, "((symbol-function 'foo2) \"fsda\")", exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(foo)", "5");
    ASSERT_EXCEPTION(m, "(foo2)", exceptions::WrongNumberOfArguments);
    ASSERT_OUTPUT_EQ(m, "(foo2 \"abc\")", "\"abc\"");
    ASSERT_OUTPUT_EQ(m, "(cadr '(1 2 3))", "2");
    ASSERT_OUTPUT_EQ(m, "(cadr nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(cdr-safe '(1 2 3))", "(2 3)");
    ASSERT_OUTPUT_EQ(m, "(cdr-safe 1)", "nil");
    ASSERT_OUTPUT_EQ(m, "(car-safe '(1 2 3))", "1");
    ASSERT_OUTPUT_EQ(m, "(car-safe 1)", "nil");
    ASSERT_OUTPUT_EQ(m, "(cdar '((1 4) 2 3))", "(4)");    
    ASSERT_OUTPUT_EQ(m, "(cdar nil)", "nil");
    ASSERT_EXCEPTION(m, "(cdar '(1 2 3))", exceptions::WrongTypeArgument);    
    ASSERT_OUTPUT_EQ(m, "(caar '((8) 2 3))", "8");
    ASSERT_OUTPUT_EQ(m, "(progn (defun xx () t) (functionp 'xx))", "t");
    ASSERT_EQ(ss.str(), "fooabc");
    ASSERT_OUTPUT_CONTAINS(m, R"code(
(lambda (x)
  "Return the hyperbolic cosine of X."
  (* 0.5 (+ (exp x) (exp (- x)))))
)code", "hyperbolic");
    ASSERT_OUTPUT_EQ(m, R"code(
(funcall (lambda (a b c) (+ a b c))
         1 2 3)
)code", "6");
    ASSERT_OUTPUT_EQ(m, R"code(
(funcall (lambda (a b c) (+ a b c)) 1 (* 2 3) 1)
)code", "8");
    ASSERT_OUTPUT_EQ(m, R"code(
(defun dumb-f () "I'm a function")
(defvar my-function 'dumb-f)
(funcall my-function)
)code", "\"I'm a function\"");

    ASSERT_OUTPUT_EQ(m, R"code(
(defun foobar () "old")
(set 'foo-backup (symbol-function 'foobar))
(defun foobar () "new")
(fset 'foobar foo-backup)
(foobar)
)code", "\"old\"");
    ASSERT_OUTPUT_EQ(m, "(mapatoms (lambda (str) (print str debugstream)))", "nil");
    assert(ss.str().find("truncate") != std::string::npos);
}

void testLet()
{
    alisp::Machine m;
    ASSERT_EXCEPTION(m, "(let (1) nil)", exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(let ((x 1) (y (+ 1 2))) (format \"%d\" x) (+ x y))", "4");
    ASSERT_OUTPUT_EQ(m, "(let* ((x 1) (y x)) y)", "1");
    ASSERT_EXCEPTION(m, "(let ((x 1) (y x)) y)", exceptions::VoidVariable);
    ASSERT_OUTPUT_EQ(m, R"code(
(setq y 2)
(let ((y 1)
      (z y))
  (list y z))
)code", "(1 2)");
    ASSERT_OUTPUT_EQ(m, R"code(
(setq y 2)
(let* ((y 1)
       (z y))    ; Use the just-established value of y.
  (list y z))
)code", "(1 1)");
    ASSERT_OUTPUT_EQ(m, R"code(
(setq abracadabra 5)
(setq foo 9)
;; Here the symbol abracadabra
;;   is the symbol whose value is examined.
(let ((abracadabra 'foo))
  (symbol-value 'abracadabra))
)code", "foo");
    ASSERT_OUTPUT_EQ(m, R"code(
;; Here, the value of abracadabra,
;;   which is foo,
;;   is the symbol whose value is examined.
(let ((abracadabra 'foo))
  (symbol-value abracadabra))
)code", "9");
    ASSERT_OUTPUT_EQ(m, "(symbol-value 'abracadabra)", "5");
   
}

void testIf()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "(if t 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(if (eq 1 1) 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(if nil 1)", "nil");
    ASSERT_OUTPUT_EQ(m, "(if nil (message \"problem\") 2 3 4)", "4");
    ASSERT_OUTPUT_EQ(m, "(if nil 1 2 3)", "3");
}

void testCyclicals()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m,
                     "(progn (set 'z (list 1 2 3))(setcdr (cdr (cdr z)) (cdr z)) z)",
                     "(1 2 3 2 . #2)");
    ASSERT_EXCEPTION(m, "(length z)", exceptions::Error);
    ASSERT_OUTPUT_EQ(m,
                     "(progn (set 'z (list 1 2 3 4 5 6 7))"
                     "(setcdr (cdr (cdr (cdr (cdr (cdr (cdr z)))))) (cdr z))"
                     "z)",
                     "(1 2 3 4 5 6 7 2 3 4 5 6 . #6)");

    // Unfortunately I haven't been able to reverse engineer how emacs cuts cyclical
    // list printing. But as long as the number following hashtag is correct and the
    // patter is visibly clear from inspecting the printed list, this shouldn't be a
    // huge problem.
    /*
    ASSERT_OUTPUT_EQ(m,
                     "(progn (set 'z (list 1 2 3 4 5 6))"
                     "(setcdr (cdr (cdr (cdr (cdr (cdr z))))) (cdr (cdr z)))"
                     "z)", "(1 2 3 4 5 6 3 4 5 6 . #5)");
    */
    ASSERT_OUTPUT_EQ(m, "(let ((a (list 1))) (proper-list-p (setcdr a a)))", "nil");
    ASSERT_OUTPUT_EQ(m, "(let ((a (list 1)))(setcdr a a))", "(1 . #0)");

    auto list = m.evaluate("'(0 1 2 3 (4 (5 6) 7 8) 9)");
    std::set<int> ints;
    for (int i=0;i<10;i++) {
        ints.insert(i);
    }
    list->asList()->cc->traverse([&](const alisp::ConsCell* cell) {
        if (cell->car->isInt()) {
            ints.erase(atoi(cell->car->toString().c_str()));
        }
        return true;
    });
    assert(!list->asList()->cc->isCyclical());
    assert(ints.empty());
    m.evaluate("nil")->asList()->traverse([](const Object&) {
        assert(false && "Traversing empty list...");
        return false;
    });
    /*
      https://www.gnu.org/software/emacs/manual/html_node/elisp/Special-Read-Syntax.html
     */
    ASSERT_OUTPUT_EQ(m, "(setq x (list 1 2 3))", "(1 2 3)");
    ASSERT_OUTPUT_EQ(m, "(setcar x x)", "(#0 2 3)");


    ASSERT_OUTPUT_EQ(m, "(setq li (list 1 2 3 4 5 6 7 8))", "(1 2 3 4 5 6 7 8)");
    ASSERT_OUTPUT_EQ(m, "(setcar (cdr (cdr (cdr li))) (cdr li))", "(2 3 #0 5 6 7 8)");
    ASSERT_OUTPUT_EQ(m, "(setcar (cdr (cdr (cdr (cdr li)))) (cdr li))", "(2 3 #0 #0 6 7 8)");
    ASSERT_OUTPUT_EQ(m, "li", "(1 2 3 #1 #1 6 7 8)");
    auto li = m.evaluate("li");
    assert(li->isList());
    assert(li->asList()->cc->isCyclical());

    ASSERT_OUTPUT_EQ(m, "(progn (setq subli (list 10 11 12 13 14 15))"
                     "(setq li2 (list 1 2 subli 3 4 subli 5 6)))",
                     "(1 2 (10 11 12 13 14 15) 3 4 (10 11 12 13 14 15) 5 6)");
    auto li2 = m.evaluate("li2");
    assert(li2->isList());
    assert(!li2->asList()->cc->isCyclical());

    ASSERT_OUTPUT_EQ(m, "(progn (setq li3 (list 1 2 3))(setcdr (cdr (cdr li3)) li3))",
                     "(1 2 3 1 2 . #2)");
    auto li3 = m.evaluate("li3");
    assert(li3->isList());
    assert(li3->asList()->cc->isCyclical());
}

void testMemoryLeaks()
{
    using namespace alisp;
    std::unique_ptr<Machine> m = std::make_unique<Machine>();
    assert(Object::getDebugRefCount() > 0);
    const int baseCount = Object::getDebugRefCount();
    ASSERT_EXCEPTION(*m, "(pop nil)", exceptions::Error);
    assert(Object::getDebugRefCount() == baseCount && "Macro call");

    // A quite simple circular test case:
    if (true) {
        auto obj = m->evaluate("(let ((a (list 1)))(setcdr a a))");
        assert(Object::getDebugRefCount() > baseCount && "Circular");
        obj = nullptr;
        assert(Object::getDebugRefCount() == baseCount && "Circular");
    }

    // A slightly more complicated one
    if (true) {
        assert(Object::getDebugRefCount() == baseCount && "Before progn");
        auto obj = m->evaluate("(progn (set 'z (list 1 2 3 4 5 6 7))"
                               "(setcdr (cdr (cdr (cdr (cdr (cdr (cdr z)))))) (cdr z))"
                               "z)");
        const std::string zname = m->parsedSymbolName("z");
        m->getSymbolOrNull(zname);
        assert(obj->eq(*m->getSymbolOrNull(zname)->variable));
        assert(obj->eq(*m->evaluate(zname.c_str())));
        assert(Object::getDebugRefCount() > baseCount && "Circular2");
        auto clone = obj->clone();
        clone = nullptr;
        assert(Object::getDebugRefCount() > baseCount && "Circular2");
        obj = nullptr;
        assert(Object::getDebugRefCount() > baseCount && "Circular2");
        m->evaluate("(unintern 'z)");
        assert(Object::getDebugRefCount() == baseCount && "Circular2");
    }

    // One involving symbols
    if (true) {
        assert(Object::getDebugRefCount() == baseCount);
        const char* code = R"code(
(progn
  (setq s1 (make-symbol "a"))
  (setq s2 (make-symbol "b"))

  (set s1 s2)
  (set s2 s1)

  (unintern 's1))
)code";
        m->evaluate(code);
        assert(Object::getDebugRefCount() > baseCount && "Syms");
        m->evaluate("(unintern 's2)");
        assert(Object::getDebugRefCount() == baseCount && "Syms");
    }

    // Of course we should zero objects left after destroying the machine
    m = nullptr;
    assert(Object::getDebugRefCount() == 0);
}

void testControlStructures()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "(and 1 2)", "2");
    ASSERT_OUTPUT_EQ(m, "(and 1 nil 2)", "nil");
    ASSERT_OUTPUT_EQ(m, "(and)", "t");
    ASSERT_OUTPUT_EQ(m, R"code(
(let ((str ""))
  (dolist (elem (list "A" "B" "C"))
    (setq str (concat str elem)))
  str)
)code", "\"ABC\"");
    std::stringstream ss;
    m.setVariable("debugstream", std::make_unique<OStreamObject>(&ss));
    ASSERT_OUTPUT_EQ(m, R"code(
(setq animals '(gazelle giraffe lion tiger))
(defun print-elements-of-list (list)
  "Print each element of LIST on a line of its own."
  (while list
    (print (car list) debugstream)
    (setq list (cdr list))))
(print-elements-of-list animals)
)code", "nil");
    ASSERT_EQ(ss.str(), "\ngazelle\n\ngiraffe\n\nlion\n\ntiger\n");
    ASSERT_OUTPUT_EQ(m, R"code(

(defun triangle (number-of-rows)  ; Version with incrementing counter.
  "Add up the number of pebbles in a triangle.
   The first row has one pebble, the second row two pebbles,
   the third row three pebbles, and so on.
   The argument is NUMBER-OF-ROWS."
  (let ((total 0)
        (row-number 1))
    (while (<= row-number number-of-rows)
      (setq total (+ total row-number))
      (setq row-number (1+ row-number)))
    total))
(triangle 7)
)code", "28");
    ASSERT_OUTPUT_EQ(m, "(unless nil 5)", "5");
    ASSERT_OUTPUT_EQ(m, "(unless nil 5 6)", "6");
    ASSERT_OUTPUT_EQ(m, "(unless nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(unless t 1)", "nil");
    ASSERT_OUTPUT_EQ(m, "(unless t 1 2)", "nil");
    ASSERT_EXCEPTION(m, "(unless)", exceptions::WrongNumberOfArguments);
    ASSERT_OUTPUT_EQ(m, R"code(
(cond
 ((= 2 4) no-evaluation-unless-condition-is-true)
 ((= 2 3) 3)
 ((= 2 2) (+ 1 1))
 ((= 2 1) 1)) ; 2
)code", "2");
    ASSERT_OUTPUT_EQ(m, R"code( (cond ((= 1 2) 1)) )code", "nil");
    ASSERT_OUTPUT_EQ(m, "(cond)", "nil");
    ASSERT_OUTPUT_EQ(m, "(xor t t)", "nil");
    ASSERT_OUTPUT_EQ(m, "(xor nil nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(xor t nil)", "t");
    ASSERT_OUTPUT_EQ(m, "(xor nil t)", "t");
}

void testConverter()
{
    Machine m;
    auto intObj = m.evaluate("5");
    assert(intObj->isConvertibleTo<int>());
    assert(intObj->isConvertibleTo<std::int64_t>());
    assert(intObj->isConvertibleTo<std::uint32_t>());
    if (!intObj->isConvertibleTo<std::variant<double, int>>()) {
        assert(false && "Convertible to int but not std::variant<int, double>");
    }
    auto variant = intObj->
        value<std::variant<double, int>>();
    int ival = std::get<int>(variant);
    assert(ival == 5);
    auto strObj = m.evaluate("\"ABC\"");
    assert(strObj->isConvertibleTo<std::string>());
    assert(strObj->isConvertibleTo<const std::string&>());
    assert(strObj->isConvertibleTo<std::string&>());
}

void testErrors()
{
    Machine m;
    std::stringstream ss;
    m.setVariable("debugstream", std::make_unique<OStreamObject>(&ss));
    ASSERT_EXCEPTION(m, "(error \"test: %d\" 1500)", exceptions::Error);
    
    ASSERT_OUTPUT_EQ(m, R"code(
(defun safe-divide (dividend divisor)
  (condition-case err
      ;; Protected form.
      (/ dividend divisor)
    ;; The handler(s).
    (some-strange-error nil)            ; This shouldn't cause problems here.
    (arith-error                        ; Condition.
     ;; Display the usual message for this error.
     (princ (error-message-string err) debugstream)
     1000000)))
)code", "safe-divide");
    ASSERT_OUTPUT_EQ(m, "(safe-divide 5 0)", "1000000");
    ASSERT_EQ(ss.str(), "arith-error:(\"Division by zero\")");

    ASSERT_OUTPUT_EQ(m, R"code(
(defun error-test-case-1 ()
  (condition-case nil
      (progn (error "test") nil)
    (error 1 2 3
     4)))
)code", "error-test-case-1");
    ASSERT_OUTPUT_EQ(m, "(error-test-case-1)", "4");
    
    ASSERT_OUTPUT_EQ(m, R"code(
; Here the error handler code is nil which should be allowed
(defun error-test-case-2 ()
  (condition-case nil
      (progn (error "test") nil)
    (error)))
)code", "error-test-case-2");
    ASSERT_OUTPUT_EQ(m, "(error-test-case-2)", "nil");
    
    ASSERT_OUTPUT_EQ(m, R"code(
; Here we have a handler for arith-error only. Should result in an uncatched error.
(defun error-test-case-3 ()
  (condition-case nil
      (progn (error "test") nil)
    (arith-error)))
)code", "error-test-case-3");
    ASSERT_EXCEPTION(m, "(error-test-case-3)", exceptions::Error);
    
    ASSERT_OUTPUT_EQ(m, R"code(
; Here we have no handlers at all. Not very useful, but shouldn't crash nevertheless!
(defun error-test-case-4 ()
  (condition-case nil
      (progn (error "test") nil)))
)code", "error-test-case-4");
    ASSERT_EXCEPTION(m, "(error-test-case-4)", exceptions::Error);
    
    ASSERT_OUTPUT_EQ(m, R"code(
(defun error-test-case-5 ()
  (condition-case nil
      (progn (error "test") nil)
    (1 t)))
)code", "error-test-case-5");
    ASSERT_EXCEPTION(m, "(error-test-case-5)", exceptions::Error);
    
    ASSERT_EXCEPTION(m, R"code(
(defun error-test-case-6 ()
  (condition-case nil
      (progn (error "test") nil)
    ()))
(error-test-case-6)
)code", exceptions::Error);

    ASSERT_OUTPUT_EQ(m, R"code(
(defun error-test-case-7 ()
  (condition-case nil
      (progn (error "nil handlers should be fine!") nil)
    ()()()()(error 123)))
(error-test-case-7)
)code", "123");
    
    ASSERT_EXCEPTION(m, R"code(
(defun error-test-case-8 ()
  (condition-case nil
      (progn (error "integers as handlers") nil)
    1 2 3 (error t)))
(error-test-case-8)
)code", exceptions::Error);
    
    ASSERT_EXCEPTION(m, R"code(
(defun error-test-case-9 ()
  (condition-case 123
      (progn (error "wrong type argument - symbol was expected") nil)
    1 2 3 (error t)))
(error-test-case-9)
)code", exceptions::WrongTypeArgument);

    ASSERT_OUTPUT_EQ(m, R"code(
(defun error-test-case-10 ()
  (condition-case nil
      (progn (error "first variable of handler may be a list") nil)
    ((some-other-error error) 123)))
(error-test-case-10)
)code", "123");

    ASSERT_OUTPUT_EQ(m, R"code(
; This shows that error hierarchy inheritance works properly.
(define-error 'my-error1 "My special arithmetic error" 'arith-error)
(define-error 'my-error2 "My special arithmetic error" 'my-error1)
(define-error 'my-error3 "My special arithmetic error" 'my-error2)
(symbol-plist 'my-error3)
)code", "(error-conditions (my-error3 my-error2 my-error1 arith-error error) error-message \"My special arithmetic error\")");
    
    ASSERT_OUTPUT_EQ(m, R"code(
; Although here an arith-error is signaled, we only have a generic error handler.
(defun safe-divide2 (dividend divisor)
  (condition-case err
      (/ dividend divisor)
    (error 1000000)))
(safe-divide2 5000 0)
)code", "1000000");
}

void testSequences()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "(progn (set 'z (list 1 2 3))"
                     "(setcdr (cdr (cdr z)) (cdr z)) z)", "(1 2 3 2 . #2)");
    ASSERT_EXCEPTION(m, "(reverse z)", exceptions::CircularList);
    ASSERT_OUTPUT_EQ(m, "(copy-sequence '())", "nil");
    ASSERT_OUTPUT_EQ(m, "(copy-sequence '(1 2 3))", "(1 2 3)");
    ASSERT_EXCEPTION(m, "(copy-sequence z)", exceptions::CircularList);
    ASSERT_OUTPUT_EQ(m, "(copy-sequence \"abc\")", "\"abc\"");
    ASSERT_EXCEPTION(m, "(reverse '(1 . 2))", exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(reverse '(1 2 3))", "(3 2 1)");
    ASSERT_OUTPUT_EQ(m, "(reverse ())", "nil");
    ASSERT_OUTPUT_EQ(m, "(reverse '(1))", "(1)");
    ASSERT_OUTPUT_EQ(m, "(mapcar 'car '((a b) (c d) (e f)))", "(a c e)");
    ASSERT_OUTPUT_EQ(m, R"code((mapcar 'string "abc"))code",
                     R"code(("a" "b" "c"))code");
    ASSERT_OUTPUT_EQ(m, "(nreverse ())", "nil");
    ASSERT_OUTPUT_EQ(m, "(nreverse '(1))", "(1)");
    ASSERT_OUTPUT_EQ(m, "(setq x (list 1 2))", "(1 2)");
    ASSERT_OUTPUT_EQ(m, "(nreverse x)", "(2 1)");
    ASSERT_OUTPUT_EQ(m, "(setq x (list 'a 'b 'c))", "(a b c)");
    ASSERT_OUTPUT_EQ(m, "(nreverse x)", "(c b a)");
    ASSERT_OUTPUT_EQ(m, "x", "(a)");
    ASSERT_OUTPUT_EQ(m, "(setq x (list 'a 'b 'c 'd 'e))", "(a b c d e)");
    ASSERT_OUTPUT_EQ(m, "(seq-elt x 3)", "d");
    ASSERT_OUTPUT_EQ(m, "(nreverse x)", "(e d c b a)");
    ASSERT_OUTPUT_EQ(m, "x", "(a)");
    ASSERT_OUTPUT_EQ(m, "(setq dotted '(1 2 3 4 . 5))", "(1 2 3 4 . 5)");
    ASSERT_EXCEPTION(m, "(nreverse dotted)", exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(progn (set 'z (list 1 2 3))"
                     "(setcdr (cdr (cdr z)) (cdr z)) z)", "(1 2 3 2 . #2)");
    ASSERT_EXCEPTION(m, "(nreverse z)", exceptions::CircularList);
    ASSERT_OUTPUT_EQ(m, "(setq nums (list 1 3 2 6 5 4 0))", "(1 3 2 6 5 4 0)");
    ASSERT_OUTPUT_EQ(m, "(sort nums #'<)", "(0 1 2 3 4 5 6)");
    ASSERT_OUTPUT_EQ(m, "nums", "(1 2 3 4 5 6)");
    ASSERT_OUTPUT_EQ(m, "(sort nil #'<)", "nil");
    ASSERT_OUTPUT_EQ(m, "(sort (list 1) #'<)", "(1)");
    ASSERT_OUTPUT_EQ(m, "(setq dotted '(1 3 2 6 5 4 . 0))", "(1 3 2 6 5 4 . 0)");
    ASSERT_EXCEPTION(m, "(sort dotted #'<)", exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(progn (set 'z (list 1 2 3))"
                     "(setcdr (cdr (cdr z)) (cdr z)) z)", "(1 2 3 2 . #2)");
    ASSERT_EXCEPTION(m, "(sort z #'<)", exceptions::CircularList);
}

static int testFunction(int a, int b) { return a + b; }

void testPublicInterface()
{
    Machine m;
    m["name"] = "Antti";
    m["cpp-func"] = [](std::string name){ return name + ", hello from C++!"; };
    m["c-func"] = testFunction;
    ASSERT_OUTPUT_EQ(m, "(cpp-func name)", "\"Antti, hello from C++!\"");
    int res = m.evaluate("(c-func 1 2)")->value<int>();
    assert(res == 3);
}

void testSetf()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "(setf (symbol-value 'foo) 5)", "5");
    ASSERT_OUTPUT_EQ(m, "foo", "5");
    ASSERT_OUTPUT_EQ(m, "(macroexpand '(setf (car x) 10))", "(setcar x 10)");
    
    ASSERT_OUTPUT_EQ(m, R"code(
(defun setnth (n x value)
  (rplaca (nthcdr n x) value)
  value)

(defun eleventh (list)
  (nth 10 list))

(defun set-eleventh (list new-val)
  (setnth 10 list new-val))

(defsetf eleventh set-eleventh)

(let ((l (list 1 2 3 4 5 6 7 8 9 10 11 12 13)))
  (setf (eleventh l) :foo)
  l)
)code", "(1 2 3 4 5 6 7 8 9 10 :foo 12 13)");

    TEST_CODE(m, R"code(
(defun middleguy (x) (nth (truncate (1- (list-length x)) 2) x)) => middleguy
(middleguy '(1 2 3 4)) => 2
(defun set-middleguy (x v)
    (unless (null x)
      (rplaca (nthcdr (truncate (1- (list-length x)) 2) x) v))
    v) => set-middleguy
(defsetf middleguy set-middleguy) => middleguy
(setq a (list 'a 'b 'c 'd)
      b (list 'x)
      c (list 1 2 3 (list 4 5 6) 7 8 9)) =>  (1 2 3 (4 5 6) 7 8 9)
(setf (middleguy a) 3) =>  3
(setf (middleguy b) 7) =>  7
(setf (middleguy (middleguy c)) 'middleguy-symbol) => middleguy-symbol
a => (a 3 c d)
b => (7)
c => (1 2 3 (4 middleguy-symbol 6) 7 8 9)
)code");
}

void test()
{
    testListBasics();
    testQuote();
    testFunctions();
    testSetf();
    testPublicInterface();
    testMacros();
    testSequences();
    testErrors();
    testNullFunction();
    testCarFunction();
    testConverter();
    testBasicArithmetic();
    testControlStructures();
    testVariables();
    testMemoryLeaks();
    testCyclicals(); // Lot of work to do here still...
    testLet();
    testSymbols();
    testIf();
    testDeepCopy();
    testEvalFunction();
    testCdrFunction();
    testConsFunction();
    testListFunction();
    testDelqFunction();
    testRemqFunction();
    testKeywords();
    testNthFunction();
    testStrings();
    testDescribeVariableFunction();
    testInternFunction();
    testEqFunction();
    testDivision();
    testSyntaxErrorDetection();
    //std::cout << "Remaining objects:\n";
    //Object::printAllObjects();
    assert(alisp::Object::getDebugRefCount() == 0);
    /*
foo                 ; A symbol named ‘foo’.
FOO                 ; A symbol named ‘FOO’, different from ‘foo’.
1+                  ; A symbol named ‘1+’
                    ;   (not ‘+1’, which is an integer).
\+1                 ; A symbol named ‘+1’
                    ;   (not a very readable name).
\(*\ 1\ 2\)         ; A symbol named ‘(* 1 2)’ (a worse name).
+-/_*~!@$%^&=:<>{}  ; A symbol named ‘+-/_*~!@$%^&=:<>{}’.
                    ;   These characters need not be escaped.
     */
}

namespace alisp {

void eval(Machine& m, const std::string& expr, bool interactive)
{
    try {
        auto res = m.evaluate(expr.c_str());
        if (!res || !interactive) {
            return;
        }
        std::cout << " => " << res->toString() << std::endl;
    }
    catch (exceptions::Error& ex) {
        ex.onHandle(m);
        std::cerr << ex.getMessageString() << std::endl;
        std::cerr << "\nCall stack:\n";
        std::cerr << ex.stackTrace << std::endl;
    }
    catch (exceptions::Exception& ex) {
        std::cerr << "An error was encountered:\n";
        std::cerr << ex.what() << std::endl;
    }
}

static bool exists(const std::string& name)
{
    std::ifstream f(name);
    return f.good();
}

static std::string readFile(const std::string &file_path)
{
    const std::ifstream input_stream(file_path, std::ios_base::binary);
    if (input_stream.fail()) {
        throw std::runtime_error("Failed to open file");
    }
    std::stringstream buffer;
    buffer << input_stream.rdbuf();
    return buffer.str();
}

}

int main(int argc, char** argv)
{
    if (argc >= 2 && std::string(argv[1]) == "--test") {
        try {
            auto start = std::chrono::steady_clock::now();
            test();
            auto end = std::chrono::steady_clock::now();
            std::cout << "Tests took "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                      << " ms" << std::endl;
        }
        catch (exceptions::Error& error) {
            std::cerr << "Unhandled exception occurred:" << std::endl;
            std::cerr << error.getMessageString() << std::endl;
            std::cerr << "Call stack:\n";
            std::cerr << error.stackTrace << std::endl;
            return 1;
        }
        return 0;
    }
    else if (argc >=2 && exists(std::string(argv[1]))) {
        Machine m;
        eval(m, readFile(std::string(argv[1])), false);
        return 0;
    }
    std::string expr;
    Machine m;
    while (std::getline(std::cin, expr)) {
        eval(m, expr, true);
    }
    return 0;
}
