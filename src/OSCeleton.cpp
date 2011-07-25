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

#include <lo/lo.h>

#include "common.h"



char *ADDRESS = "127.0.0.1";
char *PORT = "7110";

#define OUTPUT_BUFFER_SIZE 1024*16
char osc_buffer[OUTPUT_BUFFER_SIZE];

char tmp[50]; //Temp buffer for OSC address pattern
int userID;
float jointCoords[3];
float jointOrients[9];

float posConfidence;
float orientConfidence;

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

// hand data
float handCoords[3];
bool haveHand = false;

bool kitchenMode = false;
bool handMode = false;
bool mirrorMode = true;
bool play = false;
bool record = false;
bool sendRot = false;
bool filter = false;
bool preview = false;
bool raw = false;
bool sendOrient = false;
int nDimensions = 3;

void (*oscFunc)(lo_bundle*, char*) = NULL;

xn::Context context;
xn::DepthGenerator depth;
xn::DepthMetaData depthMD;
xn::UserGenerator userGenerator;
xn::HandsGenerator handsGenerator;
xn::GestureGenerator gestureGenerator;
lo_address addr;

XnChar g_strPose[20] = "";
#define GESTURE_TO_USE "Wave"


//gesture callbacks
void XN_CALLBACK_TYPE Gesture_Recognized(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pIDPosition, const XnPoint3D* pEndPosition, void* pCookie) {
    printf("Gesture recognized: %s\n", strGesture);
    gestureGenerator.RemoveGesture(strGesture);
    handsGenerator.StartTracking(*pEndPosition);
}

void XN_CALLBACK_TYPE Gesture_Process(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, XnFloat fProgress, void* pCookie) {
}

//hand callbacks new_hand, update_hand, lost_hand
void XN_CALLBACK_TYPE new_hand(xn::HandsGenerator &generator, XnUserID nId, const XnPoint3D *pPosition, XnFloat fTime, void *pCookie) {
	printf("New Hand %d\n", nId);
	if (kitchenMode) return;

	lo_send(addr, "/new_user", NULL);
}
void XN_CALLBACK_TYPE lost_hand(xn::HandsGenerator &generator, XnUserID nId, XnFloat fTime, void *pCookie) {
	printf("Lost Hand %d               \n", nId);
    gestureGenerator.AddGesture(GESTURE_TO_USE, NULL);

	if (kitchenMode) return;

	lo_send(addr, "/lost_user", NULL);
}
void XN_CALLBACK_TYPE update_hand(xn::HandsGenerator &generator, XnUserID nID, const XnPoint3D *pPosition, XnFloat fTime, void *pCookie) {
	haveHand = true;
	handCoords[0] = pPosition->X;
	handCoords[1] = pPosition->Y;
	handCoords[2] = pPosition->Z;
}

// Callback: New user was detected
void XN_CALLBACK_TYPE new_user(xn::UserGenerator& generator, XnUserID nId, void* pCookie) {
	printf("New User %d\n", nId);
	userGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);

	if (kitchenMode) return;

	lo_send(addr, "/new_user","i",(int)nId);
}



// Callback: An existing user was lost
void XN_CALLBACK_TYPE lost_user(xn::UserGenerator& generator, XnUserID nId, void* pCookie) {
	printf("Lost user %d\n", nId);

	if (kitchenMode) return;

	lo_send(addr, "/lost_user","i",(int)nId);
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

		lo_send(addr, "/new_skel","i",(int)nId);
	}
	else {
		printf("Calibration failed for user %d\n", nId);
		userGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}
}

int jointPos(XnUserID player, XnSkeletonJoint eJoint) {

	XnSkeletonJointTransformation jointTrans;

	userGenerator.GetSkeletonCap().GetSkeletonJoint(player, eJoint, jointTrans);

	posConfidence = jointTrans.position.fConfidence;

	userID = player;

	if (!raw)
	{
	  jointCoords[0] = off_x + (mult_x * (1280 - jointTrans.position.position.X) / 2560); //Normalize coords to 0..1 interval
	  jointCoords[1] = off_y + (mult_y * (960 - jointTrans.position.position.Y) / 1920); //Normalize coords to 0..1 interval
	  jointCoords[2] = off_z + (mult_z * jointTrans.position.position.Z * 7.8125 / 10000); //Normalize coords to 0..7.8125 interval
	}
	else
	{
	  jointCoords[0] = jointTrans.position.position.X;
	  jointCoords[1] = jointTrans.position.position.Y;
	  jointCoords[2] = jointTrans.position.position.Z;
	}

	if (sendOrient)
	{
	  orientConfidence = jointTrans.orientation.fConfidence;

	  for (int i=0; i<9; i++)
	  {
	    jointOrients[i] = jointTrans.orientation.orientation.elements[i];
	  }
	}

	return 0;
}

