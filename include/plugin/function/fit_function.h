#ifndef OI_CORE_PLUGIN_FUNCTION_FIT_FUNCTION_H_
#define OI_CORE_PLUGIN_FUNCTION_FIT_FUNCTION_H_
#include <math.h>

#include <array>
#include <cmath>
#include <iterator>
#include <random>
#include <utility>
#include <vector>

#include "containers.h"
#include "core_export.h"
#include "function.h"
#include "snapshot.h"

namespace oi {

//! Constants of the best-fit algorithms.
namespace fit {

//! The plane fit is singular when the middle eigenvalue of the scatter matrix
//! is smaller than this fraction of the largest one.
inline constexpr double kSingularEigenvalueRatio = 1.0e-12;
//! Unknowns of the cylinder adjustment: radius, X0, Y0, alpha, beta.
inline constexpr int kCylinderUnknowns = 5;
//! Unknowns of the circle adjustment: centre x, centre y, radius.
inline constexpr int kCircleUnknowns = 3;
//! The in-plane geometric circle fit stops when the largest Gauss-Newton
//! correction (centre x, centre y, radius), relative to the radius, falls
//! below this.
inline constexpr double kCircleGeometricTolerance = 1.0e-12;
//! The in-plane geometric circle fit fails when it has not converged after
//! this many Gauss-Newton iterations.
inline constexpr int kCircleGeometricMaxIterations = 100;
//! Independent condition equations of a spatial circle fit per point: a point
//! lies on the sphere about the centre and in the plane of the circle.
inline constexpr int kCircleEquationsPerPoint = 2;
//! Parameters of a spatial circle (centre 3, normal 2, radius 1).
inline constexpr int kSpatialCircleParameters = 6;
//! First step length of the cylinder adjustment's Armijo line search (it is
//! halved before the first trial, so the first trial is the full step).
inline constexpr double kArmijoInitialStep = 2;
//! The cylinder iteration stops when the squared correction falls below this.
inline constexpr double kMinSquaredCorrection = 0.0000000000001;
//! The cylinder iteration gives up after this many iterations.
inline constexpr int kMaxIterations = 1000;
//! Two angles (radian) closer than this are treated as equal.
inline constexpr double kAngleEqualityTolerance = 0.001;

}  // namespace fit

template <typename T>
int sgn(T val) {
  return (T(0) < val) - (val < T(0));
}

struct IdPoint {
  int id{};
  OiVec xyz;
};
inline bool operator==(const IdPoint& p1, const IdPoint& p2) {
  return p1.id == p2.id && p1.xyz == p2.xyz;
}

class CylinderApproximation {
 public:
  double approxRadius{};
  double approxXm{};
  double approxYm{};
  double approxAlpha{};
  double approxBeta{};
  double stdev{};
  std::string comment;
};

enum class ApproximationTypes {
  eFirstTwoPoints = 0,
  eDirection,
  eGuessAxis,
  eFirstTwoDummyPoint
};

/*!
 * \brief The FitFunction class
 * Function that solves geometries by fitting them using observations
 */
class OI_CORE_EXPORT FitFunction : public Function {
  friend class BestFitCircleUtil;
  friend class BestFitCylinderUtil;

 public:
  explicit FitFunction() : Function() {
    // needed elements are only observations
    NeededElement element;
    element.typeOfElement = ElementTypes::eObservationElement;
    element.description = std::string(
        "Select as many observations as you want to be used to fit the target "
        "geometry.");
    element.infinite = true;
    this->neededElements.clear();
    this->neededElements.push_back(element);
  }

  ~FitFunction() override {}
};

// D12/domain-model.md S3: replaces the old InputElementKey::eDummyPoint
// sentinel (10000) doubling as the map position. Position in a function's
// inputElements map is now always the NeededElement's real list index, so
//"where is the dummy point" is answered by searching for the role name
// instead of a magic constant - this is the same query shape tag
// resolution uses elsewhere, just scoped to one function's own declaration.
inline int dummyPointPosition(Function* function) {
  const std::vector<NeededElement>& needed = function->getNeededElements();
  for (int i = 0; i < std::ssize(needed); ++i) {
    if (needed.at(i).roleName == "dummyPoint") {
      return i;
    }
  }
  return -1;
}

class OI_CORE_EXPORT BestFitUtil : public FunctionUtil {
 protected:
  bool hasDummyPoint(FitFunction* function) {
    const int pos = dummyPointPosition(function);
    return pos >= 0 && function->getInputElements().contains(pos) &&
           function->getInputElements().at(pos).size() > 0;
  }

  bool getDummyPoint(OiVec& dummyPoint, FitFunction* function) {
    if (!hasDummyPoint(function)) {
      return false;
    }

    const int pos = dummyPointPosition(function);
    ElementTypes elementType =
        function->getNeededElements().at(pos).typeOfElement;
    InputElement inputElement = function->getInputElements().at(pos).at(0);
    OiVec dp;
    switch (elementType) {
      case ElementTypes::eObservationElement:
        dp = inputElement.observation->getXYZ();
        dp.removeLast();
        dummyPoint = dp;
        break;
      case ElementTypes::ePointElement:
        dummyPoint = inputElement.point->getPosition().getVector();
        break;
      default:
        return false;
    }
    return true;
  }

  bool normaleFromDummyPoint(OiVec& direction, FitFunction* function,
                             const OiVec& point) {
    OiVec dummyPoint(3);
    if (!getDummyPoint(dummyPoint, function)) {
      return false;
    }

    // the circle normal points from the circle centre towards the dummy
    // point, as for BestFitPlane. (The former -dot(dummy - c, c) * dummy
    // depended on where the origin is.)
    direction = dummyPoint - point;
    direction.normalize();

    return true;
  }
};

class OI_CORE_EXPORT BestFitPlaneUtil : public BestFitUtil {
 protected:
  bool bestFitPlane(OiVec& centroid, OiVec& normal, double& eVal,
                    std::vector<IdPoint> points) {
    if (points.size() == 0) {
      return false;
    }

    // centroid
    OiVec mean(3);
    for (const IdPoint& point : oi::snapshot(points)) {
      OiVec xyz = point.xyz;
      if (xyz.getSize() == 4) {
        xyz.removeLast();
      }
      mean = mean + xyz;
    }
    mean = mean * (1.0 / static_cast<double>(points.size()));

    centroid = mean;

    // principle component analysis
    OiMat a(points.size(), 3);
    for (int i = 0; i < std::ssize(points); i++) {
      a.setAt(i, 0, points.at(i).xyz.getAt(0) - centroid.getAt(0));
      a.setAt(i, 1, points.at(i).xyz.getAt(1) - centroid.getAt(1));
      a.setAt(i, 2, points.at(i).xyz.getAt(2) - centroid.getAt(2));
    }
    OiMat ata = a.t() * a;
    OiMat u(3, 3);
    OiVec d(3);
    OiMat v(3, 3);
    ata.svd(u, d, v);

    // get smallest eigenvector which is n vector
    int eigenIndex = -1;
    for (int i = 0; i < d.getSize(); i++) {
      if (d.getAt(i) < eVal || i == 0) {
        eVal = d.getAt(i);
        eigenIndex = i;
      }
    }

    // a plane needs the points to actually span two dimensions. If they
    // are collinear (or coincident) then ata is rank-deficient, *two*
    // eigenvalues are ~0, and "the eigenvector of the smallest
    // eigenvalue" picks arbitrarily between them - any plane containing
    // the line fits equally well, so the normal returned is noise.
    // Without this check that garbage was returned as success.
    double largest = 0.0;
    double middle = 0.0;
    for (int i = 0; i < d.getSize(); i++) {
      const double value = std::abs(d.getAt(i));
      if (value > largest) {
        middle = largest;
        largest = value;
      } else if (value > middle) {
        middle = value;
      }
    }

    // relative, because these are sums of squared deviations and so
    // scale with the extent of the data. Deliberately conservative: this
    // rejects the genuinely singular case, not merely ill-conditioned
    // input, so it cannot start failing fits that used to work.
    if (largest <= 0.0 || (middle / largest) < fit::kSingularEigenvalueRatio) {
      return false;
    }

    u.getCol(normal, eigenIndex);
    normal.normalize();

    return true;
  }

