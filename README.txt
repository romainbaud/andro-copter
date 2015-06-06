AndroCopter - the flying smartphone
Romain Baud

What is Androcopter?
AndroCopter is a project that aims to build an autonomous quadcopter with an Android smartphone as the onboard flight computer. The Arduino ADK board links the phone and the quadcopter electronics.
Note that the project is still experimental, and that quadcopters can be very dangerous. Be careful!

Why?
Now, smartphones are very powerful, have a lot of sensors, and are easy to program.
For example, the Nexus 4 has a lot of hardware features, useful for a quadcopter:
-Accelerometer, gyrometer, magnetometer for orientation estimation.
-GPS and barometer for positioning.
-Wi-Fi and 3G for communicating with the ground computer.
-Two cameras.
-Quad-core CPU and GPU for fast control.
-Large storage space to record movies, take pictures, log flight data...

What is working now? What features will be implemented next?
For the moment, it is possible to manually fly the AndroCopter, using a gamepad. It is also possible to take pictures from the air.
These are some features that could be added later:
-waypoints navigation, using the GPS and the barometer.
-optic flow with the bottom camera, for indoor horizontal stabilization.
-first-person view flight, using the front camera and a mirror.

How is the project organized?
There are three softwares folders:
-Android: the app to install on the phone, written in Java. Open it as an eclipse project (http://developer.android.com/sdk/index.html).
-Arduino: the sketch to upload on the ADK. Use the Arduino IDE (http://arduino.cc/en/Main/Software).
-PC: this is the PC software, written in C++.
The Hardware folder contains some drawings and schematics to actually build an AndroCopter.

How to compile the PC software?
1. Install the Qt 5 (http://qt-project.org/downloads) and SFML 2 (http://www.sfml-dev.org/download.php) libraries at a known location.
2. Edit the end of the AndroCopter.pro file, to make two pathes match your actual SFML install directory.
3. Compile using Qt Creator, or just do "qmake && make" in a terminal (Qt command prompt on Windows).

What hardware is needed?
Smartphone: for the moment, AndroCopter has only be tested with a Nexus 4. Other smartphone may work, but a gyrometer and a barometer is necessary.
Arduino board: only the Arduino ADK 2011 is supported (http://developer.android.com/tools/adk/adk.html). Other boards are likely not compatible, due to the lack of USB host.
Quadcopter base: use the motors, ESCs and propellers you like. Only PPM-controlled ESCs (the most common) are supported.