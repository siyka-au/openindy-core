#ifndef STATISTIC_H
#define STATISTIC_H
#include <map>
#include <vector>

#include "core_export.h"
#include "oi_mat.h"
#include "oi_vec.h"
#include "types.h"

namespace oi {

using namespace math;

/*!
 * \brief The Residual class
 */
class OI_CORE_EXPORT Residual {
 public:
  Residual() {}

  int elementId{-1};  // id of the element this residual belongs to
  std::map<std::string, double>
      corrections;          // the correction values (key: name)
  DimensionType dimension;  // dimension of the correction values
};

/*!
 * \brief The Statistic class
 */
class OI_CORE_EXPORT Statistic {
 public:
  Statistic();
  Statistic(const Statistic& copy);

  Statistic& operator=(const Statistic& other);

  // ###################################
  // get valid state and reset statistic
  // ###################################

  [[nodiscard]] const bool& getIsValid() const;
  void setIsValid(const bool& isValid);

  void reset();

  // #################################
  // get or set statistical parameters
  // #################################

  [[nodiscard]] const double& getS0APriori() const;
  void setS0APriori(const double& s0);

  [[nodiscard]] const double& getS0APosteriori() const;
  void setS0APosteriori(const double& s0);

  [[nodiscard]] const double& getStdev() const;
  void setStdev(const double& stdev);

  [[nodiscard]] const OiMat& getP() const;
  void setP(const OiMat& p);

  [[nodiscard]] const OiMat& getQxx() const;
  void setQxx(const OiMat& qxx);

  [[nodiscard]] const OiVec& getV() const;
  void setV(const OiVec& v);

  [[nodiscard]] const std::vector<Residual> getDisplayResiduals() const;
  [[nodiscard]] Residual getDisplayResidual(const int& elementId) const;
  void addDisplayResidual(const Residual& residual);

  void setFormError(const double& formError);
  [[nodiscard]] const double& getFormError() const;

 private:
  bool isValid;

  // ######################
  // statistical parameters
  // ######################

  double s0_apriori;
  double s0_aposteriori;

  double
      stdev;  // standard deviation of vertical distances from geometry surface

  OiMat p;
  OiMat qxx;

  OiVec v;  // vertical distances from geometry surface
  std::map<int, Residual>
      displayResidualsMap;  // map of display residuals (key: element id that
                            // the residual belongs to)

  double formError;
};

}  // namespace oi

#endif  // STATISTIC_H
