/*!
* \file constants.h
* \brief The constants.
* \author Romain Baud
* \version 0.1
* \date 2012.11.26
*/

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

/// Communication port. Chosen because it seems to be unassigned for now,
/// according to this list:
/// http://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xml
const int IN_PORT = 7444;

/// Maximum thrust command. It corresponds to the maximum of a 8 bits value,
/// because the microcontroller expects a 8 bit value for the motors powers.
const int MAX_THRUST = 255.0;

/// Size of the first part of the message, which is the size of the rest of the
/// message. This is a 32 bit value, so 4 bytes.
const int MESSAGE_SIZE_SIZE = 4;

/// Amount (per loop) of variation for the mean thrust, with the stick.
/// The Xbox gamepad sticks always return to the central position when released,
/// unlike a conventionnal remote control. Here, this is why the motors mean
/// speed is modified in a relative way, by the stick.
const double THRUST_VARSPEED = 3.0;

/// Amount (per loop) of variation for the yaw angle, with the stick.
/// On conventionnal helicopters, the yaw axis drives the yaw speed, unlike the
/// iFly which is driven by the yaw angle. This is why the yaw angle is
/// modified in a relative way, by the stick.
const double YAW_VARSPEED = 15.0;

/// Yaw amplitude when controlled by the stick or the slider, in degrees.
const double YAW_AMPLITUDE = 180.0; // [deg].

/// Pitch amplitude when controlled by the stick or the slider, in degrees.
const double PITCH_AMPLITUDE = 20.0; // [deg].

/// Roll amplitude when controlled by the stick or the slider, in degrees.
const double ROLL_AMPLITUDE = 20.0; // [deg].

/// Amplitude of the gamepad sticks axis.
const double GP_AXIS_AMPLITUDE = 1.0;

/// Time between each execution of the main loop, in milliseconds. So this will
/// also change the real variation speed of the thrust and yaw!
const int UPDATE_PERIOD_MS = 20;

/// Same as UPDATE_PERIOD_MS, but in seconds instead of milliseconds.
const double UPDATE_PERIOD_S = ((double)UPDATE_PERIOD_MS) / 1000.0;

/// Dead zone of the gamepad axis used as relative input (thrust and yaw).
/// This is useful, because when released, the input value is not exactly zero,
/// so without the dead zone this error would be integrated, and the command
/// would change unexpectedly. It is set to 5% of the full scale.
const double DEAD_ZONE_GAMEPAD = GP_AXIS_AMPLITUDE / 20.0;

/// Delay to compensate the network latency, for regulator state check.
/// The regulators state of the quadcopter is commanded by a checkbox, which
/// also has a state. In the case the quadcopter never receives the activation
/// message (so will not respond), the checkbox will untick itself, to let the
/// user checking it again.
const int REGU_ON_STATE_WAIT_TIME_MS = 4000;

/// Maximum voltage of the Li-Po battery, in volts. This is used to get a rough
/// estimation of the battery level.
const double MAX_BATTERY_VOLTAGE = 12.6;

/// Minimum safe voltage of the Li-Po battery, in volts. This is used to get a
/// rough estimation of the battery level.
const double MIN_BATTERY_VOLTAGE = 11.0;

/// Application name.
const QString APP_NAME = "AndroCopter remote";

/// Maximum pitch or roll angle (in degrees) that can be outputted by the PIDs.
const double MAX_PITCH_ROLL_TARGET_ANGLE = 20.0;

/// Filtering constant for the low-pass filter of the FPV rate.
/// Should be between 0.0 (no filtering) and 1.0 (strong filtering).
const double FPV_RATE_LPF = 0.8;

#endif // CONSTANTS_H
