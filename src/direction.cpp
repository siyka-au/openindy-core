#include "direction.h"

using namespace oi;
using namespace oi::math;

Direction::Direction() : ijk(3), ijkH(3) { this->ijkH.add(1.0); }

Direction::Direction(const OiVec& v) {
  if (v.getSize() == 3) {
    this->ijk = v;
    this->ijkH = v;
    this->ijkH.add(1.0);
  } else if (v.getSize() == 4) {
    this->ijk.add(v.getAt(0) / v.getAt(3));
    this->ijk.add(v.getAt(1) / v.getAt(3));
    this->ijk.add(v.getAt(2) / v.getAt(3));
    this->ijkH = v;
  } else {
    this->ijk = OiVec(3);
    this->ijkH = OiVec(3);
    this->ijkH.add(1.0);
  }
}

Direction::Direction(double x, double y, double z, double h) {
  this->ijk.add(x / h);
  this->ijk.add(y / h);
  this->ijk.add(z / h);
  this->ijkH.add(x);
  this->ijkH.add(y);
  this->ijkH.add(z);
  this->ijkH.add(h);
}

void Direction::setVector(double x, double y, double z, double h) {
  this->ijk.setAt(0, x / h);
  this->ijk.setAt(1, y / h);
  this->ijk.setAt(2, z / h);
  this->ijkH.setAt(0, x);
  this->ijkH.setAt(1, y);
  this->ijkH.setAt(2, z);
  this->ijkH.setAt(3, h);
}

void Direction::setVector(const OiVec& v) {
  if (v.getSize() == 3) {
    this->ijk = v;
    this->ijkH.setAt(0, v.getAt(0));
    this->ijkH.setAt(1, v.getAt(1));
    this->ijkH.setAt(2, v.getAt(2));
    this->ijkH.setAt(3, 1.0);
  } else if (v.getSize() == 4) {
    this->ijk.setAt(0, v.getAt(0) / v.getAt(3));
    this->ijk.setAt(1, v.getAt(1) / v.getAt(3));
    this->ijk.setAt(2, v.getAt(2) / v.getAt(3));
    this->ijkH = v;
  }
}

const OiVec& Direction::getVector() const { return ijk; }

const OiVec& Direction::getVectorH() const { return ijkH; }
