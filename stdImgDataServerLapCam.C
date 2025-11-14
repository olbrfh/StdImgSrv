/*
    This program is an implementation of a standard
    image data server which captures image data from
    cam port -1 (usually the webcam on your laptop.

    Copyright (C) 2018  Martin Huelse
    http://aml.somebodyelse.de


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
*/


/**
 *
 * \file stdImgDataServerLapCam.C
 *
 * \brief Captures images from device -1 (usually the webcam on your laptop)
 * and provides these data as standard image datat server.
 *
 */


#include <iostream>           // For cerr and cout
#include <cstdlib>            // For atoi()
#include <iostream>
#include <math.h>
#include <pthread.h>


//opencv
#include <opencv2/opencv.hpp>

using namespace cv;

// communication
#include "Socket.H"  // For Socket, ServerSocket, and SocketException
#include "StdImgDataServerProtocol.H"  // For Socket, ServerSocket, and SocketException





// On Linux, you must compile with the -D_REENTRANT option.  This tells
// the C/C++ libraries that the functions must be thread-safe
#ifndef _REENTRANT
#error ACK! You need to compile with _REENTRANT defined since this uses threads
#endif



using namespace std;




unsigned short SERVER_PORT_;
int CAMERA_COLOR_;
int WINDOW_WIDTH_;
int WINDOW_HEIGHT_;


char *imageData_;
int imageDataSize_;


// Set this to true to stop the threads
// - The volatile keyword tells the compiler that this value may change at any
//   time, since another thread may write to it, and that it should not be
//   included in any optimizations.
volatile bool bStop_ = true;
volatile bool accessBlocked_ = false;
volatile bool requestForRead_ = false;
void *runServer(void * genericPtr);

//server
unsigned int RCVBUFSIZE;    // Size of receive buffer
TCPServerSocket *server_;
void HandleTCPClient(TCPSocket *sock);
void initServer();

// random value generation
unsigned char randomByte();

// just some interactive text outputs
void printInfo(int argc, char *argv[]);
void printCompleteLicense(int argc, char* args[]);
void printLicense(int argc, char* args[]);


void checkCam();


/**
 *
 * @param argc number of command line parameter
 * @param *argv[] list of parameters
 */
int main(int argc, char *argv[]){

	// check command line and read parameters
	printInfo(argc, argv);
	try{
	  SERVER_PORT_   = (unsigned short) atoi(argv[1]);
	}catch(...){
		cerr << "Can't read all parameter values, terminate programm.\n\n";
		exit(0);
	};


	//CvCapture  *capture   = cvCaptureFromCAM(-1);  // internal webcam
	CvCapture  *capture   = cvCaptureFromCAM(-1);
	IplImage       *rgb   = cvQueryFrame( capture );

	WINDOW_HEIGHT_ = rgb->height;
	WINDOW_WIDTH_  = rgb->width;
	CAMERA_COLOR_  = 1;


	if((WINDOW_WIDTH_ < 1) || (WINDOW_HEIGHT_ < 1)){
		cerr << "Image dimebsions zero of negative, terminate process.\n";
		exit(0);
	};


  	// init communication
    initServer();


    // organize the thread for dealing with request from clients
    pthread_t serverID;
    pthread_create(&serverID,NULL,runServer,NULL);
    bStop_ = false; // start threads

    // allocate memory for the image data
  	if (CAMERA_COLOR_ == 0){
  		imageDataSize_ = WINDOW_WIDTH_ * WINDOW_HEIGHT_;
  	}else{
  		imageDataSize_ = WINDOW_WIDTH_ * WINDOW_HEIGHT_*3;
  	};
  	imageData_ = new char[imageDataSize_+1];
  	cout << "Number of bytes per image : " << imageDataSize_ << endl;
  	cout << "RES: " << WINDOW_WIDTH_ << " x " << WINDOW_HEIGHT_ << endl;

    unsigned char *ptrD;


    // run the processes
    for(;;){
    	if(!requestForRead_){
    		accessBlocked_ = true;

			rgb = cvQueryFrame( capture );
    		if(rgb){
    			for(int i = 0; i < WINDOW_HEIGHT_; i++){
    				for(int j = 0; j < WINDOW_WIDTH_; j++){
    					imageData_[3*((WINDOW_WIDTH_*i) + j) + 2] =
    							((uchar *)(rgb->imageData + i*rgb->widthStep))[j*rgb->nChannels + 0]; // B
    					imageData_[3*((WINDOW_WIDTH_*i) + j) + 1] =
    							((uchar *)(rgb->imageData + i*rgb->widthStep))[j*rgb->nChannels + 1]; // G
    					imageData_[3*((WINDOW_WIDTH_*i) + j) + 0] =
    							((uchar *)(rgb->imageData + i*rgb->widthStep))[j*rgb->nChannels + 2]; // R
    				};
    			};
    		};
    		accessBlocked_ = false;
    	};
    };




};



void initServer(){
	// communication
	try {
		server_ = new TCPServerSocket(SERVER_PORT_); // Server Socket object
	}catch (SocketException &e) {
		cerr << e.what() << endl;
		exit(1);
	};
};


