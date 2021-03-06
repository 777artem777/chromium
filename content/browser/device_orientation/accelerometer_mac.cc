// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/device_orientation/accelerometer_mac.h"

#include <math.h>

#include "base/logging.h"
#include "content/browser/device_orientation/orientation.h"
#include "third_party/sudden_motion_sensor/sudden_motion_sensor_mac.h"

namespace device_orientation {

// Create a AccelerometerMac object and return NULL if no valid sensor found.
DataFetcher* AccelerometerMac::Create() {
  scoped_ptr<AccelerometerMac> accelerometer(new AccelerometerMac);
  return accelerometer->Init() ? accelerometer.release() : NULL;
}

AccelerometerMac::~AccelerometerMac() {
}

AccelerometerMac::AccelerometerMac() {
}

//  Retrieve per-axis orientation values.
//
//  Axes and angles are defined according to the W3C DeviceOrientation Draft.
//  See here: http://dev.w3.org/geo/api/spec-source-orientation.html
//
//  Note: only beta and gamma angles are provided. Alpha is set to zero.
//
//  Returns false in case of error.
//
bool AccelerometerMac::GetOrientation(Orientation* orientation) {
  DCHECK(sudden_motion_sensor_.get());

  // Retrieve per-axis calibrated values.
  float axis_value[3];
  if (!sudden_motion_sensor_->ReadSensorValues(axis_value))
    return false;

  // Transform the accelerometer values to W3C draft angles.
  //
  // Accelerometer values are just dot products of the sensor axes
  // by the gravity vector 'g' with the result for the z axis inverted.
  //
  // To understand this transformation calculate the 3rd row of the z-x-y
  // Euler angles rotation matrix (because of the 'g' vector, only 3rd row
  // affects to the result). Note that z-x-y matrix means R = Ry * Rx * Rz.
  // Then, assume alpha = 0 and you get this:
  //
  // x_acc = sin(gamma)
  // y_acc = - cos(gamma) * sin(beta)
  // z_acc = cos(beta) * cos(gamma)
  //
  // After that the rest is just a bit of trigonometry.
  //
  // Also note that alpha can't be provided but it's assumed to be always zero.
  // This is necessary in order to provide enough information to solve
  // the equations.
  //
  const double kRad2deg = 180.0 / M_PI;

  orientation->alpha_ = 0.0;
  orientation->beta_  = kRad2deg * atan2(-axis_value[1], axis_value[2]);
  orientation->gamma_ = kRad2deg * asin(axis_value[0]);
  orientation->absolute_ = false;

  // Make sure that the interval boundaries comply with the specification. At
  // this point, beta is [-180, 180] and gamma is [-90, 90], but the spec has
  // the upper bound open on both.
  if (orientation->beta_ == 180.0) {
    orientation->beta_ = -180.0;  // -180 == 180 (upside-down)
  }
  if (orientation->gamma_ == 90.0) {
    static double just_less_than_90 = nextafter(90, 0);
    orientation->gamma_ = just_less_than_90;
  }

  // At this point, DCHECKing is paranoia. Never hurts.
  DCHECK_GE(orientation->beta_, -180.0);
  DCHECK_LT(orientation->beta_,  180.0);
  DCHECK_GE(orientation->gamma_, -90.0);
  DCHECK_LT(orientation->gamma_,  90.0);

  orientation->can_provide_alpha_ = false;
  orientation->can_provide_beta_  = true;
  orientation->can_provide_gamma_ = true;
  orientation->can_provide_absolute_ = false;

  return true;
}

bool AccelerometerMac::Init() {
  sudden_motion_sensor_.reset(SuddenMotionSensor::Create());
  return sudden_motion_sensor_.get() != NULL;
}

}  // namespace device_orientation