  void addDisplayResidual(FitFunction* function,
                          std::vector<InputElement>& elements, OiVec normal,
                          double dist) {
    double distance = 0.0;
    OiVec v_plane(3);
    for (const InputElement& element : oi::snapshot(elements)) {
      if (!element.observation.isNull() && element.observation->getIsSolved() &&
          element.observation->getIsValid()) {  // calculating residuals even if
                                                // observation not in use

        // calculate residual vector
        distance = normal.getAt(0) * element.observation->getXYZ().getAt(0) +
                   normal.getAt(1) * element.observation->getXYZ().getAt(1) +
                   normal.getAt(2) * element.observation->getXYZ().getAt(2) -
                   dist;
        v_plane = distance * normal;

        // set up display residual
        function->addDisplayResidual(element.observation->getId(),
                                     v_plane.getAt(0), v_plane.getAt(1),
                                     v_plane.getAt(2), distance);
      }
    }
  }
};

class OI_CORE_EXPORT BestFitCircleUtil : public BestFitUtil {
 protected:
  //! Orthogonal-distance (geometric) refinement of a 2D circle: Gauss-Newton
  //! on the radial residuals r_i = |p_i - c| - R with the parameters
  //! (cx, cy, R), starting from (cx, cy, radius). Returns false when the
  //! iteration does not converge or the normal equations are singular.
  static bool refineCircleGeometric(const std::vector<OiVec>& pts, double& cx,
                                    double& cy, double& radius) {
    for (int iteration = 0; iteration < fit::kCircleGeometricMaxIterations;
         ++iteration) {
      double nn[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
      double rhs[3] = {0, 0, 0};
      for (const OiVec& p : pts) {
        const double dx = p.getAt(0) - cx;
        const double dy = p.getAt(1) - cy;
        const double dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= 0.0) {
          return false;  // a point on the centre has no radial direction
        }
        const double j[3] = {-dx / dist, -dy / dist, -1.0};
        const double r = dist - radius;
        for (int a = 0; a < 3; ++a) {
          rhs[a] -= j[a] * r;
          for (int b = 0; b < 3; ++b) {
            nn[a][b] += j[a] * j[b];
          }
        }
      }
      OiMat normal(3, 3);
      OiVec right(3);
      for (int a = 0; a < 3; ++a) {
        right.setAt(a, rhs[a]);
        for (int b = 0; b < 3; ++b) {
          normal.setAt(a, b, nn[a][b]);
        }
      }
      OiVec delta(3);
      try {
        if (!OiMat::solve(delta, normal, right)) {
          return false;
        }
      } catch (const std::exception&) {
        return false;
      }
      cx += delta.getAt(0);
      cy += delta.getAt(1);
      radius += delta.getAt(2);
      if (!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(radius)) {
        return false;
      }
      const double step =
          std::max({std::abs(delta.getAt(0)), std::abs(delta.getAt(1)),
                    std::abs(delta.getAt(2))});
      if (step <= fit::kCircleGeometricTolerance * std::abs(radius)) {
        return true;
      }
    }
    return false;
  }