// Generate OSC message with default format
void genOscMsg(lo_bundle *bundle, char *name) {

	if (handMode || posConfidence >= 0.5f)
	{
      lo_message msg = lo_message_new();

      lo_message_add_string(msg, name);

      if (!kitchenMode)
        lo_message_add_int32(msg, userID);

	  for (int i = 0; i < nDimensions; i++)
        lo_message_add_float(msg, jointCoords[i]);

	  lo_bundle_add_message(*bundle, "/joint", msg);
	}

	if (!kitchenMode && sendOrient && orientConfidence  >= 0.5f)
	{
	  lo_message msg = lo_message_new();

	  lo_message_add_string(msg, name);

	  if (!kitchenMode)
	    lo_message_add_int32(msg, userID);

	  // x data is in first column
	  lo_message_add_float(msg, jointOrients[0]);
	  lo_message_add_float(msg, jointOrients[0+3]);
	  lo_message_add_float(msg, jointOrients[0+6]);

	  // y data is in 2nd column
	  lo_message_add_float(msg, jointOrients[1]);
	  lo_message_add_float(msg, jointOrients[1+3]);
	  lo_message_add_float(msg, jointOrients[1+6]);

	  // z data is in 3rd column
	  lo_message_add_float(msg, jointOrients[2]);
	  lo_message_add_float(msg, jointOrients[2+3]);
	  lo_message_add_float(msg, jointOrients[2+6]);

	  lo_bundle_add_message(*bundle, "/orient", msg);
	}
}

// Generate OSC message with Quartz Composer format - based on Steve Elbows's code ;)
void genQCMsg(lo_bundle *bundle, char *name) {

	if (handMode || posConfidence >= 0.5f)
	{
	  sprintf(tmp, "/joint/%s/%d", name, userID);

      lo_message msg = lo_message_new();

	  for (int i = 0; i < nDimensions; i++)
		  lo_message_add_float(msg, jointCoords[i]);

	  lo_bundle_add_message(*bundle, tmp, msg);
	}

	if (sendOrient && orientConfidence  >= 0.5f)
	{
	  sprintf(tmp, "/orient/%s/%d", name, userID);

	  lo_message msg = lo_message_new();

	  // x data is in first column
	  lo_message_add_float(msg, jointOrients[0]);
	  lo_message_add_float(msg, jointOrients[0+3]);
	  lo_message_add_float(msg, jointOrients[0+6]);

	  // y data is in 2nd column
	  lo_message_add_float(msg, jointOrients[1]);
	  lo_message_add_float(msg, jointOrients[1+3]);
	  lo_message_add_float(msg, jointOrients[1+6]);

	  // z data is in 3rd column
	  lo_message_add_float(msg, jointOrients[2]);
	  lo_message_add_float(msg, jointOrients[2+3]);
	  lo_message_add_float(msg, jointOrients[2+6]);

	  lo_bundle_add_message(*bundle, tmp, msg);
	}
}

void sendUserPosMsg(XnUserID id) {
	XnPoint3D com;
	sprintf(tmp, "/user/%d", id);
	lo_bundle bundle = lo_bundle_new(LO_TT_IMMEDIATE);
	lo_message msg = lo_message_new();

	userGenerator.GetCoM(id, com);

	if (!raw)
	{
		lo_message_add_float(msg, (float)(off_x + (mult_x * (1280 - com.X) / 2560)));
		lo_message_add_float(msg, (float)(off_y + (mult_y * (1280 - com.Y) / 2560)));
		lo_message_add_float(msg, (float)(off_z + (mult_z * com.Z * 7.8125 / 10000)));
	}
	else
	{
		lo_message_add_float(msg,com.X);
		lo_message_add_float(msg,com.Y);
		lo_message_add_float(msg,com.Z);
	}

	lo_bundle_add_message(bundle, tmp, msg);
	lo_send_bundle(addr, bundle);
}

void sendHandOSC() {
	if (!haveHand)
		return;

	lo_bundle bundle = lo_bundle_new(LO_TT_IMMEDIATE);

	if (!raw) {
	    jointCoords[0] = off_x + (mult_x * (480 - handCoords[0]) / 960); //Normalize coords to 0..1 interval
	    jointCoords[1] = off_y + (mult_y * (320 - handCoords[1]) / 640); //Normalize coords to 0..1 interval
	    jointCoords[2] = off_z + (mult_z * handCoords[2] * 7.8125 / 10000); //Normalize coords to 0..7.8125 interval
	} else {
	    jointCoords[0] = handCoords[0];
	    jointCoords[1] = handCoords[1];
	    jointCoords[2] = handCoords[2];
	}
	oscFunc(&bundle, "l_hand");
	lo_send_bundle(addr, bundle);

	printf("hand %.3f %.3f     \r", jointCoords[0], jointCoords[1]);
	haveHand = false;
}

