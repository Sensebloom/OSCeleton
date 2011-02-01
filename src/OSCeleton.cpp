/***********************************************************************

    OSCeleton - OSC proxy for kinect skeleton data.
    Copyright (C) <2010>  <Sensebloom lda.>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************/

#include <cstdio>
#include <csignal>

#include <XnCppWrapper.h>

#include <ip/UdpSocket.h>
#include <osc/OscOutboundPacketStream.h>

#include "common.h"



char *ADDRESS = "127.0.0.1";
int PORT = 7110;

#define OUTPUT_BUFFER_SIZE 1024
char osc_buffer[OUTPUT_BUFFER_SIZE];
UdpTransmitSocket *transmitSocket;

char tmp[50]; //Temp buffer for OSC address pattern
int userID;
float jointCoords[3];

//Multipliers for coordinate system. This is useful if you use
//software like animata, that needs OSC messages to use an arbitrary
//coordinate system.
double mult_x = 1;
double mult_y = 1;
double mult_z = 1;

//Offsets for coordinate system. This is useful if you use software
//like animata, that needs OSC messages to use an arbitrary coordinate
//system.
double off_x = 0.0;
double off_y = 0.0;
double off_z = 0.0;

bool kitchenMode = false;
bool mirrorMode = true;
bool play = false;
bool record = false;
bool sendRot = false;
bool filter = false;
bool preview = false;
int nDimensions = 3;

void (*oscFunc)(osc::OutboundPacketStream*, char*) = NULL;

xn::Context context;
xn::DepthGenerator depth;
xn::DepthMetaData depthMD;
xn::UserGenerator userGenerator;
XnChar g_strPose[20] = "";



// Callback: New user was detected
void XN_CALLBACK_TYPE new_user(xn::UserGenerator& generator, XnUserID nId, void* pCookie) {
	printf("New User %d\n", nId);
	userGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);

	if (kitchenMode) return;

	osc::OutboundPacketStream p(osc_buffer, OUTPUT_BUFFER_SIZE);
	p << osc::BeginBundleImmediate;
	p << osc::BeginMessage("/new_user");
	p << (int)nId;
	p << osc::EndMessage;
	p << osc::EndBundle;
	transmitSocket->Send(p.Data(), p.Size());
}



// Callback: An existing user was lost
void XN_CALLBACK_TYPE lost_user(xn::UserGenerator& generator, XnUserID nId, void* pCookie) {
	printf("Lost user %d\n", nId);

	if (kitchenMode) return;

	osc::OutboundPacketStream p( osc_buffer, OUTPUT_BUFFER_SIZE );
	p << osc::BeginBundleImmediate;
	p << osc::BeginMessage("/lost_user");
	p << (int)nId;
	p << osc::EndMessage;
	p << osc::EndBundle;
	transmitSocket->Send(p.Data(), p.Size());
}



// Callback: Detected a pose
void XN_CALLBACK_TYPE pose_detected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie) {
	printf("Pose %s detected for user %d\n", strPose, nId);
	userGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
	userGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}



// Callback: Started calibration
void XN_CALLBACK_TYPE calibration_started(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie) {
	printf("Calibration started for user %d\n", nId);
}



// Callback: Finished calibration
void XN_CALLBACK_TYPE calibration_ended(xn::SkeletonCapability& capability, XnUserID nId, XnBool bSuccess, void* pCookie) {
	if (bSuccess) {
		printf("Calibration complete, start tracking user %d\n", nId);
		userGenerator.GetSkeletonCap().StartTracking(nId);

		if (kitchenMode) return;

		osc::OutboundPacketStream p( osc_buffer, OUTPUT_BUFFER_SIZE );
		p << osc::BeginBundleImmediate;
		p << osc::BeginMessage( "/new_skel" );
		p << (int)nId;
		p << osc::EndMessage;
		p << osc::EndBundle;
		transmitSocket->Send(p.Data(), p.Size());
	}
	else {
		printf("Calibration failed for user %d\n", nId);
		userGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}
}



