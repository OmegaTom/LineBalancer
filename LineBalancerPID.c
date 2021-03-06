#pragma config(Sensor, S1,     ultrasonicSensor, sensorEV3_Ultrasonic)
#pragma config(Sensor, S2,     gyroSensor,     sensorEV3_Gyro, modeEV3Gyro_Rate)
#pragma config(Sensor, S3,     colorSensor,    sensorEV3_Color)
#pragma config(Sensor, S4,     irSensor,       sensorEV3_IRSensor, modeEV3IR_Remote)
#pragma config(Motor,  motorA,           ,             tmotorEV3_Large, openLoop, encoder)
#pragma config(Motor,  motorB,          rightMotor,    tmotorEV3_Large, openLoop, driveRight)
#pragma config(Motor,  motorC,          leftMotor,     tmotorEV3_Large, openLoop, driveLeft, encoder)
#pragma config(Motor,  motorD,          liftMotor,     tmotorEV3_Medium, PIDControl, encoder)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

// you need:
// - robotC ... you can download a trial version under 'http://www.robotc.net/' ... make sure to activate the 'natural language' option
// - a 'balancer robot' (e.g. balanc3r with the sensor on the left side!!!, or gyroboy or something similar...)
// with motors connected to B and C.
// the gyro (connected to S2) should be mounted on the left side (left hand of the robot) of the robot (the higher the better)
// if you want combine the balancing challenge with a line follower you need the light sensor somewhere
// near the floor in front of the wheels connected to S3.
// the ultrasonic (S1) sensor can be used to prevent collisions if the robot is moving (robot simply stops)
//
//
// what is new here?
//
// the 'original' lego balancing example is using a quite simple approach to
// solve the balancing problem. The lego LabView solution uses a simple
// linear combination of the gyro speed, gyro angle, robot speed and robot position
// to calculate the power that is needed to balance the robot
// (finding a working linear combination must have been a nightmare job).
// This solution is very sensitive and the room for optimizations is very small
//
// Laurens Valk has enhanced Legos simple approach by adding a single PID controller (great job). Still
// the same linear combination of the same parameters is used. Adding the PID controller goes hand in hand
// with improved stability and some more room for optimizations (but still quite limited)
// This solution goes hand in hand with the fact the the PID reference is always 0 even when we want
// a moving robot. It's fore sure a nice solution but not really a 'classic' PID control loop where
// we normally want to control a specific physical parameter like the robot speed directly.
//
//
// the following approach is using 4 parallel working PID controllers
//
// (1) 'gyro_speed': contolling the gyro speed
// (2) 'gyroAngle': controlling the gyro angle
// (3) 'robot_position': controlling the robot position
// (4) 'robot_speed': controlling the robot speed
//
// this approach gives a lot of room for parameter optimizations and is a very good example how a
// combination of several PID controllers can work. The basic idea is explained below in more detail.
//
//
// why do we need all these parameters to balance a robot?
//
// I am sure that there are a lot of robot fans out there that have tried to balance
// a robot just by using a simple control loop using the gyro.
// Imagine the robot is balanced and starts to fall forward. A simple PID conntrol loop approach
// could use just the gyro angle to calculate the motor power that is needed to reduce the angle
// (the error) again. If the parameters are chosen properly the robot will be in the balanced status
// again (anytime soon). The problem is that there is no need (from the physical point of view) to reduce
// (no 'force' that is reducing) the speed of the robot (the speed is the result of the acceleration phase
// that was necessary to get the robot back into the balanced status).
// As a consequence the robot is balanced for a while but the speed/position is not controlled
// and so you end up with a robot at full speed (after a quite short time) without the possibility to control the balancing anymore.
// So it is clear that we have to control the speed/position of the robot somehow.
// the gyro control loops in this solution are nothing special. The motor power is 'proportional' (depending on the PID parameters) to the gyro angle/speed
// (obviously the reference for these PID controllers is 0 here).
//
// for the speed/position controller loop we have to consider the following fact:
// imagine that the robot is a few mm behind the ideal reference position in a 'perfectly' balanced status. So we have to move forward to correct the position
// (to minimize the position error).
// to move forward it is not 'allowed' just to move forward because the robot would fall onto the backside.
// to move forward the robot must move backwards first and than accelerate forward (here the gyro control loop is doing the job)
// to balance again (this is the way a balancing robot can make a 'step' forward).
// In other words we have to set the motor power to negative values to be able to move forward afterwards to reach the reference position (that means that we
// are increasing the position PID error in order to get the possibility to minimize the same error afterwards. The bigger the distance to the reference point in our example
// (the bigger the error) the more we must accelerate/move into the opposite direction to make a bigger step forward afterwards (the acceleration forward is 'triggered' by
// the gyro control loops as soon the robot starts to fall forward)
// (--> so a 'classic' PID approach should work fine but the output of the position and speed PID must be weighted negative to the motor power)
// the same concept/background is valid for the robot speed ...
//
//
// for those who are interested in the mathematical background I can recommend
// https://en.wikipedia.org/wiki/PID_controller
// the wiki article is covering some complex mathematical theories and you might have leran a lot more to be able to undertand everything in detail.
// a very nice 'easy to understand' artircle about PID controllers you can find under
// http://www.inpharmix.com/jps/PID_Controller_For_Lego_Mindstorms_Robots.html



