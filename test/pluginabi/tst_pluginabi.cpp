#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "constructfunction.h"
#include "exchangeinterface.h"
#include "fitfunction.h"
#include "generatefeaturefunction.h"
#include "geodeticfunction.h"
#include "lasertracker.h"
#include "objecttransformation.h"
#include "plugin.h"
#include "pluginabi.h"
#include "sha256.h"
#include "specialfunction.h"
#include "stringutil.h"
#include "systemtransformation.h"
#include "totalstation.h"

namespace {

namespace fs = std::filesystem;

/*!
 * Guards the plugin ABI version declared in include/plugin/pluginabi.h.
 *
 * A hand-declared version number is only as good as the discipline that
 * maintains it, and the dangerous failure is not a spurious bump - it is a
 * MISSED one. A missed bump lets a plugin binary built against the old
 * contract load successfully (the IID still matches) and then call through a
 * vtable that has moved underneath it. That is memory corruption, at runtime,
 * on a user's machine, with no diagnostic.
 *
 * So this test fingerprints the declared interface surface - every header in
 * include/plugin/ - and fails when it changes without the version moving.
 * The digest is stored alongside the version it belongs to, so refreshing it
 * without bumping is a visible, deliberate act in the diff rather than
 * something that happens silently.
 *
 * Comments and blank lines are excluded from the fingerprint, so documenting
 * an interface does not trip the guard. Everything else does, deliberately -
 * this errs toward false positives, because a false positive costs one
 * command and a false negative costs a corrupted measurement session.
 *
 * WHAT IT DOES NOT COVER: layout changes to domain types that cross the
 * boundary but live outside include/plugin/ (Geometry, Position, Reading,
 * Observation, ...). Those are just as ABI-breaking. Judgement is still
 * required; see the bump criteria in pluginabi.h.
 */

//! Strips comments and insignificant whitespace so that documentation changes
//! do not read as interface changes.
std::string normalize(const std::string& source) {
  // comments out; a "//" inside a block comment is part of the block, and a
  // "/*" inside a line comment is part of the line
  std::string text;
  for (std::size_t i = 0; i < source.size();) {
    if (source.compare(i, 2, "/*") == 0) {
      const std::size_t end = source.find("*/", i + 2);
      i = end == std::string::npos ? source.size() : end + 2;
    } else if (source.compare(i, 2, "//") == 0) {
      const std::size_t end = source.find('\n', i);
      i = end == std::string::npos ? source.size() : end;
    } else {
      text.push_back(source[i]);
      ++i;
    }
  }

  std::vector<std::string> significantLines;
  for (const std::string& rawLine : oi::split(text, "\n")) {
    // whitespace runs collapse to one space, the ends are trimmed
    std::string line;
    for (const char c : oi::trim(rawLine)) {
      const bool space =
          c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
      if (space && !line.empty() && line.back() == ' ') {
        continue;
      }
      line.push_back(space ? ' ' : c);
    }
    if (!line.empty()) {
      significantLines.push_back(line);
    }
  }

  return oi::join(significantLines, "\n");
}

//! hex sha256 over every header in include/plugin/ except pluginabi.h; the
//! hashed file names are returned in the order hashed
std::string digestOfInterfaceHeaders(std::vector<std::string>& filesOut) {
  const fs::path pluginIncludeDir = fs::path(SOURCEDIR) / "include" / "plugin";

  std::vector<std::string> headers;
  for (const fs::directory_entry& entry :
       fs::recursive_directory_iterator(pluginIncludeDir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".h") {
      continue;
    }
    const std::string relative =
        entry.path().lexically_relative(pluginIncludeDir).generic_string();
    // pluginabi.h itself is excluded: bumping the version edits it, which
    // would change the digest on every bump and make the pairing circular
    if (relative == "pluginabi.h") {
      continue;
    }
    headers.push_back(relative);
  }

  // sorted, so the digest does not depend on filesystem iteration order
  std::ranges::sort(headers);

  std::string hashed;
  for (const std::string& relative : headers) {
    std::ifstream file(pluginIncludeDir / relative);
    if (!file) {
      return {};
    }
    std::stringstream contents;
    contents << file.rdbuf();
    // the file name participates, so renaming or moving an interface header
    // counts as a change even when its contents are untouched
    hashed += relative;
    hashed += normalize(contents.str());
  }

  filesOut = headers;
  return oi::codec::sha256Hex(hashed);
}

}  // namespace