int jointPos(XnUserID player, XnSkeletonJoint eJoint) {
	XnSkeletonJointPosition joint;
	userGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint, joint);

	if (joint.fConfidence < 0.5)
		return -1;

	userID = player;
	jointCoords[0] = off_x + (mult_x * (1280 - joint.position.X) / 2560); //Normalize coords to 0..1 interval
	jointCoords[1] = off_y + (mult_y * (960 - joint.position.Y) / 1920); //Normalize coords to 0..1 interval
	jointCoords[2] = off_z + (mult_z * joint.position.Z * 7.8125 / 10000); //Normalize coords to 0..7.8125 interval

//for (int i=0; i<9; i++)
	//	jointRots[i] = joint.orientation.orientation.elements[i];

	return 0;
}



// Generate OSC message with default format
void genOscMsg(osc::OutboundPacketStream *p, char *name) {
	*p << osc::BeginMessage( "/joint" );
	*p << name;
	if (!kitchenMode)
		*p << userID;
	for (int i = 0; i < nDimensions; i++)
		*p << jointCoords[i];
	//for (int i=0; i < 9; i++)
	//	*p << (float)jointRots[i];
	*p << osc::EndMessage;
}



// Generate OSC message with Quartz Composer format - based on Steve Elbows's code ;)
void genQCMsg(osc::OutboundPacketStream *p, char *name) {
	sprintf(tmp, "/joint/%s/%d", name, userID);
	*p << osc::BeginMessage(tmp);
	for (int i = 0; i < nDimensions; i++)
		*p << jointCoords[i];
	*p << osc::EndMessage;
}



void sendUserPosMsg(XnUserID id) {
	osc::OutboundPacketStream p(osc_buffer, OUTPUT_BUFFER_SIZE);
	XnPoint3D com;
	sprintf(tmp, "/user/%d", id);
	p << osc::BeginBundleImmediate;
	p << osc::BeginMessage(tmp);
	userGenerator.GetCoM(id, com);
	p << (float)(off_x + (mult_x * (1280 - com.X) / 2560));
	p << (float)(off_y + (mult_y * (1280 - com.Y) / 2560));
	p << (float)(off_z + (mult_z * com.Z * 7.8125 / 10000));
	p << osc::EndMessage;
	p << osc::EndBundle;
	transmitSocket->Send(p.Data(), p.Size());
}



void sendOSC() {
	XnUserID aUsers[15];
	XnUInt16 nUsers = 15;
	userGenerator.GetUsers(aUsers, nUsers);
	for (int i = 0; i < nUsers; ++i) {
		if (userGenerator.GetSkeletonCap().IsTracking(aUsers[i])) {
			osc::OutboundPacketStream p(osc_buffer, OUTPUT_BUFFER_SIZE);
			p << osc::BeginBundleImmediate;

			if (jointPos(aUsers[i], XN_SKEL_HEAD) == 0) {
				oscFunc(&p, "head");
			}
			if (jointPos(aUsers[i], XN_SKEL_NECK) == 0) {
				oscFunc(&p, "neck");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_COLLAR) == 0) {
				oscFunc(&p, "l_collar");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_SHOULDER) == 0) {
				oscFunc(&p, "l_shoulder");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_ELBOW) == 0) {
				oscFunc(&p, "l_elbow");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_WRIST) == 0) {
				oscFunc(&p, "l_wrist");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_HAND) == 0) {
				oscFunc(&p, "l_hand");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_FINGERTIP) == 0) {
				oscFunc(&p, "l_fingertip");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_COLLAR) == 0) {
				oscFunc(&p, "r_collar");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_SHOULDER) == 0) {
				oscFunc(&p, "r_shoulder");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_ELBOW) == 0) {
				oscFunc(&p, "r_elbow");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_WRIST) == 0) {
				oscFunc(&p, "r_wrist");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_HAND) == 0) {
				oscFunc(&p, "r_hand");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_FINGERTIP) == 0) {
				oscFunc(&p, "r_fingertip");
			}
			if (jointPos(aUsers[i], XN_SKEL_TORSO) == 0) {
				oscFunc(&p, "torso");
			}
			if (jointPos(aUsers[i], XN_SKEL_WAIST) == 0) {
				oscFunc(&p, "waist");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_HIP) == 0) {
				oscFunc(&p, "l_hip");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_KNEE) == 0) {
				oscFunc(&p, "l_knee");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_ANKLE) == 0) {
				oscFunc(&p, "l_ankle");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_FOOT) == 0) {
				oscFunc(&p, "l_foot");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_HIP) == 0) {
				oscFunc(&p, "r_hip");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_KNEE) == 0) {
				oscFunc(&p, "r_knee");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_ANKLE) == 0) {
				oscFunc(&p, "r_ankle");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_FOOT) == 0) {
				oscFunc(&p, "r_foot");
			}

			p << osc::EndBundle;
		    transmitSocket->Send(p.Data(), p.Size());
		}
		else {
			//Send user's center of mass
			sendUserPosMsg(aUsers[i]);
		}
	}
}