//***************************************************************************************************************************************************************
// and here is the code in RobotC with natural language option enabled ....


// Pragmas for showing different debug windows
//#pragma DebuggerWindows("Locals")
//#pragma DebuggerWindows("Globals")
//#pragma DebuggerWindows("DebugStream")
//#pragma DebuggerWindows("EV3LCDScreen")

#define pi ( 3.14159265 )					// needed to calculate the speed in mm/sec and the robot position in mm

// Power limit (+/- POWER_LIMIT) for motors
#define POWER_LIMIT ( 100 )
// Power limit (+/- POWER_LIMIT) for steering
#define STEERING_LIMIT ( 20 )


// 'Requested' sample time in milliseconds
#define sampleTime  ( 20.0 )					// 20ms seems to work fine without any timing overflows

// Wheel diameter in mm -- needed to compute robot position in mmm and robot speed in mmm/sec
// general: the bigger the wheels the better because max speed is higher (see pid_corr_dia)
short wheelDiameterIn_mm = 70.0;
// Convert wheel diameter to radius in mm
float wheelRadius = wheelDiameterIn_mm / 2;

float pid_corr_dia = 42.0 / wheelDiameterIn_mm;  	// original PID parameters have been optimized for small 42mm wheels
																						 			// for diffrent wheel diameters we need a corrective factor for the PID parameters 
																						 			// this linear approach works fine for 42mm, 56mm and 70mm wheels
																						 

/**
* Calibration of gyro and color sensor reference
* in case of an sensor offset this function returns the mean value of
* this offset
* while this function is called/running the robot has to be held in
* 'balanced' position
*/
float calibrate()
{
	float bias = 0.0;
	int numberOfReadings = 500;
	writeDebugStreamLine( "Resetting gyro, keep robot still" );

	eraseDisplay();
	displayCenteredBigTextLine( 2, "SensorTest");

	resetGyro( gyroSensor );
	sleep(1000);

	do
	{
		displayCenteredBigTextLine( 6, "sat:%d",getColorHue( colorSensor ));
		displayCenteredBigTextLine( 9, "dist:%d",getUSDistance( ultrasonicSensor ));
		displayCenteredBigTextLine( 12, "rate:%d",getGyroRate( gyroSensor ));
		sleep(100);
	}while(!getButtonPress(buttonEnter));

	sleep( 1000 );
	eraseDisplay();
	displayCenteredBigTextLine( 6, "GyroCal");
	sleep( 2000 );

	playSound( soundShortBlip );

	bias = 0.0;
	// And now do the actual calibration
	for( int i = 0; i < numberOfReadings; i++ ) {
		bias += getGyroRate( gyroSensor );
		displayCenteredBigTextLine( 9, "rate:%d",bias/(i+1));
		sleep(5);
	}
	bias /= numberOfReadings;
	writeDebugStreamLine("gyroRateBias (used): %f", bias );
	sleep( 2000 );
	playSound( soundShortBlip );

	return bias;
}


/**
* Initialize
* just reset the motors and the display
*/
void initialize()
{
	eraseDisplay();

	resetMotorEncoder( rightMotor );
	resetMotorEncoder( leftMotor );
	resetMotorEncoder( liftMotor );

	sleep( 100 );
}


