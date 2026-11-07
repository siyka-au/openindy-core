#!/bin/sh
# Fails if openindy-core or openindy-math (include, src, test, tools, CMake,
# conanfile) mention Qt in code: includes, Qt-style identifiers (QString,
# QObject, Q_OBJECT, qobject_cast, ...), Qt CMake packages or the Qt namespace.
# Comments and string literals that merely talk about Qt are allowed only on
# lines marked with "no-qt-check: ok" (there are none today besides this
# script's own patterns).
#
# usage: scripts/check-no-qt.sh [core-dir [math-dir]]
core=${1:-$(cd "$(dirname "$0")/.." && pwd)}
math=${2:-$core/../openindy-math}

pattern='#[[:space:]]*include[[:space:]]*[<"]Q|\bQ[A-Z][a-z][A-Za-z0-9]*\b|\bQ_[A-Z_]+\b|\bQt[0-9]*::|\bqobject_cast\b|\bq(Debug|Warning|Critical|Sqrt|Abs|Pow|Cos|Sin)\b|\bemit\b|\bqFatal\b|\bqInfo\b|find_package\([[:space:]]*Qt|\bAUTOMOC\b|\bAUTOUIC\b|\bAUTORCC\b|Qt6::|Qt5::'

status=0
for root in "$core" "$math"; do
  [ -d "$root" ] || continue
  for sub in include src test tools scripts; do
    [ -d "$root/$sub" ] || continue
    # the script itself names the patterns; skip it
    hits=$(grep -rnE --include='*.h' --include='*.hpp' --include='*.cpp' --include='*.cc' \
                 --include='*.txt' --include='*.cmake' --include='*.in' \
                 "$pattern" "$root/$sub" 2>/dev/null | grep -v 'no-qt-check: ok' | \
           grep -v '/check-no-qt.sh:')
    # lines that are comments only are not code
    hits=$(printf '%s\n' "$hits" | grep -vE '^[^:]+:[0-9]+:[[:space:]]*(//|\*|/\*|#[[:space:]]|//!)' | grep -v '^$')
    if [ -n "$hits" ]; then
      printf '%s\n' "$hits"
      status=1
    fi
  done
  for f in CMakeLists.txt conanfile.txt; do
    if [ -f "$root/$f" ]; then
      hits=$(grep -nE "$pattern" "$root/$f" | grep -vE '^[0-9]+:[[:space:]]*#' | grep -v 'no-qt-check: ok')
      if [ -n "$hits" ]; then
        printf '%s\n' "$hits" | sed "s|^|$root/$f:|"
        status=1
      fi
    fi
  done
done
if [ "$status" -ne 0 ]; then
  echo "check-no-qt: Qt references found in openindy-core/openindy-math (see above)" >&2
else
  echo "check-no-qt: ok"
fi
exit $status