int usage(char *name) {
	printf("\nUsage: %s [OPTIONS]\n\
Example: %s -a 127.0.0.1 -p 7110 -d 3 -n 1 -mx 1 -my 1 -mz 1 -ox 0 -oy 0 -oz 0\n\
\n\
(The above example corresponds to the defaults)\n\
\n\
Options:\n\
  -a <addr>\t Address to send OSC packets to (default: localhost).\n\
  -p <port>\t Port to send OSC packets to (default: 7110).\n\
  -w\t\t Activate depth view window.\n\
  -mx <n>\t Multiplier for X coordinates.\n\
  -my <n>\t Multiplier for Y coordinates.\n\
  -mz <n>\t Multiplier for Z coordinates.\n\
  -ox <n>\t Offset to add to X coordinates.\n\
  -oy <n>\t Offset to add to Y coordinates.\n\
  -oz <n>\t Offset to add to Z coordinates.\n\
  -r\t\t Reverse image (disable mirror mode).\n\
  -f\t\t Activate noise filter to reduce jerkyness.\n\
  -k\t\t Enable \"Kitchen\" mode (Animata compatibility mode).\n\
  -q\t\t Enable Quartz Composer OSC format.\n\
  -s <file>\t Save to file (only .oni supported at the moment).\n\
  -i <file>\t Play from file (only .oni supported at the moment).\n\
  -h\t\t Show help.\n\n\
For a more detailed explanation of options consult the README file.\n\n",
		   name, name);
	exit(1);
}



void checkRetVal(XnStatus nRetVal) {
	if (nRetVal != XN_STATUS_OK) {
		printf("There was a problem initializing kinect... Make sure you have \
connected both usb and power cables and that the driver and OpenNI framework \
are correctly installed.\n\n");
		exit(1);
	}
}



void terminate(int ignored) {
	context.Shutdown();
	delete transmitSocket;
	if (preview)
		glutDestroyWindow(window);
	exit(0);
}



void main_loop() {
	// Read next available data
	context.WaitAnyUpdateAll();
	// Process the data
	depth.GetMetaData(depthMD);
	sendOSC();
	if (preview)
		draw();
}



