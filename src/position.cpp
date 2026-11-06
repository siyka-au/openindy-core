#include "position.h"

using namespace oi;
using namespace oi::math;

Position::Position() : xyz(3), xyzH(3) { this->xyzH.add(1.0); }

Position::Position(bool isNullObject) : xyz(3), xyzH(3) {
  this->xyzH.add(1.0);
  this->isNullObject = isNullObject;
}

Position::Position(const OiVec& v) {
  if (v.getSize() == 3) {
    this->xyz = v;
    this->xyzH = v;
    this->xyzH.add(1.0);
  } else if (v.getSize() == 4) {
    this->xyz.add(v.getAt(0) / v.getAt(3));
    this->xyz.add(v.getAt(1) / v.getAt(3));
    this->xyz.add(v.getAt(2) / v.getAt(3));
    this->xyzH = v;
  } else {
    this->xyz = OiVec(3);
    this->xyzH = OiVec(3);
    this->xyzH.add(1.0);
  }
}

Position::Position(double x, double y, double z, double h) {
  this->xyz.add(x / h);
  this->xyz.add(y / h);
  this->xyz.add(z / h);
  this->xyzH.add(x);
  this->xyzH.add(y);
  this->xyzH.add(z);
  this->xyzH.add(h);
}

void Position::setVector(const OiVec& v) {
  if (v.getSize() == 3) {
    this->xyz = v;
    this->xyzH.setAt(0, v.getAt(0));
    this->xyzH.setAt(1, v.getAt(1));
    this->xyzH.setAt(2, v.getAt(2));
    this->xyzH.setAt(3, 1.0);
  } else if (v.getSize() == 4) {
    this->xyz.setAt(0, v.getAt(0) / v.getAt(3));
    this->xyz.setAt(1, v.getAt(1) / v.getAt(3));
    this->xyz.setAt(2, v.getAt(2) / v.getAt(3));
    this->xyzH = v;
  }
}

void Position::setVector(double x, double y, double z, double h) {
  this->xyz.setAt(0, x / h);
  this->xyz.setAt(1, y / h);
  this->xyz.setAt(2, z / h);
  this->xyzH.setAt(0, x);
  this->xyzH.setAt(1, y);
  this->xyzH.setAt(2, z);
  this->xyzH.setAt(3, h);
}

const OiVec& Position::getVector() const { return this->xyz; }

const OiVec& Position::getVectorH() const { return this->xyzH; }

const Position Position::NullObject = Position(true);
