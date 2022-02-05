#define ENABLE_DEBUG_REFCOUNTING
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
    // std::cout << expr << std::endl;
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
        std::cerr << "Exception thrown while evaluating " << expr << std::endl;
        throw;
    }
    // std::cout << " => " << out << std::endl;
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

void testListBasics()
{
    Machine m;
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
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "'()", "nil");
    ASSERT_OUTPUT_EQ(m, "'(1 2 3)", "(1 2 3)");
    ASSERT_OUTPUT_EQ(m, "(quote (+ 1 2))", "(+ 1 2)");
    ASSERT_OUTPUT_EQ(m, "(quote foo)", "foo");
    ASSERT_OUTPUT_EQ(m, "'foo", "foo");
    ASSERT_OUTPUT_EQ(m, "''foo", "'foo");
    ASSERT_OUTPUT_EQ(m, "'(quote foo)", "'foo");
}

void testCarFunction()
{
    Machine m;
    assert(expect<alisp::exceptions::WrongTypeArgument>([&]() {
        m.evaluate("(car 1)");
    }));
    assert(expect<alisp::exceptions::WrongTypeArgument>([&]() {
        m.evaluate("(car (+ 1 1))");
    }));
    assert(m.evaluate("(car nil)")->toString() == NilName);
    assert(m.evaluate("(car ())")->toString() == NilName);
    assert(m.evaluate("(car '())")->toString() == NilName);
    assert(m.evaluate("(car '(1 2))")->toString() == "1");
    ASSERT_EXCEPTION(m, "(car (1 2))", exceptions::Error);
    assert(m.evaluate("(car '(1 2))")->toString() == "1");
    assert(m.evaluate("(car '((1 2)))")->toString() == "(1 2)");
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
    ASSERT_OUTPUT_EQ(m, R"code((format "test"))code", R"code("test")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "a%%b"))code", R"code("a%b")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%d" 15))code", R"code("15")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%5d" 15))code", R"code("   15")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%015d" 30))code", R"code("000000000000030")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%0000015d" 30))code", R"code("000000000000030")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%s" "cabra"))code", R"code("cabra")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "%S" "cabra"))code", R"code(""cabra"")code");
    ASSERT_OUTPUT_EQ(m, R"code((format "num: %d.%%" 50))code", R"code("num: 50.%")code");
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
    ASSERT_OUTPUT_EQ(m, R"code((elt "aジb" 1))code", R"code(12472)code");
    ASSERT_EXCEPTION(m, R"code((elt "aジb" 3))code", alisp::exceptions::Error);
    ASSERT_EXCEPTION(m, R"code((elt "" 0))code", alisp::exceptions::Error);
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
    ASSERT_OUTPUT_EQ(m, R"code((parse-integer "1"))code", "1\n1");
    ASSERT_OUTPUT_EQ(m, R"code((parse-integer "123"))code", "123\n3");
    ASSERT_OUTPUT_EQ(m, R"code((string-to-number "3"))code", "3");
    ASSERT_OUTPUT_CONTAINS(m, R"code((string-to-number "3.5"))code", "3.5");
    ASSERT_OUTPUT_EQ(m, R"code((string-to-number "X256"))code", "0");
}

void testDivision()
{
    alisp::Machine m;
    assert(m.evaluate("(/ 10 2)")->toString() == "5");
    // Same as emacs: one float arg means that also preceding integer divisions
    // are handled as floating point operations.
    assert(std::abs(m.evaluate("(/ 10 3 3.0)")->value<double>() - 1.11111111) < 0.001);
    assert(expect<alisp::exceptions::ArithError>([&]() {
        m.evaluate("(/ 1 0)");
    }));
    assert(expect<alisp::exceptions::ArithError>([&]() {
        m.evaluate("(/ 1 0.0)");
    }));
}

