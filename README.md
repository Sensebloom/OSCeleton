OSCeleton
=========

What is this?
-------------

As the title says, it's just a small program that takes kinect
skeleton data from the OpenNI framework and spits out the coordinates
of the skeleton's joints via OSC messages. These can can then be used
on your language / framework of choice.


How do I use it?
----------------

### First you need to install the OpenNI driver, framework, and middleware
#### Windows / Linux / Mac OSX
Get avin's hacked Primesense PSDK driver for kinect:
[https://github.com/avin2/SensorKinect](https://github.com/avin2/SensorKinect)
Folow his instructions for installing the OpenNI framwork, the driver,
and the NITE middleware.

#### After OpenNI / NITE is working
Then you can run one of the precompiled binaries in the "bin"
directory or compile your own:

on Linux:
    make

on OSX you can use the precompiled binary in bin/OSX or compile with:
    make osceleton-osx
> NOTE FOR MAC USERS: You must run OSCeleton from the terminal or it
> will not run correctly.

on windows: you can use the precompiled binary in bin\win32 or use the
VC++ express .sln file.

If you run the executable without any arguments, it will send the OSC
messagens in the default format to localhost on port 7110.
To learn about the OSC message format, continue reading below or check
out our processing examples at
[https://github.com/Sensebloom/OSCeleton-examples](https://github.com/Sensebloom/OSCeleton-examples)

#### Other stuff
Another fun way to test OSCeleton is to use the awesome animata
skeletal animation software by the Kitchen Budapest guys. You can get
it at:
[http://animata.kibu.hu/](http://animata.kibu.hu/)

Animata needs its OSC messages in a very specific format, so you must
use the "-k" ("kitchen" mode) option. Multiplying the x and y
coordinates and adding some offsets can also be useful to tune the
size of the skeleton, try running it like this:
    OSCeleton.exe -k -mx 640 -my 480 -ox -160

If your animation is going crazy try to play with -mx and -my values,
and -ox and -oy values a bit.

To get a complete list of available options run OSCeleton -h.


OSC Message format
------------------

### New user detected - no skeleton available yet. This is a good time
### for you to ask the user to do the calibration pose:

    Address pattern: "/new_user"
    Type tag: "i"
    i: A numeric ID attributed to the new user.


### New skeleton detected - The calibration was finished successfully,
### joint coordinate messages for this user will be incoming soon ;):

    Address pattern: "/new_skel"
    Type tag: "i"
    i: ID of the user whose skeleton is detected.


### Lost user - we have lost the user with the following id:

    Address pattern: "/lost_user"
    Type tag: "i"
    i: The ID of the lost user. (This ID will be free for reuse from now on)


### Joint message - message with the coordinates of each skeleton
### joint:

    Address pattern: "/joint"
    Type tag: "sifff"
    s: Joint name, check out the full list of joints below.
    i: The ID of the user.
    f: X coordinate of joint in interval [0.0, 1.0]
    f: Y coordinate of joint in interval [0.0, 1.0]
    f: Z coordinate of joint in interval [0.0, 7.0]

If you use "-k" option, the typetag will be "sff", and no user ID or Z
coordinate will be sent. new_user, new_skel and lost_user messages
will not be sent either. This is used for compatibility with the
excellent Kitchen Budapest animata skeletal animation software.


### Full list of joints

* head
* neck
* torso

* r_collar #not working yet
* r_shoulder
* r_elbow
* r_wrist #not working yet
* r_hand
* r_finger #not working yet

* l_collar #not working yet
* l_shoulder
* l_elbow
* l_wrist #not working yet
* l_hand
* l_finger #not working yet

* r_hip
* r_knee
* r_ankle
* r_foot

* l_hip
* l_knee
* l_ankle
* l_foot


Other
-----

### For death threats and other stuff, come join the fun in our [google group](http://groups.google.com/group/osceleton)!

Have fun!