int main(int argc, char **argv) {
	unsigned int arg = 1,
				 require_argument = 0;
	XnMapOutputMode mapMode;
	XnStatus nRetVal = XN_STATUS_OK;
	XnCallbackHandle hUserCallbacks, hCalibrationCallbacks, hPoseCallbacks;
	xn::Recorder recorder;

	context.Init();

	while ((arg < argc) && (argv[arg][0] == '-')) {
		switch (argv[arg][1]) {
			case 'a':
			case 'p':
			case 'm':
			case 'o':
				require_argument = 1;
				break;
			default:
				require_argument = 0;
				break;
		}

		if ( require_argument && arg+1 >= argc ) {
			printf("The option %s require an argument.\n", argv[arg]);
			usage(argv[0]);
		}

		switch (argv[arg][1]) {
		case 'h':
			usage(argv[0]);
			break;
		case 'a': //Set ip address
			ADDRESS = argv[arg+1];
			break;
		case 'p': //Set port
			if(sscanf(argv[arg+1], "%d", &PORT) == EOF ) {
				printf("Bad port number given.\n");
				usage(argv[0]);
			}
			break;
		case 'w':
			preview = true;
			break;
		case 's':
			checkRetVal(recorder.Create(context));
			checkRetVal(recorder.SetDestination(XN_RECORD_MEDIUM_FILE, argv[arg+1]));
			record = true;
			arg++;
			break;
		case 'i':
			checkRetVal(context.OpenFileRecording(argv[arg+1]));
			play = true;
			arg++;
			break;
		case 'm': //Set multipliers
			switch(argv[arg][2]) {
			case 'x': // Set X multiplier
				if(sscanf(argv[arg+1], "%lf", &mult_x)  == EOF ) {
					printf("Bad X multiplier.\n");
					usage(argv[0]);
				}
				break;
			case 'y': // Set Y multiplier
				if(sscanf(argv[arg+1], "%lf", &mult_y)  == EOF ) {
					printf("Bad Y multiplier.\n");
					usage(argv[0]);
				}
				break;
			case 'z': // Set Z multiplier
				if(sscanf(argv[arg+1], "%lf", &mult_z)  == EOF ) {
					printf("Bad Z multiplier.\n");
					usage(argv[0]);
				}
				break;
			default:
				printf("Bad multiplier option given.\n");
				usage(argv[0]);
			}
			break;
		case 'o': //Set offsets
			switch(argv[arg][2]) {
			case 'x': // Set X offset
				if(sscanf(argv[arg+1], "%lf", &off_x)  == EOF ) {
					printf("Bad X offset.\n");
					usage(argv[0]);
				}
				break;
			case 'y': // Set Y offset
				if(sscanf(argv[arg+1], "%lf", &off_y)  == EOF ) {
					printf("Bad Y offset.\n");
					usage(argv[0]);
				}
				break;
			case 'z': // Set Z offset
				if(sscanf(argv[arg+1], "%lf", &off_z)  == EOF ) {
					printf("Bad Z offset.\n");
					usage(argv[0]);
				}
				break;
			default:
				printf("Bad offset option given.\n");
				usage(argv[0]);
			}
			break;
		// case 't':
		// 	sendRot = true;
		// break;
		case 'f':
			filter = true;
			break;
		case 'k':
			kitchenMode = true;
			break;
		case 'q': // Set Quartz Composer mode
			oscFunc = &genQCMsg;
			break;
		case 'r':
			mirrorMode = false;
			break;
		default:
			printf("Unrecognized option.\n");
			usage(argv[0]);
		}
		if ( require_argument )
			arg += 2;
		else
			arg ++;
	}

	if (kitchenMode)
		nDimensions = 2;
	if (oscFunc == NULL)
		oscFunc = genOscMsg;

	checkRetVal(depth.Create(context));

	if (!play) {
		mapMode.nXRes = XN_VGA_X_RES;
		mapMode.nYRes = XN_VGA_Y_RES;
		mapMode.nFPS = 30;
		depth.SetMapOutputMode(mapMode);
	}

	nRetVal = context.FindExistingNode(XN_NODE_TYPE_USER, userGenerator);
	if (nRetVal != XN_STATUS_OK)
		nRetVal = userGenerator.Create(context);

	checkRetVal(userGenerator.RegisterUserCallbacks(new_user, lost_user, NULL, hUserCallbacks));
	checkRetVal(userGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(calibration_started, calibration_ended, NULL, hCalibrationCallbacks));
	checkRetVal(userGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(pose_detected, NULL, NULL, hPoseCallbacks));
	checkRetVal(userGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose));
	checkRetVal(userGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL));
	if (filter)
		userGenerator.GetSkeletonCap().SetSmoothing(0.8);
	xnSetMirror(depth, !mirrorMode);

	transmitSocket = new UdpTransmitSocket(IpEndpointName(ADDRESS, PORT));
	signal(SIGTERM, terminate);
	signal(SIGINT, terminate);

	printf("Configured to send OSC messages to %s:%d\n", ADDRESS, PORT);
	printf("Multipliers (x, y, z): %f, %f, %f\n", mult_x, mult_y, mult_z);
	printf("Offsets (x, y, z): %f, %f, %f\n", off_x, off_y, off_z);

	printf("OSC Message format: ");
	if (kitchenMode)
		printf("Kitchen (Animata compatibility)\n");
	else if (oscFunc == genQCMsg)
		printf("Quartz Composer\n");
	else
		printf("Default OSCeleton format\n");

	printf("Initialized Kinect, looking for users...\n\n");
	context.StartGeneratingAll();

	if (record)
		recorder.AddNodeToRecording(depth, XN_CODEC_16Z_EMB_TABLES);

	if (preview) {
		init_window(argc, argv, 640, 480, main_loop);
		glutMainLoop();
	}
	else {
		while(true)
			main_loop();
	}

	terminate(0);
}

