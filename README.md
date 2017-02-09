# LineBalancer
Lego Mindstorms EV3 balancing line follower robot in robotC. Fully PID controlled.

you need:
  - robotC ... you can download a trial version under 'http://www.robotc.net/' ... make sure to activate the 'natural language' option
  - a 'balancer robot' (e.g. balanc3r with the sensor on the left side!!!, or gyroboy or something similar...)
  - motors connected to B and C.
  - the gyro (connected to S2) should be mounted on the left side (left hand of the robot) of the robot (the higher the better)
if you want combine the balancing challenge with a line follower you need the light sensor somewhere
near the floor in front of the wheels connected to S3.
the ultrasonic (S1) sensor can be used to prevent collisions if the robot is moving (robot simply stops)


what is new here?

the 'original' lego balancing example is using a quite simple approach to
solve the balancing problem. The lego LabView solution uses a simple
linear combination of the gyro speed, gyro angle, robot speed and robot position
to calculate the power that is needed to balance the robot
(finding a working linear combination must have been a nightmare job).
This solution is very sensitive and the room for optimizations is very small

Laurens Valk has enhanced Legos simple approach by adding a single PID controller (great job). Still
the same linear combination of the same parameters is used. Adding the PID controller goes hand in hand
with improved stability and some more room for optimizations (but still quite limited)
This solution goes hand in hand with the fact the the PID reference is always 0 even when we want
a moving robot. It's fore sure a nice solution but not really a 'classic' PID control loop where
we normally want to control a specific physical parameter like the robot speed directly.

the following approach is using 4 parallel working PID controllers

 (1) 'gyro_speed': contolling the gyro speed
 (2) 'gyroAngle': controlling the gyro angle
 (3) 'robot_position': controlling the robot position
 (4) 'robot_speed': controlling the robot speed

this approach gives a lot of room for parameter optimizations and is a very good example how a
combination of several PID controllers can work. The basic idea is explained below in more detail.


why do we need all these parameters to balance a robot?

I am sure that there are a lot of robot fans out there that have tried to balance
a robot just by using a simple control loop using the gyro.
Imagine the robot is balanced and starts to fall forward. A simple PID conntrol loop approach
could use just the gyro angle to calculate the motor power that is needed to reduce the angle
(the error) again. If the parameters are chosen properly the robot will be in the balanced status
again (anytime soon). The problem is that there is no need (from the physical point of view) to reduce
(no 'force' that is reducing) the speed of the robot (the speed is the result of the acceleration phase
that was necessary to get the robot back into the balanced status).
As a consequence the robot is balanced for a while but the speed/position is not controlled
and so you end up with a robot at full speed (after a quite short time) without the possibility to control the balancing anymore.
So it is clear that we have to control the speed/position of the robot somehow.
the gyro control loops in this solution are nothing special. The motor power is 'proportional' (depending on the PID parameters)
to the gyro angle/speed (obviously the reference for these PID controllers is 0 here).

for the speed/position controller loop we have to consider the following fact:
imagine that the robot is a few mm behind the ideal reference position in a 'perfectly' balanced status.
So we have to move forward to correct the position (to minimize the position error).
To move forward it is not 'allowed' just to move forward because the robot would fall onto the backside.
To move forward the robot must move backwards first and than accelerate forward (here the gyro control loop is doing the job)
To balance again (this is the way a balancing robot can make a 'step' forward).
In other words we have to set the motor power to negative values to be able to move forward afterwards to reach the
reference position (that means that we are increasing the position PID error in order to get the possibility to minimize the same
error afterwards. The bigger the distance to the reference point in our example (the bigger the error) the more
we must accelerate/move into the opposite direction to make a bigger step forward afterwards
(the acceleration forward is 'triggered' by the gyro control loops as soon the robot starts to fall forward)
(--> so a 'classic' PID approach should work fine but the output of the position and speed PID must be weighted
negative to the motor power)
the same concept/background is valid for the robot speed ...


for those who are interested in the mathematical background I can recommend
https://en.wikipedia.org/wiki/PID_controller
the wiki article is covering some complex mathematical theories and you might have to learn a lot more to be able to understand everything in detail.
a very nice 'easy to understand' article about PID controllers you can find under
http://www.inpharmix.com/jps/PID_Controller_For_Lego_Mindstorms_Robots.html