void sendOSC() {
	if (handMode) {
		sendHandOSC();
		return;
	}
	XnUserID aUsers[15];
	XnUInt16 nUsers = 15;
	userGenerator.GetUsers(aUsers, nUsers);
	for (int i = 0; i < nUsers; ++i) {
		if (userGenerator.GetSkeletonCap().IsTracking(aUsers[i])) {
			lo_bundle bundle = lo_bundle_new(LO_TT_IMMEDIATE);

			if (jointPos(aUsers[i], XN_SKEL_HEAD) == 0) {
				oscFunc(&bundle, "head");
			}
			if (jointPos(aUsers[i], XN_SKEL_NECK) == 0) {
				oscFunc(&bundle, "neck");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_COLLAR) == 0) {
				oscFunc(&bundle, "l_collar");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_SHOULDER) == 0) {
				oscFunc(&bundle, "l_shoulder");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_ELBOW) == 0) {
				oscFunc(&bundle, "l_elbow");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_WRIST) == 0) {
				oscFunc(&bundle, "l_wrist");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_HAND) == 0) {
				oscFunc(&bundle, "l_hand");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_FINGERTIP) == 0) {
				oscFunc(&bundle, "l_fingertip");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_COLLAR) == 0) {
				oscFunc(&bundle, "r_collar");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_SHOULDER) == 0) {
				oscFunc(&bundle, "r_shoulder");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_ELBOW) == 0) {
				oscFunc(&bundle, "r_elbow");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_WRIST) == 0) {
				oscFunc(&bundle, "r_wrist");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_HAND) == 0) {
				oscFunc(&bundle, "r_hand");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_FINGERTIP) == 0) {
				oscFunc(&bundle, "r_fingertip");
			}
			if (jointPos(aUsers[i], XN_SKEL_TORSO) == 0) {
				oscFunc(&bundle, "torso");
			}
			if (jointPos(aUsers[i], XN_SKEL_WAIST) == 0) {
				oscFunc(&bundle, "waist");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_HIP) == 0) {
				oscFunc(&bundle, "l_hip");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_KNEE) == 0) {
				oscFunc(&bundle, "l_knee");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_ANKLE) == 0) {
				oscFunc(&bundle, "l_ankle");
			}
			if (jointPos(aUsers[i], XN_SKEL_LEFT_FOOT) == 0) {
				oscFunc(&bundle, "l_foot");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_HIP) == 0) {
				oscFunc(&bundle, "r_hip");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_KNEE) == 0) {
				oscFunc(&bundle, "r_knee");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_ANKLE) == 0) {
				oscFunc(&bundle, "r_ankle");
			}
			if (jointPos(aUsers[i], XN_SKEL_RIGHT_FOOT) == 0) {
				oscFunc(&bundle, "r_foot");
			}

			lo_send_bundle(addr, bundle);
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
  -n\t\t Enable hand tracking mode\n\
  -q\t\t Enable Quartz Composer OSC format.\n\
  -s <file>\t Save to file (only .oni supported at the moment).\n\
  -i <file>\t Play from file (only .oni supported at the moment).\n\
  -xr\t\tOutput raw kinect data\n\
  -xt\t\tOutput joint orientation data\n\
  -xd\t\tTurn on puppet defaults: -xr -xt -q -w -r\n\
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
	lo_address_free(addr);
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
	printf("Initializing...\n");
	unsigned int arg = 1,
				 require_argument = 0,
				 port_argument = 0;
	XnMapOutputMode mapMode;
	XnStatus nRetVal = XN_STATUS_OK;
	XnCallbackHandle hUserCallbacks, hCalibrationCallbacks, hPoseCallbacks, hHandsCallbacks, hGestureCallbacks;
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
			port_argument = arg+1;
			PORT = argv[arg+1];
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
		case 'n':
			handMode = true;
			printf("Switching to hand mode\n");
			break;
		case 'q': // Set Quartz Composer mode
			oscFunc = &genQCMsg;
			break;
		case 'r':
			mirrorMode = false;
			break;
        case 'x': //Set multipliers
			switch(argv[arg][2]) {
			case 'r': // turn on raw mode
				raw = true;
				break;
            case 't': // send joint orientations
				sendOrient = true;
				break;
			case 'd': // turn on default options
				raw = true;
				preview = true;
				sendOrient = true;
				mirrorMode = false;
				oscFunc = &genQCMsg;
				break;
			default:
				printf("Bad option given.\n");
				usage(argv[0]);
			}
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

	if (handMode) {
		nRetVal = handsGenerator.Create(context);
		nRetVal = gestureGenerator.Create(context);
		nRetVal = gestureGenerator.RegisterGestureCallbacks(Gesture_Recognized, Gesture_Process, NULL, hGestureCallbacks);
		nRetVal = handsGenerator.RegisterHandCallbacks(new_hand, update_hand, lost_hand, NULL, hHandsCallbacks);
		if (filter)
			handsGenerator.SetSmoothing(0.2);
	}
	else {
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
	}

	xnSetMirror(depth, !mirrorMode);

	addr = lo_address_new(ADDRESS, PORT);
	signal(SIGTERM, terminate);
	signal(SIGINT, terminate);

	printf("Configured to send OSC messages to %s:%s\n", ADDRESS, PORT);
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

	if (handMode) {
		nRetVal = gestureGenerator.AddGesture(GESTURE_TO_USE, NULL);
	}
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