  //! Plane by principal components, then a circle in the plane. With
  //! `geometric` the circle minimises the squared radial (orthogonal)
  //! distances (nonlinear least squares, started from the algebraic fit);
  //! otherwise it is the algebraic (Kasa) fit, which is linear least squares
  //! on x^2 + y^2 + a x + b y + c = 0 and biased on short arcs.
  bool bestFitCircleInPlane(FitFunction* function, Circle& circle,
                            std::vector<IdPoint> points,
                            std::vector<IdPoint> usablePoints,
                            bool geometric = true) {
    // calculate centroid
    OiVec centroid(4);
    for (const IdPoint& point : std::as_const(points)) {
      centroid = centroid + point.xyz;
    }
    centroid = centroid / (double)points.size();
    centroid.removeLast();

    // principle component analysis
    OiMat a(points.size(), 3);
    for (int i = 0; i < std::ssize(points); i++) {
      a.setAt(i, 0, points.at(i).xyz.getAt(0) - centroid.getAt(0));
      a.setAt(i, 1, points.at(i).xyz.getAt(1) - centroid.getAt(1));
      a.setAt(i, 2, points.at(i).xyz.getAt(2) - centroid.getAt(2));
    }
    OiMat ata = a.t() * a;
    OiMat u(3, 3);
    OiVec d(3);
    OiMat v(3, 3);
    ata.svd(u, d, v);

    // get smallest eigenvector which is n vector
    int eigenIndex = -1;
    double eVal = 0.0;
    for (int i = 0; i < d.getSize(); i++) {
      if (d.getAt(i) < eVal || i == 0) {
        eVal = d.getAt(i);
        eigenIndex = i;
      }
    }
    OiVec n(3);
    u.getCol(n, eigenIndex);
    n.normalize();

    OiVec direction(3);
    if (hasDummyPoint(function)) {
      // computing circle normale by dummy point
      normaleFromDummyPoint(direction, function, centroid);
    } else {
      // check that the normal vector of the plane is defined by the first three
      // points A, B and C (cross product)
      OiVec ab = points.at(1).xyz - points.at(0).xyz;
      ab.removeLast();
      OiVec ac = points.at(2).xyz - points.at(0).xyz;
      ac.removeLast();
      OiVec::cross(direction, ab, ac);
      direction.normalize();
    }
    if (fit::opposesDirection(n, direction)) {
      n = n * -1.0;
    }

    // calculate smallest distance of the plane from the origin
    double dOrigin = n.getAt(0) * centroid.getAt(0) +
                     n.getAt(1) * centroid.getAt(1) +
                     n.getAt(2) * centroid.getAt(2);

    // calculate the distances of each observation from the plane
    /*OiVec planeDistances;
    for(int i = 0; i < inputObservations.size(); i++){

        double distance = std::abs(n.getAt(0) *
    inputObservations.at(i)->getXYZ().getAt(0)
                               + n.getAt(1) *
    inputObservations.at(i)->getXYZ().getAt(1)
                               + n.getAt(2) *
    inputObservations.at(i)->getXYZ().getAt(2) - dOrigin);

        planeDistances.add( distance );

    }*/

    // get transformation matrix
    OiMat t = u.inv();

    // transform centroid into 2D space
    OiVec centroid2D = t * centroid;
    centroid2D.removeLast();

    // calculate centroid reduced coordinates in 2D space
    std::vector<OiVec> centroidReducedCoordinates;
    std::vector<OiVec> allCentroidReducedCoordinates;

    for (const IdPoint& point : oi::snapshot(usablePoints)) {
      OiVec xyz(4);
      xyz = point.xyz;
      xyz.removeLast();  // remove the homogeneous item
      xyz = t * xyz;
      xyz.removeLast();
      OiVec reduced = xyz - centroid2D;
      allCentroidReducedCoordinates.push_back(reduced);
      if (oi::contains(points, point)) {
        centroidReducedCoordinates.push_back(reduced);
      }
    }

    // calculate best fit circle in 2D space
    OiVec A1(points.size());
    OiMat A2(points.size(), 3);
    for (int i = 0; i < std::ssize(points); i++) {
      A1.setAt(i, centroidReducedCoordinates.at(i).getAt(0) *
                          centroidReducedCoordinates.at(i).getAt(0) +
                      centroidReducedCoordinates.at(i).getAt(1) *
                          centroidReducedCoordinates.at(i).getAt(1));

      A2.setAt(i, 0, centroidReducedCoordinates.at(i).getAt(0));
      A2.setAt(i, 1, centroidReducedCoordinates.at(i).getAt(1));
      A2.setAt(i, 2, 1.0);
    }
    OiVec s(3);
    try {
      if (!OiMat::solve(s, A2.t() * A2, -1.0 * A2.t() * A1)) {
        return false;
      }
    } catch (const exception& e) {
      function->sendMessage(e.what(), MessageTypes::eErrorMessage);
      return false;
    }

    // calculate center and radius in 2D space
    OiVec xm(3);
    xm.setAt(0, (-1.0 * s.getAt(0) / 2) + centroid2D.getAt(0));
    xm.setAt(1, (-1.0 * s.getAt(1) / 2) + centroid2D.getAt(1));
    xm.setAt(2, (t * centroid).getAt(2));
    double radius = std::sqrt(
        (s.getAt(0) * s.getAt(0) + s.getAt(1) * s.getAt(1)) / 4 - s.getAt(2));

    if (geometric) {
      // refine in the centroid-reduced frame (better conditioned), starting
      // from the algebraic solution
      double cx = xm.getAt(0) - centroid2D.getAt(0);
      double cy = xm.getAt(1) - centroid2D.getAt(1);
      if (!std::isfinite(radius) ||
          !refineCircleGeometric(centroidReducedCoordinates, cx, cy, radius)) {
        function->sendMessage("The geometric circle fit did not converge.",
                              MessageTypes::eErrorMessage);
        return false;
      }
      xm.setAt(0, cx + centroid2D.getAt(0));
      xm.setAt(1, cy + centroid2D.getAt(1));
    }

    // calculate distance of each observation from the 2D circle
    OiVec circleDistances;
    OiVec allCircleDistances;
    int i = 0;
    for (const IdPoint& point : oi::snapshot(usablePoints)) {
      OiVec reduced = allCentroidReducedCoordinates[i++];
      OiVec diff(2);
      diff.setAt(0, reduced.getAt(0) + centroid2D.getAt(0) - xm.getAt(0));
      diff.setAt(1, reduced.getAt(1) + centroid2D.getAt(1) - xm.getAt(1));

      double dr = diff.length() - radius;

      allCircleDistances.add(dr);
      if (oi::contains(points, point)) {
        circleDistances.add(dr);
      }
    }

    // transform center into 3D space
    OiMat t_inv = t.inv();
    xm = t_inv * xm;

    // display residuals (radial) and the sum of squared out-of-plane distances
    double sumHeights = 0.0;
    for (int i = 0; i < std::ssize(usablePoints); i++) {
      const IdPoint point = usablePoints[i];

      // out-of-plane distance of the point from the fitted plane
      const double height = n.getAt(0) * point.xyz.getAt(0) +
                            n.getAt(1) * point.xyz.getAt(1) +
                            n.getAt(2) * point.xyz.getAt(2) - dOrigin;
      if (oi::contains(points, point)) {
        sumHeights += height * height;
      }

      // display residual: the signed radial distance to the circle
      function->addDisplayResidual(point.id, allCircleDistances.getAt(i));
    }

    // set result
    Position circlePosition;
    circlePosition.setVector(xm);
    Radius circleRadius;
    circleRadius.setRadius(radius);
    Direction circleNormal;
    circleNormal.setVector(n);
    circle.setCircle(circlePosition, circleNormal, circleRadius);

    // set statistic. The squared corrections are the radial distances to the
    // circle plus the out-of-plane distances to the PCA plane. A spatial circle
    // has two independent condition equations per point (on the sphere about
    // the centre, in the plane) and six parameters, so the redundancy is
    // 2n - 6 and stdev = sqrt(vv / (2n - 6)) is an unbiased estimate of the
    // per-coordinate sigma for isotropic noise (checked by simulation in
    // openindy-py/tests/vetting). s0 a posteriori is the same number (unit
    // weights, a priori 1).
    double sumRadial = 0.0;
    for (int i = 0; i < std::ssize(centroidReducedCoordinates); i++) {
      const double v_i = circleDistances.getAt(i);
      sumRadial += v_i * v_i;
    }
    const int redundancy =
        fit::kCircleEquationsPerPoint * static_cast<int>(points.size()) -
        fit::kSpatialCircleParameters;
    double stdev = 0.0;
    if (redundancy > 0) {
      stdev =
          std::sqrt((sumRadial + sumHeights) / static_cast<double>(redundancy));
    } else {
      function->sendMessage(
          "The standard deviation of the circle is undefined without "
          "redundant points.",
          MessageTypes::eWarningMessage);
    }
    function->statistic.setS0APriori(1.0);
    function->statistic.setS0APosteriori(stdev);
    function->statistic.setIsValid(true);
    function->statistic.setStdev(stdev);
    circle.setStatistic(function->statistic);

    return true;
  }
};

class OI_CORE_EXPORT BestFitCylinderUtil {
 private:
  std::vector<CylinderApproximation> approximations;