void HandleTCPClient(TCPSocket *sock){
  cout << "Handling central unit\n";
  try {
    cout << sock->getForeignAddress() << ":";
  } catch (SocketException &e) {
    cerr << "Unable to get foreign address" << endl;
  }
  try {
    cout << sock->getForeignPort();
  } catch (SocketException &e) {
    cerr << "Unable to get foreign port" << endl;
  }
  cout << endl;
  // Send received string and receive again until the end of transmission
  int revBUFFER_SIZE = 100;

  char revBuffer[revBUFFER_SIZE];
  int recvMsgSize;

  char echoMetaData[124];
  char echoUnknownCommand[1024];

  do{
    try{
    	recvMsgSize = sock->recv(revBuffer, revBUFFER_SIZE);
    	if(recvMsgSize < 1) break;
    }catch(...){
    	break;
    };


    if(!(strncmp(GET_META_DATA,revBuffer,strlen(GET_META_DATA)))){
    	// send meta data
    	echoMetaData[0]='\0';
    	sprintf(echoMetaData,"[W=%d,H=%d,O=%c,C=%d,X=%c%c%c,B=%d,BTS=%d]%c",
    			WINDOW_WIDTH_,WINDOW_HEIGHT_,'W',CAMERA_COLOR_,'R','G','B',imageDataSize_,0,'\0');
    	sock->send(echoMetaData,strlen(echoMetaData));
    }else if(!(strncmp("GET_IMAGE_DATA",revBuffer,strlen(GET_IMAGE_DATA)))){
    	// send image data
		do{
			if(!accessBlocked_){
				sock->send(imageData_, imageDataSize_);
				requestForRead_ = false;
				break;
			}else{
				requestForRead_ = true;
				cout << "access blocked continue request.\n";
			};
		}while(true);
    }else if(!(strncmp(GET_VERSION,revBuffer,strlen(GET_VERSION)))){
    	echoMetaData[0]='\0';
    	sprintf(echoMetaData,"%s%c",CURRENT_VERSION,'\0');
    	sock->send(echoMetaData,strlen(echoMetaData));
    }else{
    	// send protocol
    	echoUnknownCommand[0]='\0';
    	sprintf(echoUnknownCommand,"%s please try:\n %s\n %s\n %s\n%c",UNKNOWN_COMMAND, GET_VERSION, GET_META_DATA, GET_IMAGE_DATA,'\0');
		sock->send(echoUnknownCommand, strlen(echoUnknownCommand));
    };
  }while(true);

  delete sock;

};


void *runServer(void * genericPtr){

	//cout << "+WAIT"; cout.flush();
	while(bStop_){;};  // wait
	pthread_yield();

	//cout << "YES left \n"; cout.flush();
	int i = 0;
	try{
		do{   // Run forever
			HandleTCPClient(server_->accept());       // Wait for a client to connect
	    }while(!bStop_);
	}catch (SocketException &e) {
	    cerr << e.what() << endl;
	};
	// Relenquish the CPU for another thread
	pthread_yield();

	delete server_;
	bStop_ = true;
	return NULL;
};

unsigned char randomByte(){
	unsigned int i = ((unsigned int) 255*((float)rand()) / ((float) RAND_MAX));
	return ((unsigned char) i);
};


void printInfo(int argc, char *argv[]){
		  if (argc == 2){
			  return;
		  }else if (argc == 3){
			  printCompleteLicense(argc,argv);
		  }else{     // Test for correct number of arguments
		    cerr << "Usage of " << argv[0] << " : \n\n"
		         << argv[0] << " <port> " << endl;
		    cerr << "\n"
		    	 << "<port>          port number of this server\n";
		    printLicense(argc,argv);
		  };
};

void printLicense(int argc, char* args[]){
	char c;
	cout << endl;
	cout << "" << "Copyright (C) 2008  Martin Huelse msh@aber.ac.uk \n";
    cout << "" << "This program comes with ABSOLUTELY NO WARRANTY.\n";
    cout << "" << "This is free software, and you are welcome to redistribute it\n";
    cout << "" << "under certain conditions; type: \n";
    cout << "" << "'" << args[0] << " show c' for details.\n\n";
    exit(0);
};



void printCompleteLicense(int argc, char* args[]){
	cout << endl;
	cout << "\t" << "Copyright (C) 2008  Martin Huelse msh@aber.ac.uk \n\n";
	cout << "\t"<< "This program is free software: you can redistribute it and/or modify \n";
	cout << "\t"<< "it under the terms of the GNU General Public License as published by \n";
	cout << "\t"<< "the Free Software Foundation, either version 3 of the License, or \n";
	cout << "\t"<< "(at your option) any later version. \n\n";

	cout << "\t"<< "This program is distributed in the hope that it will be useful, \n";
	cout << "\t"<< "but WITHOUT ANY WARRANTY; without even the implied warranty of \n";
	cout << "\t"<< "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \n";
	cout << "\t"<< "GNU General Public License for more details. \n\n";

	cout << "\t"<< "You might have received a copy of the GNU General Public License \n";
	cout << "\t"<< "along with this program.  If not, see <http://www.gnu.org/licenses/>. \n\n";

	exit(0);
};


void checkCam(){
	cvNamedWindow("LapCam", CV_WINDOW_AUTOSIZE );
	CvCapture  *capture   = cvCaptureFromCAM(-1);
	IplImage       *rgb   = cvQueryFrame( capture );

	std::cout << "(V,H) = (" << rgb->width << "," << rgb->height << ") color: "<< rgb->colorModel << "\n";

	while(true){
		rgb = cvQueryFrame( capture );
		if( rgb==0 ) break;
		cvShowImage( "LapCam", rgb );
		cv::waitKey(5); //Wait of 100 ms
	}
	return;
}