/**
* Read encoders and determine current robot speed and robot position in mm/s and mm.
*
* 8 angles of the encoders are buffered. The encoder speed is the difference of the
* 'actual angle' - the 'eldest angle'
* multiplying this difference with (wheel radius)*(pi/180) / delta T represents the
* robot speed in mm/sec
* the robot position is given by the actual encoder angle * (wheel radius)*(pi/180)
*/
#define MAX_ENCODER_VALUES ( 8 )
void readEncoders(float *r_speed, float *r_position, float dT)
{
	static long encoderValues[ MAX_ENCODER_VALUES ] = {0,0,0,0,0,0,0,0};
	static int encoderValueIndex = 0;

	long avgEncoderValue = ( getMotorEncoder( rightMotor ) + getMotorEncoder( leftMotor ) ) / 2;
	long eldest = encoderValues[ encoderValueIndex ];
	encoderValues[ encoderValueIndex ] = avgEncoderValue;
	if ( ++encoderValueIndex == MAX_ENCODER_VALUES ) encoderValueIndex = 0;

	*r_speed = wheelRadius * (( avgEncoderValue - eldest ) / ( dT * (MAX_ENCODER_VALUES) )) * (pi/180.0);
	*r_position = wheelRadius * avgEncoderValue * (pi/180.0);
}


/**
* Reads gyro and estimates angle and angular velocity
* only the gyro rate is used to 'measure' both paramaters.
* The actual sensor bias is eliminated.
* The bias is updated over time to handle a possible sensor drift
* the gyro angle is calculated by simple integration of the sensor rates
*/
void readGyro(float *g_speed, float *g_angle, float *gyroRateBias,float dT)
{
	// Estimate of the current robotAngleBias due to drifting gyro
	static float robotAngle = 0.0;
	// define gyro rate bias update ratio (calculation is based on mean value)
	float gyroRateBiasUpdateRatio = 0.2;

	float currentGyroRateMeasurement = getGyroRate( gyroSensor );
	// Estimate gyroRateBias rate by updating the value slowly
	// By multiplying with dt we make scale meanUpdateRatio to 1 sec
	*gyroRateBias = *gyroRateBias * ( 1 - dT * gyroRateBiasUpdateRatio ) + currentGyroRateMeasurement * dT * gyroRateBiasUpdateRatio;
	// Angular velocity can be computed directly now
	*g_speed = currentGyroRateMeasurement - *gyroRateBias;
	// And angle is computed based on integration from last epoch
	robotAngle += *g_speed * dT;

	*g_angle = robotAngle;
}





/**
* PID control
* dmp: damping of the integral ( working value is something near but smaller  than 0 )
* sat: saturation of integral ( working value something like 'max_power'/ki )
*
* this PID implementation includes
* - derivative filtering (the derivative term is based on input derivate)
* - anti windup
* - setpoint (reference) weighting
*
*/
typedef struct
{
	float kp;
	float sw;

	float bi;
	float ad;
	float bd;
	float br;
	float saturation_low;
	float saturation_high;

	float input_old;
	float I;						// integral
	float D;						// derivative term
}PID;

/**
* set all gain factors/parameters of the specidfied PID controller
* kp									proprtianl gain
* ki									integral gain
* kd 									derivative gain
* setpoint_weigth			setpoint weight for the proportional
* Tf									derivative filter time constant  (kd/kp)/N  N=2..20
* Tt									anti-windup time constant
* saturation_low			anti-windup upper saturarion level
* saturation_high			anti-windup lower saturation level
* dT									sampling time
*/
void PID_init(PID *pid, float kp, float ki, float kd, float setpoint_weight, float Tf, float Tt, float saturation_low, float saturation_high, float dT)
{
	pid->kp = kp;
	pid->bi = ki * dT;
	pid->ad = Tf / (Tf + dT);
	pid->bd = kd / (Tf + dT);
	pid->br = dT / Tt;
	pid->saturation_low = saturation_low;
	pid->saturation_high = saturation_high;
	pid->sw = setpoint_weight;

	pid->D = 0;						// derivative term
	pid->I = 0;						// error integral
	pid->input_old = 0;			// used to calculate the derivative term based on measured value
}