 protected:
  bool bestFitCylinder(FitFunction* function, Cylinder& cylinder,
                       std::vector<IdPoint> points,
                       std::vector<IdPoint> usablePoints) {
    ApproximationTypes approximationType =
        ApproximationTypes::eFirstTwoPoints;  // default
    if (function->getScalarInputParams().stringParameter.size() > 0) {
      std::string value = oi::value(
          function->getScalarInputParams().stringParameter, "approximation");
      if (value.compare("direction") == 0) {
        approximationType = ApproximationTypes::eDirection;
      } else if (value.compare("guess axis") == 0) {
        approximationType = ApproximationTypes::eGuessAxis;
      } else if (value.compare("first two dummy points") == 0) {
        approximationType = ApproximationTypes::eFirstTwoDummyPoint;
      } else if (value.compare("first two points") == 0) {
        approximationType = ApproximationTypes::eFirstTwoPoints;
      }
    }

    // calculate centroid of all observations
    OiVec centroid(4);
    for (const IdPoint& point : std::as_const(points)) {
      centroid = centroid + point.xyz;
    }
    centroid = centroid / (double)points.size();

    // calculate centroid reduced observations
    std::vector<IdPoint> allReducedInputObservations;
    std::vector<IdPoint> reducedInputObservations;
    for (const IdPoint& obs : std::as_const(usablePoints)) {
      IdPoint rObs;
      rObs.id = obs.id;
      rObs.xyz = obs.xyz - centroid;
      allReducedInputObservations.push_back(rObs);
      if (oi::contains(points, obs)) {
        reducedInputObservations.push_back(rObs);
      }
    }

    // approximate cylinder by 2D circle fit
    if (!this->approximateCylinder(function, cylinder, reducedInputObservations,
                                   approximationType)) {
      function->sendMessage(
          std::format("Error while generating approximations for cylinder "
                      "parameters of cylinder {}",
                      cylinder.getFeatureName()),
          MessageTypes::eErrorMessage);
      return false;
    }

    // filter the best approximation
    CylinderApproximation bestApproximation;
    double bestStdev = numeric_limits<double>::max();
    for (const CylinderApproximation& approximation :
         std::as_const(approximations)) {
      if (approximation.stdev < bestStdev) {
        bestStdev = approximation.stdev;
        bestApproximation = approximation;
      }
    }

    OiMat Ralpha(4, 4);
    OiMat Rbeta(4, 4);
    OiMat Rall(4, 4);

    Ralpha.setAt(0, 0, 1.0);
    Ralpha.setAt(1, 1, std::cos(bestApproximation.approxAlpha));
    Ralpha.setAt(1, 2, -std::sin(bestApproximation.approxAlpha));
    Ralpha.setAt(2, 1, std::sin(bestApproximation.approxAlpha));
    Ralpha.setAt(2, 2, std::cos(bestApproximation.approxAlpha));
    Rbeta.setAt(0, 0, std::cos(bestApproximation.approxBeta));
    Rbeta.setAt(0, 2, std::sin(bestApproximation.approxBeta));
    Rbeta.setAt(1, 1, 1.0);
    Rbeta.setAt(2, 0, -std::sin(bestApproximation.approxBeta));
    Rbeta.setAt(2, 2, std::cos(bestApproximation.approxBeta));

    Rall = Rbeta * Ralpha;
    Rall.setAt(3, 3, 1.0);

    for (auto& reducedInputObservation : reducedInputObservations) {
      reducedInputObservation.xyz = Rall * reducedInputObservation.xyz;
      reducedInputObservation.xyz.setAt(3, 1.0);
    }
    for (auto& allReducedInputObservation : allReducedInputObservations) {
      allReducedInputObservation.xyz = Rall * allReducedInputObservation.xyz;
      allReducedInputObservation.xyz.setAt(3, 1.0);
    }

    bestApproximation.approxAlpha = 0.0;
    bestApproximation.approxBeta = 0.0;

    if (!this->fitCylinder(function, cylinder, reducedInputObservations,
                           allReducedInputObservations, bestApproximation)) {
      function->sendMessage(
          std::format("Error while fitting cylinder {} with solution: {}",
                      cylinder.getFeatureName(), bestApproximation.comment),
          MessageTypes::eErrorMessage);
      return false;
    }

    function->sendMessage(
        std::format("cylinder ({}) best solution: {}",
                    cylinder.getFeatureName(), bestApproximation.comment),
        MessageTypes::eInformationMessage);

    // transform cylinder position (centroid + rotation)
    Position cylinderPosition = cylinder.getPosition();
    OiVec pos = (Rall.inv() * cylinderPosition.getVectorH()) + centroid;
    pos.setAt(3, 1.0);
    cylinderPosition.setVector(pos);

    // transform cylinder axis (centroid + rotation)
    Direction cylinderAxis = cylinder.getDirection();
    OiVec axis = (Rall.inv() * cylinderAxis.getVectorH());
    axis.removeLast();
    axis.normalize();
    cylinderAxis.setVector(axis);

    OiVec direction(3);
    switch (approximationType) {
      case ApproximationTypes::eGuessAxis: {
        break;
      }
      case ApproximationTypes::eDirection: {
        for (const InputElement& element :
             oi::value(function->getInputElements(), 1)) {
          if (!element.geometry.isNull() && element.geometry->getIsSolved() &&
              element.geometry->hasDirection()) {
            direction = element.geometry->getDirection().getVector();
            break;
          }
        }
        break;
      }
      case ApproximationTypes::eFirstTwoDummyPoint: {
        // D12/Stage 7c-ii: the dummyPoint role now resolves to
        // ePointElement (every fit function but BestFitPoint does),
        // not just eObservationElement - read whichever the
        // InputElement actually carries, same as
        // BestFitUtil::getDummyPoint already does for plane/circle.
        std::vector<OiVec> dummyPoints;
        for (const InputElement& element : oi::value(
                 function->getInputElements(), dummyPointPosition(function))) {
          if (!element.observation.isNull() &&
              element.observation->getIsSolved()) {
            dummyPoints.push_back(element.observation->getXYZ());
          } else if (!element.point.isNull() && element.point->getIsSolved()) {
            dummyPoints.push_back(element.point->getPosition().getVectorH());
          }
        }
        if (dummyPoints.size() >= 2) {
          OiVec diff = dummyPoints.at(1) - dummyPoints.at(0);
          diff.removeLast();
          diff.normalize();
          direction = diff;
        }
        break;
      }
      case ApproximationTypes::eFirstTwoPoints: {
        OiVec diff = points.at(1).xyz - points.at(0).xyz;
        diff.removeLast();
        diff.normalize();
        direction = diff;
        break;
      }
    }

    // a zero direction (no usable hint) has dot 0 and leaves the axis alone
    if (fit::opposesDirection(axis, direction)) {
      Direction cylinderDirection = cylinder.getDirection();
      cylinderDirection.setVector(axis * -1.0);
      cylinder.setCylinder(cylinderPosition, cylinderDirection,
                           cylinder.getRadius());
    } else {
      Direction cylinderDirection = cylinder.getDirection();
      cylinderDirection.setVector(axis);
      cylinder.setCylinder(cylinderPosition, cylinderDirection,
                           cylinder.getRadius());
    }

    return true;
  }

