// The numbering of features created in one go ("P09" x3 -> P09, P10, P11).
// The code splits the trailing digits off the name, and used to do it with
// QString; these cases pin the behaviour.
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "chooselalib.h"
#include "featureattributes.h"
#include "oijob.h"

namespace {

std::vector<std::string> createNames(const std::string& name, int count) {
  oi::OiJob job;
  oi::FeatureAttributes attributes;
  attributes.typeOfFeature = oi::FeatureTypes::ePointFeature;
  attributes.name = name;
  attributes.count = count;
  std::vector<std::string> names;
  for (const auto& feature : job.addFeatures(attributes)) {
    names.push_back(feature->getFeature()->getFeatureName());
  }
  return names;
}

}  // namespace

TEST_CASE("feature names are numbered from the trailing digits") {
  oi::math::ChooseLALib::setLinearAlgebra(
      oi::math::ChooseLALib::LinearAlgebraLibrary::Armadillo);
  using V = std::vector<std::string>;
  CHECK(createNames("P", 1) == V{"P"});
  CHECK(createNames("P09", 3) == V{"P09", "P10", "P11"});
  CHECK(createNames("P009", 3) == V{"P009", "P010", "P011"});
  CHECK(createNames("A", 3) == V{"A01", "A02", "A03"});
  // a single digit always gets a leading zero when several are created
  CHECK(createNames("Point 7", 2) == V{"Point 07", "Point 08"});
  // multi-byte characters before the digits are left alone
  CHECK(createNames("Punkt\xc3\xa4"
                    "5",
                    2) == V{"Punkt\xc3\xa4"
                            "05",
                            "Punkt\xc3\xa4"
                            "06"});
}
