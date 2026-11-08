// A valid shared library that is not an OpenIndy plugin: no entry points.
extern "C" int oi_test_unrelated_symbol() { return 1; }