  /*!
   * \brief approximateCylinder
   * \param cylinder
   * \param inputObservations
   * \return
   */
  bool approximateCylinder(FitFunction* function, Cylinder& /*cylinder*/,
                           const std::vector<IdPoint>& inputObservations,
                           ApproximationTypes approximationType) {
    // clear current approximations
    this->approximations.clear();

    switch (approximationType) {
      case ApproximationTypes::eGuessAxis: {
        // get the number of observations
        int numPoints = inputObservations.size();

        // calculate centroid of all observations
        OiVec centroid3D(4);
        for (const IdPoint& obs : std::as_const(inputObservations)) {
          centroid3D = centroid3D + obs.xyz;
        }
        centroid3D.removeLast();
        centroid3D = centroid3D / numPoints;

        // set up covariance matrix of all observations
        OiMat H(3, 3);
        for (int k = 0; k < numPoints; k++) {
          IdPoint obs = inputObservations.at(k);
          for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
              H.setAt(
                  i, j,
                  H.getAt(i, j) + (obs.xyz.getAt(i) - centroid3D.getAt(i)) *
                                      (obs.xyz.getAt(j) - centroid3D.getAt(j)));
            }
          }
        }

        // singular value decomposition of H to get the major axis direction of
        // the cylinder observations
        OiMat U(3, 3);
        OiVec d(3);
        OiMat V(3, 3);
        try {
          H.svd(U, d, V);
        } catch (const exception& e) {
          function->sendMessage(
              std::format("SVD error cylinder minimum solution: {}", e.what()),
              MessageTypes::eErrorMessage);
          return false;
        }

        // one of the eigen-vectors is the approximate cylinder axis
        bool foundOneVaildApproximation = false;
        for (int i = 0; i < 3; i++) {
          OiVec pn;  // possible normal vector

          U.getCol(pn, i);  // get eigenvector i

          if (approximateCylinder(function, pn, inputObservations,
                                  std::format("eigenvector {}", i))) {
            foundOneVaildApproximation = true;
          }
        }
        return foundOneVaildApproximation;
        break;
      }
      case ApproximationTypes::eDirection: {
        // ########################
        //  approximation direction
        // ########################

        Direction approxAxis;
        // find first direction
        for (const InputElement& element :
             oi::value(function->getInputElements(), 1)) {
          if (!element.geometry.isNull() && element.geometry->getIsSolved() &&
              element.geometry->hasDirection()) {
            approxAxis = element.geometry->getDirection();
            break;
          }
        }
        // valid direction found
        OiVec an = approxAxis.getVector();
        if (!(an.getAt(0) == 0 && an.getAt(1) == 0 && an.getAt(2) == 0)) {
          return approximateCylinder(function, an, inputObservations,
                                     "approxmation direction");
        } else {
          return false;
        }

        break;
      }
      case ApproximationTypes::eFirstTwoDummyPoint: {
        // D12/Stage 7c-ii: see the identical fix in bestFitCylinder()
        // above - the dummyPoint role resolves to ePointElement now.
        std::vector<OiVec> dummyPoints;
        for (const InputElement& element : oi::value(
                 function->getInputElements(), dummyPointPosition(function))) {
          if (!element.observation.isNull() &&
              element.observation->getIsSolved()) {
            dummyPoints.push_back(element.observation->getXYZ());
          } else if (!element.point.isNull() && element.point->getIsSolved()) {
            dummyPoints.push_back(element.point->getPosition().getVectorH());
          }
        }
        if (dummyPoints.size() < 2) {
          return false;
        }
        OiVec diff = dummyPoints.at(1) - dummyPoints.at(0);
        diff.removeLast();

        return approximateCylinder(function, diff, inputObservations,
                                   "first two dummy points");

        break;
      }
      case ApproximationTypes::eFirstTwoPoints: {
        default:
          // ##############################################################
          // another approximation comes from the first two cylinder points
          // ##############################################################

          OiVec diff =
              inputObservations.at(1).xyz - inputObservations.at(0).xyz;
          diff.removeLast();

          return approximateCylinder(function, diff, inputObservations,
                                     "first two cylinder points");

          break;
      }
    }