TEST_CASE("interface headers match the declared ABI version") {
  std::vector<std::string> hashedFiles;
  const std::string actualDigest = digestOfInterfaceHeaders(hashedFiles);
  REQUIRE_FALSE(actualDigest.empty());
  REQUIRE_FALSE(hashedFiles.empty());

  const std::string digestPath =
      (fs::path(SOURCEDIR) / "plugin-abi.digest").string();

  std::string recordedVersion;
  std::string recordedDigest;
  if (std::ifstream digestFile(digestPath); digestFile) {
    digestFile >> recordedVersion >> recordedDigest;
  }

  const std::string currentVersion = oi::number(OI_PLUGIN_ABI_VERSION);
  const std::string expectedFileContents =
      std::format("{} {}\n", currentVersion, actualDigest);

  if (recordedVersion == currentVersion && recordedDigest == actualDigest) {
    return;
  }

  // write what the file should say, so the fix is a copy rather than a
  // transcription of a 64-character hex string from a test log
  const std::string suggestionPath =
      (fs::current_path() / "plugin-abi.digest.actual").string();
  if (std::ofstream suggestion(suggestionPath); suggestion) {
    suggestion << expectedFileContents;
  }

  FAIL(std::format(
      "\n"
      "The plugin interface headers no longer match the recorded fingerprint "
      "for\n"
      "plugin ABI version {0}.\n"
      "\n"
      "  recorded: version {1}, digest {2}\n"
      "  actual:   version {0}, digest {3}\n"
      "\n"
      "If you changed the plugin contract (see the bump criteria in\n"
      "include/plugin/pluginabi.h), bump OI_PLUGIN_ABI_VERSION and then "
      "run:\n"
      "\n"
      "    cp {4} {5}\n"
      "\n"
      "If the change genuinely cannot break an existing plugin binary, "
      "refresh\n"
      "the digest with the same command without bumping - but do that "
      "knowingly,\n"
      "because the diff is the only record that you decided it was safe.\n",
      currentVersion,
      recordedVersion.empty() ? "<none recorded>" : recordedVersion,
      recordedDigest.empty() ? "<none recorded>" : recordedDigest, actualDigest,
      suggestionPath, digestPath));
}

/*!
 * Every plugin interface must key its IID off the ABI version. One that does
 * not would keep a constant IID across a genuine ABI break - it would keep
 * loading, and keep being wrong.
 */
TEST_CASE("every interface IID carries the ABI version") {
  const std::string suffix = oi::number(OI_PLUGIN_ABI_VERSION);

  // a representative IID from each interface family
  const std::map<std::string, std::string> iids = {
      {"Plugin", Plugin_iidd},
      {"Sensor", Sensor_iidd},
      {"LaserTracker", LaserTracker_iidd},
      {"TotalStation", TotalStation_iidd},
      {"Function", Function_iid},
      {"FitFunction", FitFunction_iidd},
      {"ConstructFunction", ConstructFunction_iidd},
      {"SystemTransformation", SystemTransformation_iidd},
      {"ObjectTransformation", ObjectTransformation_iidd},
      {"GeodeticFunction", GeodeticFunction_iidd},
      {"SpecialFunction", SpecialFunction_iidd},
      {"GenerateFeatureFunction", GenerateFeatureFunction_iidd},
      {"Simulation", Simulation_iidd},
      {"BundleAdjustment", BundleAdjustment_iidd},
      {"Exchange", Exchange_iidd},
      {"ExchangeSimpleAscii", ExchangeSimpleAscii_iidd},
      {"ExchangeDefinedFormat", ExchangeDefinedFormat_iidd}};

  std::set<std::string> distinct;
  for (const auto& [name, iid] : iids) {
    INFO(name << "'s IID is " << iid);
    CHECK(iid.ends_with(suffix));
    CHECK(iid.starts_with("de.openIndy.plugin"));
    distinct.insert(iid);
  }

  // and they must be distinct, or two interface categories would be
  // mutually castable
  CHECK(distinct.size() == iids.size());
}
