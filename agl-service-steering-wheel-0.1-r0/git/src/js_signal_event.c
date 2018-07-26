/*
 * Copyright (c) 2017 TOYOTA MOTOR CORPORATION
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <alloca.h>

#include "af-steering-wheel-binding.h"
#include "js_signal_event.h"

// Axis
#define JS_STEERING 			0
#define JS_THROTTLE 			2
#define JS_BRAKE				3
// Button
#define JS_LEFT_PADDLE_SHIFTER	4
#define JS_RIGHT_PADDLE_SHIFTER	5
#define JS_TURN_SIGNAL_RIGHT	6
#define JS_TURN_SIGNAL_LEFT		7

// Property string
#define VEHICLE_SPEED				"VehicleSpeed"
#define ENGINE_SPEED				"EngineSpeed"
#define ACCELERATOR_PEDAL_POSITION	"AcceleratorPedalPosition"
#define TRANSMISSION_GEAR_INFO		"TransmissionGearInfo"
#define TRANSMISSION_MODE			"TransmissionMode"
#define STEERING_WHEEL_ANGLE		"SteeringWheelAngle"
#define TURN_SIGNAL_STATUS			"TurnSignalStatus"
#define LIGHT_STATUS_BRAKE			"LightStatusBrake"

#define MAX_TRANSMISSION_GEAR_INFO 6
#define MIN_TRANSMISSION_GEAR_INFO 0

enum eTransmissionGearInfo
{
	eTransmissionGearInfoLeft = 0,
	eTransmissionGearInfoRight = 1,
};

enum eTurnSignalStatus
{
	eTurnSignalStatusOff = 0,
	eTurnSignalStatusRight = 1,
	eTurnSignalStatusLeft = 2,
	eTurnSignalStatusHazard = 3
};

double gearRatio[8] =
{
	0.0,	//Neutral
	1.0/4.12,	//First
	1.0/2.84,	//Second
	1.0/2.28,	//Third
	1.0/1.45,	//Fourth
	1.0/1.0,	//Fifth
	1.0/0.69,	//Sixth
	1.0/3.21	//Reverse
};

static int gAcceleratorPedalPosition = 0;
static int gLightStatusBrake = 0;
static int gTransmissionGearInfo = 1;
static int gSteeringWheelAngle = 0;
static enum eTurnSignalStatus gTurnSignalStatus = eTurnSignalStatusOff;

// Method to set value
static void setAcceleratorPedalPosition(int val)
{
	if(gAcceleratorPedalPosition != val)
	{
		gAcceleratorPedalPosition = val;
	}
}

static void setLightStatusBrake(int val)
{
	if(gLightStatusBrake != val)
	{
		gLightStatusBrake = val;
	}
}

static void setTransmissionGearInfo(int val, enum eTransmissionGearInfo eGear)
{
	if(eGear == eTransmissionGearInfoLeft)
	{
		if(val && gTransmissionGearInfo < MAX_TRANSMISSION_GEAR_INFO)
		{
			gTransmissionGearInfo = gTransmissionGearInfo + 1;
		}
	}
	else if(eGear == eTransmissionGearInfoRight)
	{
		if(val && gTransmissionGearInfo > MIN_TRANSMISSION_GEAR_INFO)
		{
			gTransmissionGearInfo = gTransmissionGearInfo - 1;
		}
	}
}

static void setSteeringWheelAngle(int val)
{
	if(gSteeringWheelAngle != val)
	{
		gSteeringWheelAngle = val;
	}
}

static void setTurnSignalStatus(int val, enum eTurnSignalStatus turn)
{
	if(val)
	{
		if(turn == eTurnSignalStatusRight)
		{
			gTurnSignalStatus = eTurnSignalStatusRight;
		}
		else if(turn == eTurnSignalStatusLeft)
		{
			gTurnSignalStatus = eTurnSignalStatusLeft;
		}
	}
	else
	{
		gTurnSignalStatus = eTurnSignalStatusOff;
	}
}

// Method to calculate property
static int calcAcceleratorPedalPosition()
{
	return  (int)(((double)(gAcceleratorPedalPosition - 32767)/(double)-65534.0)*(double)100.0);
}

static int calcEngineSpeed()
{
	int acceleratorPedalPosition = calcAcceleratorPedalPosition();
	int engineSpeed = (int)acceleratorPedalPosition * 100;

	return engineSpeed;
}

static int calcVehicleSpeed()
{
	int engineSpeed = calcEngineSpeed();
	double transmissionGearInfoRatio = gearRatio[(gTransmissionGearInfo == 128 ? 7 : gTransmissionGearInfo)];
	int vehicleSpeed = (int)(engineSpeed * transmissionGearInfoRatio / 100);

	return vehicleSpeed;
}

static int calcSteeringWheelAngle()
{
	double steeringWheelAngle  = (((double)gSteeringWheelAngle / (double)32767.0) + (double)1.0) * (double)180.0;

	return (int)steeringWheelAngle;
}

// Method to update property
static void updateValue(char *prop, int val)
{
	unsigned int nProp = wheel_info->nData;
	for(unsigned int i = 0; i < nProp; i++)
	{
		if(strcmp(prop, wheel_info->property[i].name) == 0)
		{
			if(wheel_info->property[i].curValue.int16_val != val)
			{
				wheel_info->property[i].curValue.int16_val = (int16_t)val;
				//NOTICEMSG("Update property %s", wheel_info->property[i].name);
				notify_property_changed(&wheel_info->property[i]);
			}
		}
	}
}

void updateTransmissionGearInfo()
{
	updateValue(TRANSMISSION_GEAR_INFO, gTransmissionGearInfo);
}

void updateTransmissionMode()
{
	updateValue(TRANSMISSION_MODE, gTransmissionGearInfo);
}

static void updateAcceleratorPedalPosition()
{
	int acceleratorPedalPosition;

	acceleratorPedalPosition = calcAcceleratorPedalPosition();
	updateValue(ACCELERATOR_PEDAL_POSITION, acceleratorPedalPosition);
}

static void updateEngineSpeed()
{
	// Update EngineSpeed
	int engineSpeed;

	engineSpeed = calcEngineSpeed();
	updateValue(ENGINE_SPEED, engineSpeed);
}

static void updateVehicleSpeed()
{
	// Update VehicleSpeed
	int vehicleSpeed;

	vehicleSpeed = calcVehicleSpeed();
	updateValue(VEHICLE_SPEED, vehicleSpeed);
}

static void updateLightStatusBrake()
{
	int lightStatusBrake;

	lightStatusBrake = (gLightStatusBrake < 20000);
	updateValue(LIGHT_STATUS_BRAKE, lightStatusBrake);
}

static void updateSteeringWheelAngle()
{
	int steering = calcSteeringWheelAngle();
	updateValue(STEERING_WHEEL_ANGLE, steering);
}

static void updateTurnSignalStatus()
{
	updateValue(TURN_SIGNAL_STATUS, gTurnSignalStatus);
}

// Method to handle joy stick event
void newButtonValue(char number, int val)
{
	switch (number)
	{
		case JS_LEFT_PADDLE_SHIFTER:	//Left paddle shifter
			// Set gear position
			setTransmissionGearInfo(val, eTransmissionGearInfoLeft);

			// Update property
			updateTransmissionGearInfo();
			updateTransmissionMode();
			updateVehicleSpeed();
			break;

		case JS_RIGHT_PADDLE_SHIFTER:	//Right paddle shifter
			// Set gear position
			setTransmissionGearInfo(val, eTransmissionGearInfoRight);

			// Update property
			updateTransmissionGearInfo();
			updateTransmissionMode();
			updateVehicleSpeed();
			break;

		case JS_TURN_SIGNAL_RIGHT:	//Right upper wheel button
			// Set value
			setTurnSignalStatus(val, eTurnSignalStatusRight);

			// Update property
			updateTurnSignalStatus();
			break;

		case JS_TURN_SIGNAL_LEFT:	//Left upper wheel button
			// Set value
			setTurnSignalStatus(val, eTurnSignalStatusLeft);

			// Update property
			updateTurnSignalStatus();
			break;

		default:
			break;
	}
}

void newAxisValue(char number, int val)
{
	switch (number)
	{
		case JS_STEERING:	//Wheel angle, -32767 - 32767
			// Set value
			setSteeringWheelAngle(val);

			// Update property
			updateSteeringWheelAngle();
			break;

		case JS_THROTTLE:	//Throttle, -32767 (depressed) - 32767 (undepressed)
			// Set origin value
			setAcceleratorPedalPosition(val);

			// Update property
			updateAcceleratorPedalPosition();
			updateEngineSpeed();
			updateVehicleSpeed();
			break;

		case JS_BRAKE:
			// Set value
			setLightStatusBrake(val);

			// Update property
			updateLightStatusBrake();
			break;	//Brake, -32767 (depressed) - 32767 (undepressed)

		default:
			break;
	}
}
