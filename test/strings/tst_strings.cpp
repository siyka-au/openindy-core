// Golden values for the string helpers. The number formats are the ones
// QString::number produced (checked against Qt when this was written), since
// the model's display strings and the codec's text values keep them.
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "stringutil.h"
#include "value.h"

TEST_CASE("number: fixed") {
  CHECK(oi::number(3.14159, 'f', 3) == "3.142");
  CHECK(oi::number(0.0, 'f', 3) == "0.000");
  CHECK(oi::number(-0.0, 'f', 2) == "0.00");
  CHECK(oi::number(-0.001, 'f', 2) == "-0.00");
  CHECK(oi::number(9.995, 'f', 2) == "9.99");  // lies just below the tie
  CHECK(oi::number(1e22, 'f', 0) == "10000000000000000000000");
  CHECK(oi::number(1e15, 'f', 2) == "1000000000000000.00");
  CHECK(oi::number(-1234.5678, 'f', 1) == "-1234.6");
  CHECK(oi::number(0.999, 'f', 2) == "1.00");
}

TEST_CASE("number: exact ties round away from zero like Qt, not to even") {
  CHECK(oi::number(2.5, 'f', 0) == "3");
  CHECK(oi::number(0.5, 'f', 0) == "1");
  CHECK(oi::number(1.5, 'f', 0) == "2");
  CHECK(oi::number(-2.5, 'f', 0) == "-3");
  CHECK(oi::number(0.125, 'f', 2) == "0.13");
  CHECK(oi::number(0.0625, 'f', 3) == "0.063");
  CHECK(oi::number(0.375, 'f', 2) == "0.38");
  CHECK(oi::number(9.5, 'f', 0) == "10");
  CHECK(oi::number(99.5, 'f', 0) == "100");
  CHECK(oi::number(2.5, 'g', 1) == "3");
  CHECK(oi::number(25.0, 'g', 1) == "3e+01");
  CHECK(oi::number(9.5, 'g', 1) == "1e+01");
  CHECK(oi::number(1.25, 'e', 1) == "1.3e+00");
  CHECK(oi::number(1.5, 'e', 0) == "2e+00");
}

TEST_CASE("number: general") {
  CHECK(oi::number(0.5, 'g', 1) == "0.5");
  CHECK(oi::number(1e-5) == "1e-05");
  CHECK(oi::number(0.0001) == "0.0001");
  CHECK(oi::number(123456.0) == "123456");
  CHECK(oi::number(1234567.0) == "1.23457e+06");
  CHECK(oi::number(1e6) == "1e+06");
  CHECK(oi::number(100000.0) == "100000");
  CHECK(oi::number(1e100) == "1e+100");
  CHECK(oi::number(0.1 + 0.2, 'g', 17) == "0.30000000000000004");
  CHECK(oi::number(0.1, 'g', 17) == "0.10000000000000001");
  CHECK(oi::number(0.3, 'g', 2) == "0.3");
  CHECK(oi::number(-0.0) == "0");
  CHECK(oi::number(0.0) == "0");
  CHECK(oi::number(1234.5678, 'G', 3) == "1.23E+03");
}

TEST_CASE("number: scientific") {
  CHECK(oi::number(0.00001234, 'e', 3) == "1.234e-05");
  CHECK(oi::number(12345.5, 'e', 4) == "1.2346e+04");
  CHECK(oi::number(255.0, 'E', 2) == "2.55E+02");
  CHECK(oi::number(0.0, 'e', 2) == "0.00e+00");
}

TEST_CASE("number: not finite, integers") {
  CHECK(oi::number(std::numeric_limits<double>::quiet_NaN()) == "nan");
  CHECK(oi::number(-std::numeric_limits<double>::quiet_NaN()) == "nan");
  CHECK(oi::number(std::numeric_limits<double>::infinity()) == "inf");
  CHECK(oi::number(-std::numeric_limits<double>::infinity()) == "-inf");
  CHECK(oi::number(42) == "42");
  CHECK(oi::number(-7) == "-7");
  CHECK(oi::number(std::size_t{12}) == "12");
}

TEST_CASE("toDouble and toInt report failure") {
  CHECK(oi::toDouble("3.5") == 3.5);
  CHECK(oi::toDouble("  -2e3 ") == -2000.0);
  CHECK(oi::toDouble("+1.5") == 1.5);
  CHECK_FALSE(oi::toDouble("").has_value());
  CHECK_FALSE(oi::toDouble("abc").has_value());
  CHECK_FALSE(oi::toDouble("1.5x").has_value());
  CHECK_FALSE(oi::toDouble("1,5").has_value());
  CHECK(oi::toInt("42") == 42);
  CHECK(oi::toInt(" -7 ") == -7);
  CHECK(oi::toInt("ff", 16) == 255);
  CHECK_FALSE(oi::toInt("4.2").has_value());
  CHECK_FALSE(oi::toInt("99999999999").has_value());
  CHECK_FALSE(oi::toInt("").has_value());
}

TEST_CASE("split, join, trim, case") {
  using V = std::vector<std::string>;
  CHECK(oi::split("a.b..c", ".") == V{"a", "b", "", "c"});
  CHECK(oi::split("a.b..c", ".", false) == V{"a", "b", "c"});
  CHECK(oi::split("", ".") == V{""});
  CHECK(oi::split("", ".", false).empty());
  CHECK(oi::split("a, b", ", ") == V{"a", "b"});
  CHECK(oi::split("abc", "") == V{"abc"});
  CHECK(oi::join(V{"a", "b", "c"}, ", ") == "a, b, c");
  CHECK(oi::join(V{}, ",").empty());
  CHECK(oi::trim("  x y \n") == "x y");
  CHECK(oi::toLower("AbC1") == "abc1");
  CHECK(oi::toUpper("AbC1") == "ABC1");
  CHECK(oi::equalsIgnoreCase("Plane", "pLANE"));
  CHECK_FALSE(oi::equalsIgnoreCase("Plane", "Planes"));
  CHECK(oi::compareIgnoreCase("abc", "ABD") < 0);
  CHECK(oi::compareIgnoreCase("abc", "ABC") == 0);
  CHECK(oi::compareIgnoreCase("abc", "AB") > 0);
  CHECK(oi::startsWithIgnoreCase("Point12", "pOINT"));
  CHECK_FALSE(oi::startsWithIgnoreCase("Po", "Point"));
}

TEST_CASE("Value converts like a variant") {
  CHECK_FALSE(oi::Value().isValid());
  CHECK(oi::Value(true).toBool());
  CHECK(oi::Value(2.5).toDouble() == 2.5);
  CHECK(oi::Value(7).toInt() == 7);
  CHECK(oi::Value("3.5").toDouble() == 3.5);
  CHECK(oi::Value("false").toBool() == false);
  CHECK(oi::Value(1.5).toString() == "1.5");
  CHECK(oi::Value(false).toString() == "false");
  CHECK(oi::Value("x") == oi::Value(std::string("x")));
}