void testCdrFunction()
{
    Machine m;
    ASSERT_OUTPUT_EQ(m, "(cdr '(a b c))", "(b c)");
    ASSERT_OUTPUT_EQ(m, "(cdr '(a))", "nil");
    ASSERT_OUTPUT_EQ(m, "(cdr '())", "nil");
    ASSERT_OUTPUT_EQ(m, "(cdr (cdr '(a b c)))", "(c)");
    ASSERT_EXCEPTION(m, "(cdr 1)", alisp::exceptions::WrongTypeArgument);
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
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "'(;comment\n1)", "(1)");
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
    ASSERT_OUTPUT_EQ(m, "(set 'y 15)", "15");
    ASSERT_OUTPUT_EQ(m, "(progn (setq x 1) (let (x z) (setq x 2) "
                     "(setq z 3) (setq y x)) (list x y))", "(1 2)");
    ASSERT_OUTPUT_EQ(m, "(setq x 1) ; Put a value in the global binding.", "1");
    ASSERT_EXCEPTION(m, "(let ((x 2)) (makunbound 'x) x)", alisp::exceptions::VoidVariable);
    ASSERT_OUTPUT_EQ(m, "x ; The global binding is unchanged.", "1");
    
    ASSERT_EXCEPTION(m, R"code(
(let ((x 2))             ; Locally bind it.
  (let ((x 3))           ; And again.
    (makunbound 'x)      ; Void the innermost-local binding.
    x))                  ; And refer: it’s void.
)code", alisp::exceptions::VoidVariable);

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
    ASSERT_OUTPUT_EQ(m, "'('a 'b)", "('a 'b)");
    ASSERT_OUTPUT_EQ(m, "'('a'b)", "('a 'b)");
    ASSERT_OUTPUT_EQ(m, "(symbolp 'abc)", "t");
    ASSERT_OUTPUT_EQ(m, "(symbol-name 'abc)", "\"abc\"");
    assert(expect<alisp::exceptions::VoidVariable>([&]() {
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
    ASSERT_EXCEPTION(m, "(message \"%d\" sym)", alisp::exceptions::Error);
    ASSERT_OUTPUT_EQ(m, "(unintern sym)", "t"); // this removes abra from objarray
    ASSERT_EXCEPTION(m, "(message \"%d\" ABRA)", alisp::exceptions::VoidVariable);
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable sym)", "ABRA's value is 500");
}

void testDescribeVariableFunction()
{
    alisp::Machine m;
    ASSERT_EXCEPTION(m, "(describe-variable a)", alisp::exceptions::VoidVariable);
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 'a)", "a is void as a variable");
    m.evaluate("(setq a 12345)");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 'a)", "12345");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable nil)", "nil's value is nil");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable t)", "t's value is t");
    ASSERT_OUTPUT_CONTAINS(m, "(describe-variable 't)", "t's value is t");
}

void testBasicArithmetic()
{
    alisp::Machine m;
    ASSERT_OUTPUT_EQ(m, "-1", "-1");
    ASSERT_OUTPUT_EQ(m, "(<= 2.1 2)", "nil");
    ASSERT_OUTPUT_EQ(m, "(<= 1 2)", "t");
    ASSERT_OUTPUT_EQ(m, "(<= 2 2)", "t");
    ASSERT_OUTPUT_EQ(m, "(< 2 2)", "nil");
    ASSERT_OUTPUT_EQ(m, "(<= 1 2 3)", "t");
    ASSERT_OUTPUT_EQ(m, "(<= 1 2 3 4.0)", "t");
    ASSERT_OUTPUT_EQ(m, "(<= 1 2 3 4.0 3)", "nil");
    ASSERT_OUTPUT_EQ(m, "(% 5 2)", "1");
    ASSERT_EXCEPTION(m, "(% 5 2.0)", alisp::exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(+ 1 1)", "2");
    ASSERT_OUTPUT_EQ(m, "(+)", "0");
    ASSERT_OUTPUT_EQ(m, "(* 3 4)", "12");
    ASSERT_OUTPUT_EQ(m, "(*)", "1");
    ASSERT_OUTPUT_EQ(m, "(+ 1 -1)", "0");
    ASSERT_OUTPUT_EQ(m, "(1+ 0)", "1");
    ASSERT_OUTPUT_CONTAINS(m, "(1+ 0.0)", "1.0");
    ASSERT_EXCEPTION(m, "(1+ \"a\")", alisp::exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_CONTAINS(m, "(+ +.1 -0.1)", "0.0");
    ASSERT_OUTPUT_EQ(m, "(= 1 1)", "t");
    ASSERT_OUTPUT_EQ(m, "(= 1.0 1)", "t");
    ASSERT_OUTPUT_EQ(m, "(= 1 1.0)", "t");
    ASSERT_OUTPUT_EQ(m, "(= 1 1.0 1.0)", "t");
    ASSERT_OUTPUT_EQ(m, "(= 1 1.0 1.0 1.0)", "t");
    ASSERT_OUTPUT_EQ(m, "(= 1 2)", "nil");
    ASSERT_EXCEPTION(m, "(truncate 1 0)", alisp::exceptions::ArithError);
    ASSERT_OUTPUT_EQ(m, "(truncate 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(truncate 1.1)", "1");
    ASSERT_OUTPUT_EQ(m, "(truncate -1.2)", "-1");
    ASSERT_OUTPUT_EQ(m, "(truncate 19.5 3.2)", "6");
    ASSERT_OUTPUT_EQ(m, "(truncate 5.999 nil)", "5");
    ASSERT_OUTPUT_EQ(m, "(ceiling -1.5)", "-1");
    ASSERT_OUTPUT_EQ(m, "(floor -1.5)", "-2");
    ASSERT_OUTPUT_EQ(m, "(floor 1.5)", "1");
    ASSERT_OUTPUT_EQ(m, "(ceiling 2)", "2");
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
    ASSERT_OUTPUT_EQ(m, "(macroexpand '(setf (car x) 7))", "(let* ((v x)) (setcar v 7))");
    ASSERT_OUTPUT_EQ(m, "(macroexpand 1)", "1");
    ASSERT_OUTPUT_EQ(m, "(macroexpand nil)", "nil");
    ASSERT_OUTPUT_EQ(m, "(macroexpand '(inc r))", "(set 'r (1+ r))");
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
}

void testLet()
{
    alisp::Machine m;
    ASSERT_EXCEPTION(m, "(let (1) nil)", alisp::exceptions::WrongTypeArgument);
    ASSERT_OUTPUT_EQ(m, "(let ((x 1) (y (+ 1 2))) (format \"%d\" x) (+ x y))", "4");
    ASSERT_OUTPUT_EQ(m, "(let* ((x 1) (y x)) y)", "1");
    ASSERT_EXCEPTION(m, "(let ((x 1) (y x)) y)", alisp::exceptions::VoidVariable);
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
    assert(!alisp::makeList(&m)->cc->isCyclical());
    ASSERT_OUTPUT_EQ(m,
                     "(progn (set 'z (list 1 2 3))(setcdr (cdr (cdr z)) (cdr z)) z)",
                     "(1 2 3 2 . #2)");
    ASSERT_EXCEPTION(m, "(length z)", alisp::exceptions::Error);
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
    m.evaluate("nil")->asList()->cc->traverse([](const alisp::ConsCell* cell) {
        assert(false && "Traversing empty list...");
        return false;
    });
    /*
      https://www.gnu.org/software/emacs/manual/html_node/elisp/Special-Read-Syntax.html
     */
    ASSERT_OUTPUT_EQ(m, "(setq x (list 1 2 3))", "(1 2 3)");
    ASSERT_OUTPUT_EQ(m, "(setcar x x)", "(#0 2 3)");
}

void testMemoryLeaks()
{
    using namespace alisp;
    std::unique_ptr<Machine> m = std::make_unique<Machine>();
    assert(Object::getDebugRefCount() > 0);
    const int baseCount = Object::getDebugRefCount();
    ASSERT_EXCEPTION(*m, "(pop nil)", alisp::exceptions::Error);
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
        assert(obj->equals(*m->getSymbolOrNull(zname)->variable));
        assert(obj->equals(*m->evaluate(zname.c_str())));
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

void test()
{
    testFunctions();
    testListBasics();
    testNullFunction();
    testCarFunction();
    testConverter();
    testBasicArithmetic();
    testControlStructures();
    testVariables();
    testMemoryLeaks();
    testCyclicals(); // Lot of work to do here still...
    testMacros();
    testLet();
    testQuote();
    testSymbols();
    testIf();
    testDeepCopy();
    testEvalFunction();
    testCdrFunction();
    testConsFunction();
    testListFunction();
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
    catch (alisp::exceptions::Exception& ex) {
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
        test();
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