float PID_calc(PID *pid, float reference, float input, float *curr_err)
{
	*curr_err = reference - input;
	float P = pid->kp * (pid->sw * reference - input);
	pid->D = pid->ad * pid->D - pid->bd * (input - pid->input_old);
	float v = P + pid->I + pid->D;
	float u = v;
	if(u > pid->saturation_high) u = pid->saturation_high;
	else if(u < pid->saturation_low) u = pid->saturation_low;
	pid->I = pid->I + pid->bi * (reference - input) + pid->br * (u - v);
	pid->input_old = input;

	return v;
}
/*****************




/**
* Propagates reference position based on wanted speed and time step.
* Returns updated referencePosition
*/
float position( float lastReferencePosition, float requestedSpeed, float dT )
{
	return lastReferencePosition + requestedSpeed * dT;
}


/**
* Main program
*/
#define ROBOT_SPEED (000.00)				// robot speed mm/sec
#define EDGE_THRESHOLD			10
task main()
{
	// active line follower?
	bool follow = false;						// set true if the robot should follow a line
	bool stopatedge = false;				// if true the robot stops as soon an edge is detected

	float lineRef = 45.0;						// color sensor refernce value for PID control

	// avoid abstacles
	bool sonic = false;							// set true if the robot should stop in case the ultrasonic sensor is detecting 'something'
	bool avoid = false;
	float AvoidOutput = 0;

	bool ir_remote = true;					// activate simple IR remote control
	int steer_left = 0;							// ir steering left motor
	int steer_right = 0;						// ir steering rigth motor

	// this is the requested robot speed in m/s
	float requestedSpeed=ROBOT_SPEED;


	// Counter for counting main loop entries used to measure the mean sample time
	unsigned long counter = 0;

	// Reference position in meters. Robot starts at position 0
	float referencePosition = 0.0;
	// hold the bias values for the gyro (measured in 'calibrate')
	float gyroRateBias;



	// the whole balancing loop consists of 4 PID controllers.
	// (1) 'gyro_speed': contolling the gyro speed
	// (2) 'gyroAngle': controlling the gyro angle
	// (3) 'robot_position': controlling the robot position
	// (4) 'robot_speed': controlling the robot speed

	// gyro PID outputs / controllers
	float pidGyroAngleOutput,gyro_angle_err;							// gyro angle PID output and error
	float pidGyroSpeedOutput,gyro_speed_err;							// gyro speed PID output and error
	float gyroSpeed,gyroAngle;														// holding measured values of the gyro speed and angle
	PID gyro_angle;																				// gyro angle PID controller
	PID gyro_speed;																				// gyro speed PID controller

	// encoder PID outputs / controllers
	float pidRobotPositionOutput,robot_position_err;			// robot position PID output and error
	float pidRobotSpeedOutput,robot_speed_err;						// robot speed PID output and error
	float robotSpeed,robotPosition;  											// holding the measured values for robot position and speed
	PID robot_position;																		// robot position PID controller
	PID robot_speed;																			// robot speed PID controller


	// simple line follower PID output / controller (steering)
	float pidLineOutput,line_err;													// line follower PID output and error
	PID line;																							// line follower PID (fixed refernce == 'lineRef')

	bool blocked = false;																	// used to implement a hysteresis for the ultrasonic sensor

	// motor power for left and right motor
	float rightPower,leftPower;

	// (just for debugging...) Number of times the control loop did not meet its deadline
	int controlLoopTimerOverflow = 0;
	// used to calculate dT (sampling time)
	float time_sum;



	initialize();										// reset motors and display

	gyroRateBias = calibrate();			// get the gyro bias




	// all PID parameters are open to be optimized ;) ... current parameters are working nice but for sure there are better ones
	// for the momnent no derivative terms have been used. I am quite sure that there is some room to use them ;)

	// initialize the PID controls
	// parameters for the gyro control loops
	// void PID_init(PID *pid, float kp, float ki, float kd, float setpoint_weight, float Tf, float Tt, float saturation_low, float saturation_high, float dT)
	//PID_init(&gyro_angle, 35.0, 55.0, 0.25, 1.0, 0.25/35.0/10.0, 0.3, -100, 100, sampleTime/1000.0);  // optimal for 42mm wheels
	//PID_init(&gyro_speed, 1.5, 5.0, 0.001, 1.0, 0.001/1.5/15.0, 0.3, -100, 100, sampleTime/1000.0);		// optimal for 42mm wheels
	
	// supports different wheel diameters: 42mm,56mm,70mm tested
	PID_init(&gyro_angle, 35.0 * pid_corr_dia, 55.0 * pid_corr_dia, 0.25 * pid_corr_dia , 1.0, (0.25 * pid_corr_dia) / (35.0 * pid_corr_dia) / 10.0, 0.3, -100, 100, sampleTime/1000.0);
	PID_init(&gyro_speed, 1.5 * pid_corr_dia, 5.0 * pid_corr_dia, 0.001 * pid_corr_dia, 1.0, (0.001 * pid_corr_dia) / (1.5 * pid_corr_dia) / 15.0, 0.3, -100, 100, sampleTime/1000.0);


	// parameters for the encoder control loop (robot position and speed))
	// the output of both controllers (robot position and speed) must be 'subtracted' from the motor power.
	//PID_init(&robot_position, 1.5, 1.0, 0.01, 0.2, 0.01/1.5/10.0, 0.3, -100, 100, sampleTime/1000.0);  // optimal for 42mm wheels
	//PID_init(&robot_speed, 0.47, 0.16, 0.01, 0.2, 0.01/0.47/10.0, 0.3, -100, 100, sampleTime/1000.0);  // optimal for 42mm wheels
	
	// supports different wheel diameters: 42mm,56mm,70mm tested
	PID_init(&robot_position, 1.5 * pid_corr_dia, 1.0  * pid_corr_dia, 0.01 * pid_corr_dia, 0.2, (0.01 * pid_corr_dia) / (1.5 * pid_corr_dia) / 10.0, 0.3, -100, 100, sampleTime/1000.0);
	PID_init(&robot_speed,   0.47 * pid_corr_dia, 0.16 * pid_corr_dia, 0.01 * pid_corr_dia, 0.2, (0.01 * pid_corr_dia) / (0.47 * pid_corr_dia) / 10.0, 0.3, -100, 100, sampleTime/1000.0);


	// simple line follower PID --> steering
	// error == getColorHue - lineRef, lineRef == initial value ('edge' of line)
	PID_init(&line, 1.0, 0.3, 0.0, 1.0, 0.0, 1.0, -100, 100, sampleTime/1000.0);				// working fine for 5cm/sec

	eraseDisplay();

	writeDebugStreamLine("*** Starting main loop");

	float dT = (float)sampleTime / 1000.0;						// 'requested' sample time

	resetTimer( T1 );																	// time is used to measure the true sample time
	time_sum = 0.0;																		// prepare sample time measurement


	// balance the robot till the enter button is pressed
	while( !getButtonPress(buttonEnter) ) {

			// check if something is blocking the way ... if yes just stop and just balance (robot speed = 0)
			if(sonic)
			{
				if(!blocked)
				{
					if(getUSDistance( ultrasonicSensor ) < 50)
					{
						requestedSpeed=0.0;
						blocked = true;
					}
				}
				else if(getUSDistance( ultrasonicSensor ) > 100)
				{
					requestedSpeed=ROBOT_SPEED;
					blocked = false;
				}
			}

			// PID control of robot speed and position
			// measure current robot position and robot speed
			readEncoders(&robotSpeed,&robotPosition, dT);
			// Get expected position based on current position and requested speed
			referencePosition = position( referencePosition, requestedSpeed, dT );
			// controllers for robot position and speed.
			pidRobotPositionOutput = PID_calc(&robot_position, referencePosition, robotPosition, &robot_position_err );		// reference is 'next' position (depending on the requested robot speed)
			pidRobotSpeedOutput = PID_calc(&robot_speed, requestedSpeed, robotSpeed, &robot_speed_err );									// reference is the requested robot speed


			// PID control gyro angle and speed
			// Read the gyro and update gyroRateBias and robot angle
			readGyro(&gyroSpeed, &gyroAngle, &gyroRateBias, dT);
			// controller for gyro based balancing. Reference is 0 for speed and angle to balance the robot
			pidGyroAngleOutput = PID_calc(&gyro_angle, 0, gyroAngle, &gyro_angle_err );
			pidGyroSpeedOutput = PID_calc(&gyro_speed, 0, gyroSpeed, &gyro_speed_err );



			// just show the current measurements and PID error of the robot position and speed
			// to get an idea if everything is controlled as expected ... debug
			displayCenteredBigTextLine( 1, "rsp:%f",robotSpeed);
			displayCenteredBigTextLine( 3, "ser:%f",robot_speed_err);
			displayCenteredBigTextLine( 5, "ref:%f",referencePosition);
			displayCenteredBigTextLine( 7, "pos:%f",robotPosition);
			displayCenteredBigTextLine( 9, "per:%f",robot_position_err);
			//displayCenteredBigTextLine( 9, "per:%f",getColorHue (colorSensor));



			// simple line follower controll
			// depending on the physical properties of your line you may need
			// to adapt lineRef ... (default == 45)
			if(follow)
		  {
				// PID control line follower
		  	pidLineOutput = PID_calc(&line, lineRef, getColorHue (colorSensor), &line_err);
				// limit the 'steering power'
		  	if(pidLineOutput>STEERING_LIMIT) pidLineOutput=STEERING_LIMIT;
				else if(pidLineOutput<-STEERING_LIMIT) pidLineOutput=-STEERING_LIMIT;
		  }
		  else pidLineOutput = 0.0;		// do not follow the line



			if(avoid)
		  {
				if(getUSDistance( ultrasonicSensor ) < 80) AvoidOutput = 15; // turn till the way is free ;)
		  	else AvoidOutput = 0;
			}
		  else AvoidOutput = 0;		// clear way



			if(ir_remote)
			{
		  	switch(getIRRemoteButtons(irSensor))
		  	{
		  		case IR_RED_UP: requestedSpeed += 1.0; break;
		  		case IR_RED_DOWN: requestedSpeed -= 1.0;break;
		  		case IR_BLUE_UP: {steer_right += 1; steer_left -= 1;};break;
		  		case IR_BLUE_DOWN :	{steer_right -= 1; steer_left += 1;};break;
		  		case IR_RED_UP_RED_DOWN : requestedSpeed = 0.0; break;
		  		case IR_BLUE_UP_BLUE_DOWN : {steer_right = 0; steer_left = 0;};break;
		  		case IR_BEACON_MODE_ON : {requestedSpeed = 0.0; steer_right = 0; steer_left = 0;};break;
		  		case IR_RED_UP_BLUE_UP : setMotor(liftMotor, -5);break;
		  		case IR_RED_DOWN_BLUE_DOWN : setMotor(liftMotor, 5);break;
		  		default: setMotor(liftMotor, 0);
		  	}
		  }


		  if(stopatedge && (getIRRemoteButtons(irSensor) == IR_NONE))
		  {
		  	if(getColorHue (colorSensor) < EDGE_THRESHOLD)
		  	{
		  		// stop the robot. The sensor is telling us the there is an endge in front of the wheels
		  		requestedSpeed = 0.0;
		  		steer_right = 0;
		  		steer_left = 0;
		  		pidLineOutput = 0;
		  		AvoidOutput = 0;

		  	}
		  }


		  // please consider that 'pidRobotPositionOutput' and 'pidRobotSpeedOutput' are subtracted (see explanation above)
		  float average_power = -pidRobotPositionOutput-pidRobotSpeedOutput+pidGyroAngleOutput+pidGyroSpeedOutput;
			rightPower = average_power + pidLineOutput + AvoidOutput + steer_right;
			leftPower  = average_power - pidLineOutput - AvoidOutput + steer_left;


			// limit power for both motors (100 == maximum)
			if(rightPower>POWER_LIMIT) rightPower=POWER_LIMIT;
			else if(rightPower<-POWER_LIMIT) rightPower=-POWER_LIMIT;
			if(leftPower>POWER_LIMIT) leftPower=POWER_LIMIT;
			else if(leftPower<-POWER_LIMIT) leftPower=-POWER_LIMIT;

			setMotor( rightMotor, rightPower );
			setMotor( leftMotor, leftPower );

			// check if we have a sample time overflow
			// should never happen. In case just increase 'sampleTime'
			if ( getTimer( T1, milliseconds ) > sampleTime ) controlLoopTimerOverflow++;

			// measure mean dt (== loop duration == sample time)
			while( getTimer( T1, milliseconds ) < sampleTime  ) {};			// wait till we have sampleTime
			time_sum += getTimer( T1, milliseconds ) / 1000;
			resetTimer( T1 );
			dT = time_sum / (++counter);
		}

		setMultipleMotors( 0, leftMotor, rightMotor );

		eraseDisplay();
		displayCenteredBigTextLine( 4, "OVF: %4d", controlLoopTimerOverflow );


		sleep(30000);

	}