    return false;
  }

  bool approximateCylinder(FitFunction* function, OiVec pn,
                           const std::vector<IdPoint>& inputObservations,
                           std::string label) {
    // get the number of observations
    int numPoints = inputObservations.size();

    // init helper variables
    OiVec centroid2D(2);  // centroid of 2D circle

    double a = 0.0, b = 0.0;  // sin + cos of rotation angles
    double a_alpha = 0.0, b_alpha = 0.0, a_beta = 0.0,
           b_beta = 0.0;  // possible rotation angles (check acos + asin)
    double alpha = 0.0, beta = 0.0;  // rotation angles
    double _y = 0.0, _z = 0.0;
    double tx = 0.0, ty = 0.0;  // transformed 2D coordinates
    double x_m = 0.0, y_m = 0.0, radius = 0.0,
           sum_vv = 0.0;  // result parameters in circle fit
    OiVec v;

    // rotation matrices
    OiMat Ralpha(3, 3);
    OiMat Rbeta(3, 3);
    OiMat Rall(3, 3);

    // a matrices for circle adjustment
    OiVec A1(numPoints);
    OiMat A2(numPoints, 3);

    // result vector for circle fit
    OiVec s(3);

    // rotated 2D points
    std::vector<double> points2D_x;
    std::vector<double> points2D_y;

    // rotated centroid reduced 2D points
    std::vector<double> centroidreduced2D_x;
    std::vector<double> centroidreduced2D_y;

    // calculate rotation angles
    if (pn.getAt(1) == 0 && pn.getAt(2) == 0) {
      a_alpha = kPi / 2;
      b_alpha = kPi / 2;
    } else if (pn.getAt(1) == 0) {
      a_alpha = 0;
      b_alpha = 0;
    } else {
      a = std::sqrt(1.0 / (1.0 + std::pow(pn.getAt(2) / pn.getAt(1), 2)));
      b = a * pn.getAt(2) / pn.getAt(1);

      a_alpha = std::asin(a);
      b_alpha = std::acos(b);
    }

    alpha = 0.0;

    // cos and sin are ambiguous, therefor check the right angle here
    if (this->compareAngles(b_alpha, a_alpha)) {
      alpha = a_alpha;
    } else if (this->compareAngles(this->getCorrespondingCos(b_alpha),
                                   a_alpha)) {
      alpha = a_alpha;
    } else if (this->compareAngles(b_alpha,
                                   this->getCorrespondingSin(a_alpha))) {
      alpha = b_alpha;
    } else {
      alpha = this->getCorrespondingCos(b_alpha);
    }

    // transform eigenvector --> y-component should be 0 now
    _y = pn.getAt(1);
    _z = pn.getAt(2);
    pn.setAt(1, _y * std::cos(alpha) - _z * std::sin(alpha));
    pn.setAt(2, _y * std::sin(alpha) + _z * std::cos(alpha));

    if (pn.getAt(0) == 0 && pn.getAt(2) == 0) {
      a_beta = kPi / 2;
      b_beta = kPi / 2;
    } else if (pn.getAt(0) == 0) {
      a_beta = 0;
      b_beta = 0;
    } else {
      a = std::sqrt(1.0 / (1.0 + std::pow(pn.getAt(2) / pn.getAt(0), 2)));
      b = -1.0 * pn.getAt(2) * a / pn.getAt(0);

      a_beta = std::asin(a);
      b_beta = std::acos(b);
    }

    beta = 0.0;

    // cos and sin are ambiguous, therefor check the right angle here
    if (this->compareAngles(b_beta, a_beta)) {
      beta = b_beta;
    } else if (this->compareAngles(this->getCorrespondingCos(b_beta), a_beta)) {
      beta = a_beta;
    } else if (this->compareAngles(b_beta, this->getCorrespondingSin(a_beta))) {
      beta = b_beta;
    } else {
      beta = this->getCorrespondingCos(b_beta);
    }

    // set up rotation matrices
    Ralpha.setAt(0, 0, 1.0);
    Ralpha.setAt(1, 1, std::cos(alpha));
    Ralpha.setAt(1, 2, -std::sin(alpha));
    Ralpha.setAt(2, 1, std::sin(alpha));
    Ralpha.setAt(2, 2, std::cos(alpha));
    Rbeta.setAt(0, 0, std::cos(beta));
    Rbeta.setAt(0, 2, std::sin(beta));
    Rbeta.setAt(1, 1, 1.0);
    Rbeta.setAt(2, 0, -std::sin(beta));
    Rbeta.setAt(2, 2, std::cos(beta));

    Rall = Rbeta * Ralpha;

    // ###########################################
    // circle fit to determine midpoint and radius
    // ###########################################

    // clear transformed point lists
    points2D_x.clear();
    points2D_y.clear();
    centroidreduced2D_x.clear();
    centroidreduced2D_y.clear();

    // reset 2D centroid and recalculate with new rotation
    centroid2D = OiVec(2);

    // rotate observations in XY-plane and calculate 2D centroid
    for (int j = 0; j < numPoints; j++) {
      IdPoint obs = inputObservations.at(j);

      tx = Rall.getAt(0, 0) * obs.xyz.getAt(0) +
           Rall.getAt(0, 1) * obs.xyz.getAt(1) +
           Rall.getAt(0, 2) * obs.xyz.getAt(2);
      ty = Rall.getAt(1, 0) * obs.xyz.getAt(0) +
           Rall.getAt(1, 1) * obs.xyz.getAt(1) +
           Rall.getAt(1, 2) * obs.xyz.getAt(2);

      points2D_x.push_back(static_cast<double>(tx));
      points2D_y.push_back(static_cast<double>(ty));

      centroid2D.setAt(0, centroid2D.getAt(0) + tx);
      centroid2D.setAt(1, centroid2D.getAt(1) + ty);
    }
    centroid2D.setAt(0, centroid2D.getAt(0) / static_cast<double>(numPoints));
    centroid2D.setAt(1, centroid2D.getAt(1) / static_cast<double>(numPoints));

    // calculate centroid reduced 2D coordinates
    for (int j = 0; j < numPoints; j++) {
      centroidreduced2D_x.push_back(
          static_cast<double>(points2D_x.at(j) - centroid2D.getAt(0)));
      centroidreduced2D_y.push_back(
          static_cast<double>(points2D_y.at(j) - centroid2D.getAt(1)));
    }

    // fill a matrices
    for (int j = 0; j < numPoints; j++) {
      tx = centroidreduced2D_x.at(j);
      ty = centroidreduced2D_y.at(j);

      A1.setAt(j, tx * tx + ty * ty);

      A2.setAt(j, 0, tx);
      A2.setAt(j, 1, ty);
      A2.setAt(j, 2, 1.0);
    }

    // solve equation system to get circle parameters
    try {
      if (!OiMat::solve(s, A2.t() * A2, -1.0 * A2.t() * A1)) {
        function->sendMessage("can not solve cylinder parameters",
                              MessageTypes::eErrorMessage);
        return false;
      }
    } catch (const exception& e) {
      function->sendMessage(
          std::format("inv error cylinder minimum solution: {}", e.what()),
          MessageTypes::eErrorMessage);
      return false;
    }

    // calculate midpoint + radius
    x_m = (-1.0 * s.getAt(0) / 2) + centroid2D.getAt(0);
    y_m = (-1.0 * s.getAt(1) / 2) + centroid2D.getAt(1);
    radius = std::sqrt((s.getAt(0) * s.getAt(0) + s.getAt(1) * s.getAt(1)) / 4 -
                       s.getAt(2));

    // calculate statistic
    v = -1.0 * A1 - A2 * s;
    OiVec::dot(sum_vv, v, v);
    sum_vv = std::sqrt(sum_vv / (numPoints - fit::kCircleUnknowns));

    // add approximation
    CylinderApproximation approximation;
    approximation.approxAlpha = alpha;
    approximation.approxBeta = beta;
    approximation.approxXm = -1.0 * x_m;
    approximation.approxYm = -1.0 * y_m;
    approximation.approxRadius = radius;
    approximation.stdev = sum_vv;
    approximation.comment = label;
    this->approximations.push_back(approximation);

    return true;
  }

  /*!
   * \brief fitCylinder
   * \param cylinder
   * \param inputObservations
   * \param approximation
   * \return
   */
  bool fitCylinder(FitFunction* function, Cylinder& cylinder,
                   const std::vector<IdPoint>& inputObservations,
                   const std::vector<IdPoint>& allUsableObservations,
                   const CylinderApproximation& approximation) {
    // get the number of observations
    const int numPoints = inputObservations.size();

    // centroids of all points
    OiVec centroid(4);

    // initialize variables
    OiVec v(numPoints * 3);   // approximation of corrections
    OiVec L0(numPoints * 3);  // L + v
    OiVec c(numPoints + fit::kCylinderUnknowns);  // right side
    OiMat N(numPoints + fit::kCylinderUnknowns,
            numPoints + fit::kCylinderUnknowns);    // normal equation matrix
    OiVec res(numPoints + fit::kCylinderUnknowns);  // adjustment result
    double _r = 0.0, _X0 = 0.0, _Y0 = 0.0, _alpha = 0.0, _beta = 0.0;
    double _r_armijo = 0.0, _X0_armijo = 0.0, _Y0_armijo = 0.0,
           _alpha_armijo = 0.0, _beta_armijo = 0.0;
    // OiVec a(5);
    // OiVec b(5);
    OiVec a(numPoints);
    OiVec b(numPoints);
    double _x = 0.0, _y = 0.0, _z = 0.0;
    double _x_armijo = 0.0, _y_armijo = 0.0, _z_armijo = 0.0;
    double a1 = 0.0, a2 = 0.0;
    double diff = 0.0, _xr = 0.0, _yr = 0.0;
    double sigma = fit::kArmijoInitialStep;
    OiMat Ralpha(3, 3);
    OiMat Rbeta(3, 3);
    OiMat Rall(3, 3);

    // set approximations of unknowns
    _r = approximation.approxRadius;
    _X0 = approximation.approxXm;
    _Y0 = approximation.approxYm;
    _alpha = approximation.approxAlpha;
    _beta = approximation.approxBeta;
    OiVec x(fit::kCylinderUnknowns);  // vector of unknown corrections

    // fill L vector
    for (int i = 0; i < numPoints; i++) {
      IdPoint obs = inputObservations.at(i);

      L0.setAt(i * 3, obs.xyz.getAt(0));
      L0.setAt(i * 3 + 1, obs.xyz.getAt(1));
      L0.setAt(i * 3 + 2, obs.xyz.getAt(2));

      centroid = centroid + obs.xyz;
    }
    centroid.removeLast();
    centroid = centroid / (double)numPoints;

    int numIterations = 0;

    double stopAA = 0.0, stopBB = 0.0, stopXX = 0.0, stopVV = 0.0;
    while (true) {
      // improve unknowns
      _r += x.getAt(0);
      _X0 += x.getAt(1);
      _Y0 += x.getAt(2);
      _alpha += x.getAt(3);
      _beta += x.getAt(4);

      // improve observations
      // L0 = L0 + v;

      // build rotation matrices
      Ralpha.setAt(0, 0, 1.0);
      Ralpha.setAt(1, 1, std::cos(_alpha));
      Ralpha.setAt(1, 2, -std::sin(_alpha));
      Ralpha.setAt(2, 1, std::sin(_alpha));
      Ralpha.setAt(2, 2, std::cos(_alpha));
      Rbeta.setAt(0, 0, std::cos(_beta));
      Rbeta.setAt(0, 2, std::sin(_beta));
      Rbeta.setAt(1, 1, 1.0);
      Rbeta.setAt(2, 0, -std::sin(_beta));
      Rbeta.setAt(2, 2, std::cos(_beta));

      Rall = Rbeta * Ralpha;

      // fill A and B matrix + w vector + right side
      for (int i = 0; i < numPoints; i++) {
        _x = L0.getAt(i * 3);
        _y = L0.getAt(i * 3 + 1);
        _z = L0.getAt(i * 3 + 2);

        a1 = _X0 + _x * std::cos(_beta) +
             _y * std::sin(_alpha) * std::sin(_beta) +
             _z * std::cos(_alpha) * std::sin(_beta);
        a2 = _Y0 + _y * std::cos(_alpha) - _z * std::sin(_alpha);

        // A
        N.setAt(i, numPoints, 1.0);
        N.setAt(i, numPoints + 1, -1.0 * a1 / std::sqrt(a1 * a1 + a2 * a2));
        N.setAt(i, numPoints + 2, -1.0 * a2 / std::sqrt(a1 * a1 + a2 * a2));
        N.setAt(i, numPoints + 3,
                -1.0 *
                    ((_y * std::sin(_beta) * std::cos(_alpha) -
                      _z * std::sin(_beta) * std::sin(_alpha)) *
                         a1 -
                     (_y * std::sin(_alpha) + _z * std::cos(_alpha)) * a2) /
                    std::sqrt(a1 * a1 + a2 * a2));
        N.setAt(i, numPoints + 4,
                -1.0 *
                    (_y * std::sin(_alpha) * std::cos(_beta) -
                     _x * std::sin(_beta) +
                     _z * std::cos(_alpha) * std::cos(_beta)) *
                    a1 / std::sqrt(a1 * a1 + a2 * a2));

        // AT
        N.setAt(numPoints, i, 1.0);
        N.setAt(numPoints + 1, i, -1.0 * a1 / std::sqrt(a1 * a1 + a2 * a2));
        N.setAt(numPoints + 2, i, -1.0 * a2 / std::sqrt(a1 * a1 + a2 * a2));
        N.setAt(numPoints + 3, i,
                -1.0 *
                    ((_y * std::sin(_beta) * std::cos(_alpha) -
                      _z * std::sin(_beta) * std::sin(_alpha)) *
                         a1 -
                     (_y * std::sin(_alpha) + _z * std::cos(_alpha)) * a2) /
                    std::sqrt(a1 * a1 + a2 * a2));
        N.setAt(numPoints + 4, i,
                -1.0 *
                    (_y * std::sin(_alpha) * std::cos(_beta) -
                     _x * std::sin(_beta) +
                     _z * std::cos(_alpha) * std::cos(_beta)) *
                    a1 / std::sqrt(a1 * a1 + a2 * a2));

        // BBT
        N.setAt(i, i,
                (-1.0 * std::cos(_beta) * a1 / std::sqrt(a1 * a1 + a2 * a2)) *
                        (-1.0 * std::cos(_beta) * a1 /
                         std::sqrt(a1 * a1 + a2 * a2)) +
                    (-1.0 *
                     (std::sin(_alpha) * std::sin(_beta) * a1 +
                      std::cos(_alpha) * a2) /
                     std::sqrt(a1 * a1 + a2 * a2)) *
                        (-1.0 *
                         (std::sin(_alpha) * std::sin(_beta) * a1 +
                          std::cos(_alpha) * a2) /
                         std::sqrt(a1 * a1 + a2 * a2)) +
                    (-1.0 *
                     (std::cos(_alpha) * std::sin(_beta) * a1 -
                      std::sin(_alpha) * a2) /
                     std::sqrt(a1 * a1 + a2 * a2)) *
                        (-1.0 *
                         (std::cos(_alpha) * std::sin(_beta) * a1 -
                          std::sin(_alpha) * a2) /
                         std::sqrt(a1 * a1 + a2 * a2)));

        _xr = Rall.getAt(0, 0) * _x + Rall.getAt(0, 1) * _y +
              Rall.getAt(0, 2) * _z + _X0;
        _yr = Rall.getAt(1, 0) * _x + Rall.getAt(1, 1) * _y +
              Rall.getAt(1, 2) * _z + _Y0;

        // approximate radius of the cylinder minus distance of point i to the
        // z-axis is the contradiction
        diff = _r - std::sqrt(_xr * _xr + _yr * _yr);

        c.setAt(i, diff);
      }

      // solve the normal equation system
      try {
        if (!OiMat::solve(res, N, -1.0 * c)) {
          function->sendMessage(std::string("solve error cylinder fit"),
                                MessageTypes::eErrorMessage);
          return false;
        }
      } catch (const exception& e) {
        function->sendMessage(
            std::format("solve error cylinder fit: {}", e.what()),
            MessageTypes::eErrorMessage);
        return false;
      }

      // get new corrections for unknowns
      for (int i = numPoints; i < numPoints + fit::kCylinderUnknowns; i++) {
        x.setAt(i - numPoints, res.getAt(i));
      }

      // calculate improvements
      for (int i = 0; i < numPoints; ++i) {
        IdPoint obs = inputObservations.at(i);

        _x = obs.xyz.getAt(0);
        _y = obs.xyz.getAt(1);
        _z = obs.xyz.getAt(2);

        a1 = _X0 + _x * std::cos(_beta) +
             _y * std::sin(_alpha) * std::sin(_beta) +
             _z * std::cos(_alpha) * std::sin(_beta);
        a2 = _Y0 + _y * std::cos(_alpha) - _z * std::sin(_alpha);

        v.setAt(i * 3,
                (-1.0 * std::cos(_beta) * a1 / std::sqrt(a1 * a1 + a2 * a2)) *
                    res.getAt(i));
        v.setAt(i * 3 + 1, (-1.0 *
                            (std::sin(_alpha) * std::sin(_beta) * a1 +
                             std::cos(_alpha) * a2) /
                            std::sqrt(a1 * a1 + a2 * a2)) *
                               res.getAt(i));
        v.setAt(i * 3 + 2, (-1.0 *
                            (std::cos(_alpha) * std::sin(_beta) * a1 -
                             std::sin(_alpha) * a2) /
                            std::sqrt(a1 * a1 + a2 * a2)) *
                               res.getAt(i));
      }

      while (true) {
        sigma = sigma / 2;

        _r_armijo = _r + sigma * x.getAt(0);
        _X0_armijo = _X0 + sigma * x.getAt(1);
        _Y0_armijo = _Y0 + sigma * x.getAt(2);
        _alpha_armijo = _alpha + sigma * x.getAt(3);
        _beta_armijo = _beta + sigma * x.getAt(4);

        for (int i = 0; i < numPoints; i++) {
          _x = L0.getAt(i * 3);
          _y = L0.getAt(i * 3 + 1);
          _z = L0.getAt(i * 3 + 2);

          _x_armijo = L0.getAt(i * 3) + sigma * v.getAt(i * 3);
          _y_armijo = L0.getAt(i * 3 + 1) + sigma * v.getAt(i * 3 + 1);
          _z_armijo = L0.getAt(i * 3 + 2) + sigma * v.getAt(i * 3 + 2);

          a.setAt(
              i, _r_armijo -
                     std::sqrt(
                         (_X0_armijo + _x_armijo * std::cos(_beta_armijo) +
                          _y_armijo * std::sin(_alpha_armijo) *
                              std::sin(_beta_armijo) +
                          _z_armijo * std::cos(_alpha_armijo) *
                              std::sin(_beta_armijo)) *
                             (_X0_armijo + _x_armijo * std::cos(_beta_armijo) +
                              _y_armijo * std::sin(_alpha_armijo) *
                                  std::sin(_beta_armijo) +
                              _z_armijo * std::cos(_alpha_armijo) *
                                  std::sin(_beta_armijo)) +
                         (_Y0_armijo + _y_armijo * std::cos(_alpha_armijo) -
                          _z_armijo * std::sin(_alpha_armijo)) *
                             (_Y0_armijo + _y_armijo * std::cos(_alpha_armijo) -
                              _z_armijo * std::sin(_alpha_armijo))));

          b.setAt(i,
                  _r - std::sqrt((_X0 + _x * std::cos(_beta) +
                                  _y * std::sin(_alpha) * std::sin(_beta) +
                                  _z * std::cos(_alpha) * std::sin(_beta)) *
                                     (_X0 + _x * std::cos(_beta) +
                                      _y * std::sin(_alpha) * std::sin(_beta) +
                                      _z * std::cos(_alpha) * std::sin(_beta)) +
                                 (_Y0 + _y * std::cos(_alpha) -
                                  _z * std::sin(_alpha)) *
                                     (_Y0 + _y * std::cos(_alpha) -
                                      _z * std::sin(_alpha))));
        }

        OiVec::dot(stopAA, a, a);
        OiVec::dot(stopBB, b, b);

        if (!(stopAA > stopBB)) {
          break;
        }
      }

      OiVec::dot(stopXX, x, x);
      OiVec::dot(stopVV, v, v);

      x = sigma * x;
      v = sigma * v;

      sigma = fit::kArmijoInitialStep;

      numIterations++;

      if (!(stopXX > fit::kMinSquaredCorrection &&
            numIterations < fit::kMaxIterations)) {
        break;
      }
    }

    if (numIterations >= fit::kMaxIterations) {
      function->sendMessage(
          std::format("to many iterations: {}", numIterations),
          MessageTypes::eWarningMessage);
      return false;
    }

    // apply corrections to unknowns
    _r += x.getAt(0);
    _X0 += x.getAt(1);
    _Y0 += x.getAt(2);
    _alpha += x.getAt(3);
    _beta += x.getAt(4);

    // calculate rotation matrices
    Ralpha.setAt(0, 0, 1.0);
    Ralpha.setAt(1, 1, std::cos(_alpha));
    Ralpha.setAt(1, 2, -std::sin(_alpha));
    Ralpha.setAt(2, 1, std::sin(_alpha));
    Ralpha.setAt(2, 2, std::cos(_alpha));
    Rbeta.setAt(0, 0, std::cos(_beta));
    Rbeta.setAt(0, 2, std::sin(_beta));
    Rbeta.setAt(1, 1, 1.0);
    Rbeta.setAt(2, 0, -std::sin(_beta));
    Rbeta.setAt(2, 2, std::cos(_beta));

    Rall = Rbeta * Ralpha;

    // calculate point on cylinder axis
    OiVec xyz(3);
    xyz.setAt(0, _X0);
    xyz.setAt(1, _Y0);
    xyz.setAt(2, 0.0);
    OiMat::solve(xyz, Rall, -1.0 * xyz);

    // calculate cylinder axis
    OiVec axis(3);
    axis.setAt(2, 1.0);
    OiMat::solve(axis, Rall, axis);

    // reset / clear statistic
    function->statistic.reset();

    // calculate sum vv
    double sumVV = 0.0;
    double vrMin = numeric_limits<double>::max();
    double vrMax = numeric_limits<double>::lowest();
    for (const IdPoint& observation : oi::snapshot(allUsableObservations)) {
      OiVec v_obs(3);
      _x = observation.xyz.getAt(0);
      _y = observation.xyz.getAt(1);
      _z = observation.xyz.getAt(2);

      std::array<double, 3> b;  // vector between point on cylinder axis and
                                // point p which is probably on cylinder
      b[0] = static_cast<double>(_x - xyz.getAt(0));
      b[1] = static_cast<double>(_y - xyz.getAt(1));
      b[2] = static_cast<double>(_z - xyz.getAt(2));

      std::array<double, 3>
          n0CrossB;  // cross product of cylinder axis (length 1) and b
      n0CrossB[0] =
          static_cast<double>(axis.getAt(1) * b[2] - axis.getAt(2) * b[1]);
      n0CrossB[1] =
          static_cast<double>(axis.getAt(2) * b[0] - axis.getAt(0) * b[2]);
      n0CrossB[2] =
          static_cast<double>(axis.getAt(0) * b[1] - axis.getAt(1) * b[0]);

      double radiusActual =
          0.0;  // smallest distance of point p to cylinder axis

      radiusActual =
          std::sqrt(n0CrossB[0] * n0CrossB[0] + n0CrossB[1] * n0CrossB[1] +
                    n0CrossB[2] * n0CrossB[2]);

      double distance = 0.0;

      distance = static_cast<double>(radiusActual - _r);  // distance error
      if (oi::contains(
              inputObservations,
              observation)) {  // calculate form error from "used" observations
        vrMin = min(vrMin, distance);
        vrMax = max(vrMax, distance);
      }

      // calculate residual vector
      v_obs.setAt(0, b[0]);
      v_obs.setAt(1, b[1]);
      v_obs.setAt(2, b[2]);
      v_obs.normalize();
      v_obs = distance * v_obs;

      // set up display residuals
      if (false) {
        function->addDisplayResidual(
            observation.id, v_obs.getAt(0), v_obs.getAt(1), v_obs.getAt(2),
            std::sqrt(v_obs.getAt(0) * v_obs.getAt(0) +
                      v_obs.getAt(2) * v_obs.getAt(2)));
      } else {
        function->addDisplayResidual(observation.id, distance);
      }

      if (oi::contains(inputObservations, observation)) {
        sumVV += distance * distance;
      }
    }

    function->statistic.setFormError(vrMax - vrMin);  // roundness

    // set up result
    Radius cylinderRadius(_r);
    Direction cylinderDirection(axis);
    Position cylinderPosition(xyz);
    cylinder.setCylinder(cylinderPosition, cylinderDirection, cylinderRadius);
    function->statistic.setIsValid(true);
    function->statistic.setStdev(std::sqrt(
        sumVV / (static_cast<double>(numPoints) - fit::kCylinderUnknowns)));
    cylinder.setStatistic(function->statistic);

    return true;
  }

  /*!
   * \brief getCorrespondingCos
   * \param a
   * \return
   */
  double getCorrespondingCos(double a) {
    while (a < 0.0) {
      a += 2 * kPi;
    }
    while (a > 2 * kPi) {
      a -= 2 * kPi;
    }
    return 2 * kPi - a;
  }

  /*!
   * \brief getCorrespondingSin
   * \param a
   * \return
   */
  double getCorrespondingSin(double a) {
    while (a < 0.0) {
      a += 2 * kPi;
    }
    while (a > 2 * kPi) {
      a -= 2 * kPi;
    }
    if (a <= kPi) {
      return kPi - a;
    } else {
      return 3 * kPi - a;
    }
  }

  /*!
   * \brief compareAngles
   * \param a
   * \param b
   * \return
   */
  bool compareAngles(double a, double b) {
    double t = 2 * kPi;

    // a und b between 0 und 2*kPi
    while (a < 0.0) {
      a += t;
    }
    while (a > t) {
      a -= t;
    }
    while (b < 0.0) {
      b += t;
    }
    while (b > t) {
      b -= t;
    }

    double diff = a - b;
    if (diff < fit::kAngleEqualityTolerance &&
        diff > -fit::kAngleEqualityTolerance) {
      return true;
    }
    return false;
  }
};

};  // namespace oi

#include <format>

#include "plugin_abi.h"

inline const std::string FitFunction_iidd =
    oi::pluginIid("de.openIndy.plugin.function.fitFunction.v");

#endif  // OI_CORE_PLUGIN_FUNCTION_FIT_FUNCTION_H_
