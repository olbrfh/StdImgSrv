/*
    This program is an implementation of a standard
    image data server which takes date from another
    standard image data server (client) and provides
    filtered data as standard image data server.

    Copyright (C) 2009  Martin Huelse
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
 * \file stdImgDataserverClientBlobDetector.C
 *
 * \brief Captures images from device -1 (usually the webcam on your laptop)
 * and provides these data as standard image data server.
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


// Set this to true to stop the threads
// - The volatile keyword tells the compiler that this value may change at any
//   time, since another thread may write to it, and that it should not be
//   included in any optimizations.
volatile bool bStop_ = true;
volatile bool accessBlocked_ = false;

TCPServerSocket *thisServer_;
TCPSocket *dataSource_ = NULL;

unsigned short THIS_SERVER_PORT_;
unsigned short SOURCE_SERVER_PORT_;
char          *SOURCE_SERVER_ADR_;

const int IMAGE_COLOR_ = 0;


void *runServer(void * genericPtr);
void HandleTCPClient(TCPSocket *sock);



unsigned char *rawImageData_;
unsigned char *monitorData_;
int rawImageDataSize_;
int imageWidth_;
int imageHeight_;
int colorValue_;
unsigned char *blobCoord_;
int blobCoordSize_ = 4;

int detectTrash_;


// image data raw data received (grey valued)
IplImage *openCvImageRawGrey_;

// view
char* winNameMonitor_;
IplImage *openCvImageMinitor_;




void printInfo(int argc, char *argv[]);
int  sizeRawImageData(TCPSocket *socket, int *s, int *w, int *h, int *color);
bool updateImageData(TCPSocket *socket,unsigned char *storageImageData, int size);

void createMonitorWin(char* winName,IplImage *openCvImg);
void updateMonitor(IplImage *openCvImgMonitor,  unsigned char *imgD);
//void updateMonitor(IplImage *openCvImgRawData, IplImage *openCvImgMonitor);
void updateRawImageView(IplImage *openCvImageRaw, unsigned char *imgD);


/**
 *
 * @param argc number of command line parameter
 * @param *argv[] list of parameters
 */
int main(int argc, char *argv[]){
	printInfo(argc,argv);

	// communication

	// server for sending out the filter results
	try {
		thisServer_ = new TCPServerSocket(THIS_SERVER_PORT_); // Server Socket object
	}catch (SocketException &e) {
		cerr << e.what() << endl;
		exit(1);
	};

	// client for getting the rgb data
	try{
		dataSource_ = new TCPSocket(SOURCE_SERVER_ADR_, SOURCE_SERVER_PORT_);
	}catch(...){
		cout << "No server for image data.\n" << SOURCE_SERVER_ADR_ << endl << endl;
		exit(1);
	};
	sizeRawImageData(dataSource_, &rawImageDataSize_, &imageWidth_,&imageHeight_,&colorValue_);
	if(rawImageDataSize_ < 1){
		cout << "No data available, check image data server.\n";
		exit(1);
	};
	printf("Receive image data %i x %i  total bytes %i \n", imageWidth_, imageHeight_, rawImageDataSize_);

	if(colorValue_ > 0){
		cout << "Image data must be grey value data.\n";
		exit(1);
	};

	rawImageData_ = new unsigned char[rawImageDataSize_];
	monitorData_  = new unsigned char[imageWidth_*imageHeight_]; // no RGB, just grey values
	blobCoord_    = new unsigned char[blobCoordSize_];

	//view
	winNameMonitor_ = new char[16]; sprintf(winNameMonitor_,"Blob Detector");
	openCvImageMinitor_ = cvCreateImage(cvSize(imageWidth_,imageHeight_),IPL_DEPTH_8U,3);
	createMonitorWin(winNameMonitor_,openCvImageMinitor_);
	cvCreateTrackbar("thrash value" ,winNameMonitor_, &detectTrash_, 255, NULL );

	openCvImageRawGrey_ = cvCreateImage(cvSize(imageWidth_,imageHeight_),IPL_DEPTH_8U,1);



    // organize threads
    pthread_t serverID;
    pthread_create(&serverID,NULL,runServer,NULL);
    bStop_ = false; // start threads



	int i=0;
	while(updateImageData(dataSource_,rawImageData_,rawImageDataSize_)){
		//updateRawImageView(openCvImageRawGrey_,rawImageData_);
		accessBlocked_ = true;
		updateMonitor(openCvImageMinitor_,rawImageData_);
		accessBlocked_ = false;
		cvShowImage(winNameMonitor_,openCvImageMinitor_);
		cvWaitKey(2);
	};

	pthread_yield();
	bStop_ = true;
	pthread_join(serverID, NULL);


	delete dataSource_;
	delete [] rawImageData_;
	delete [] monitorData_;
	exit(0);
};


void updateRawImageView(IplImage *openCvImageRaw, unsigned char *imgD){
/*
	for(int i = 0; i < imageHeight_; i++){
		for(int j = 0; j < imageWidth_; j++){
			((uchar *)(openCvImageRaw->imageData + i*openCvImageRaw->widthStep))[j] =
					imgD[(imageWidth_*i) + j];
		};
	};
*/


	for(int i = 0; i < imageHeight_; i++){
		for(int j = 0; j < imageWidth_; j++){
				((uchar *)(openCvImageRaw->imageData + i*openCvImageRaw->widthStep))[j*openCvImageRaw->nChannels + 0] =
						imgD[(imageWidth_*i) + j]; // B
				((uchar *)(openCvImageRaw->imageData + i*openCvImageRaw->widthStep))[j*openCvImageRaw->nChannels + 1] =
						imgD[(imageWidth_*i) + j]; // G
				((uchar *)(openCvImageRaw->imageData + i*openCvImageRaw->widthStep))[j*openCvImageRaw->nChannels + 2] =
						imgD[(imageWidth_*i) + j]; // R
		};
	};
}

void updateMonitor(IplImage *openCvImgMonitor,  unsigned char *imgD){

	/*
	// transfer raw data into the monitor image data
	for(int i = 0; i < imageHeight_; i++){
		for(int j = 0; j < imageWidth_; j++){
			((uchar *)(openCvImgMonitor->imageData + i*openCvImgMonitor->widthStep))[j] =
					((uchar *)(openCvImgRawData->imageData + i*openCvImgRawData->widthStep))[j];
		};
	};
	*/

	// write data into image structure to display image
	for(int i = 0; i < imageHeight_; i++){
		for(int j = 0; j < imageWidth_; j++){
				((uchar *)(openCvImgMonitor->imageData + i*openCvImgMonitor->widthStep))[j*openCvImgMonitor->nChannels + 0] =
						imgD[(imageWidth_*i) + j]; // B
				((uchar *)(openCvImgMonitor->imageData + i*openCvImgMonitor->widthStep))[j*openCvImgMonitor->nChannels + 1] =
						imgD[(imageWidth_*i) + j]; // G
				((uchar *)(openCvImgMonitor->imageData + i*openCvImgMonitor->widthStep))[j*openCvImgMonitor->nChannels + 2] =
						imgD[(imageWidth_*i) + j]; // R
		};
	};


	int value, w, h, maxH, minH, maxW, minW, meanW, meanH;

	// find maximal height value
	maxH = -1;
	for(int i = 0; i < imageHeight_; i++){
		for(int j = 0; j < imageWidth_; j++){
			value = imgD[(imageWidth_*i) + j];
			//value = ((uchar *)(openCvImgMonitor->imageData + i*openCvImgMonitor->widthStep))[j];
			if(value > detectTrash_){
				cvCircle(openCvImgMonitor, cvPoint(j,i), 10, cvScalar(255,255,255), 2, 8);
				w = j;
				h = i;
				maxH = h;
				i=imageHeight_;
				j=imageWidth_;
			}
		};
	};

	// find maximal height value
	minH = -1;
	for(int i =imageHeight_-1; i > -1; i--){
		for(int j = imageWidth_; j > -1 ; j--){
			value = imgD[(imageWidth_*i) + j];
			//value = ((uchar *)(openCvImgMonitor->imageData + i*openCvImgMonitor->widthStep))[j];
			if(value > detectTrash_){
				cvCircle(openCvImgMonitor, cvPoint(j,i), 10, cvScalar(255,255,255), 2, 8);
				w = j;
				h = i;
				minH = h;
				i=-1;
				j=-1;
			}
		};
	};


	// find maximal width value
	maxW = -1;
	for(int i = 0; i < imageWidth_; i++){
		for(int j = 0; j < imageHeight_; j++){
			value = imgD[(imageWidth_*j) + i];
			//value = ((uchar *)(openCvImgMonitor->imageData + i*openCvImgMonitor->widthStep))[j];
			if(value > detectTrash_){
				cvCircle(openCvImgMonitor, cvPoint(i,j), 10, cvScalar(255,255,255), 2, 8);
				h = j;
				w = i;
				maxW = w;
				i=imageWidth_;
				j=imageHeight_;
			}
		};
	};

	// find maximal width value
	minW = -1;
	for(int i = imageWidth_; i > -1 ; i--){
		for(int j = imageHeight_; j > -1 ; j--){
			value = imgD[(imageWidth_*j) + i];
			//value = ((uchar *)(openCvImgMonitor->imageData + i*openCvImgMonitor->widthStep))[j];
			if(value > detectTrash_){
				cvCircle(openCvImgMonitor, cvPoint(i,j), 10, cvScalar(255,255,255), 2, 8);
				h = j;
				w = i;
				minW = w;
				i=-1;
				j=-1;
			}
		};
	};



	cvLine(openCvImgMonitor, cvPoint(0,maxH),cvPoint(imageWidth_,maxH), cvScalar(255,0,0), 2, 8);
	cvLine(openCvImgMonitor, cvPoint(0,minH),cvPoint(imageWidth_,minH), cvScalar(0,0,255), 2, 8);

	cvLine(openCvImgMonitor, cvPoint(maxW,0),cvPoint(maxW, imageHeight_), cvScalar(255,0,0), 2, 8);
	cvLine(openCvImgMonitor, cvPoint(minW,0),cvPoint(minW, imageHeight_), cvScalar(0,0,255), 2, 8);


	if( ((maxW < 0) && (minW < 0)) ||
		((maxH < 0) && (minH < 0))	){
		blobCoord_[0] = 0;
		blobCoord_[1] = 0;
		blobCoord_[2] = 0;
		blobCoord_[3] = 0;
	}else{
		if(maxW < minW){
			int tmp;
			tmp = maxW;
			maxW = minW;
			minW = tmp;
		}
		if(maxH < minH){
			int tmp;
			tmp = maxH;
			maxH = minH;
			minH = tmp;
		}

		meanW = ((maxW - minW) / 2) + minW;
		meanH = ((maxH - minH) / 2) + minH;


		blobCoord_[0] = ((int)0xFF)   & meanW;  // 1st 8 bits of meanW
		blobCoord_[1] = (((int)0xFF00) & meanW) >> 8 ;  // 2nd 8 bits of meanW
		blobCoord_[2] = ((int)0xFF)   & meanH;  // 1st 8 bits of meanH
		blobCoord_[3] = (((int)0xFF00) & meanH) >> 8;  // 2nd 8 bits of meanH

		cout << "width: " << meanW  << "    height: " << meanH << endl;

		int bloobCoordWidth = 0;
		int bloobCoordHight = 0;

		bloobCoordWidth = (int) (blobCoord_[1]);
		bloobCoordWidth = bloobCoordWidth << 8;
		bloobCoordWidth = bloobCoordWidth   | ((int) blobCoord_[0]);

		bloobCoordHight = (int) (blobCoord_[3]);
		bloobCoordHight = bloobCoordHight << 8;
		bloobCoordHight = bloobCoordHight   | ((int) blobCoord_[2]);

		//printf("HEX: %08x \t-->\t %08x - %08x\n", meanW, blobCoord_[0], blobCoord_[1]);


		//cout << "CALC: width: " << bloobCoordWidth  << "    height: " << bloobCoordHight << endl;

	}





	// find minimal height value


};

void createMonitorWin(char* winName,IplImage *openCvImg){
	cvNamedWindow(winName, 0);
	cvShowImage(winName,openCvImg);
};

int  sizeRawImageData(TCPSocket *socket, int *s, int *w, int *h, int *color){
	int rcvBufferSize = 124;
	char echoBuffer[rcvBufferSize];
	int bytesReceived = 0;
	socket->send("GET_META_DATA",strlen("GET_META_DATA"));
	if( (bytesReceived = socket->recv(echoBuffer,rcvBufferSize)) <= 0){
		return (-1);
	};

	// interprete received data
	char ch1,ch2,ch3,imageOrg;
	int  recvImageDataSize, nmbBytesTimeStamp;
	int nmbTokens = sscanf(echoBuffer,"[W=%d,H=%d,O=%c,C=%d,X=%c%c%c,B=%d,BTS=%d]",
			w,h,&imageOrg,color,&ch1,&ch2,&ch3,&recvImageDataSize,&nmbBytesTimeStamp);
	if(nmbTokens != 9){
		cerr << "Can't interprete image meta data, terminate process.\n";
		return (-1);
	};
	*s = recvImageDataSize + nmbBytesTimeStamp;
	return *s;
};

bool updateImageData(TCPSocket *socket,unsigned char *storageImageData, int size){
	int rcvBufferSize = size;
	int idx = 0;
	int bytesReceived = 0;
	int totalBytesReceived = 0;
	socket->send("GET_IMAGE_DATA",strlen("GET_IMAGE_DATA"));
	do{
		bytesReceived = socket->recv(&(storageImageData[idx]),rcvBufferSize);
		totalBytesReceived += bytesReceived;
		idx = totalBytesReceived;
	}while(totalBytesReceived < size);
	return true;
};


void printInfo(int argc, char *argv[]){
		  if (argc != 4){     // Test for correct number of arguments
		    cerr << "Usage: " << argv[0]
		         << " <Port of this server> <Server of Data> <Port of Server of Data>" << endl;
		    exit(1);
		  };

		  SOURCE_SERVER_PORT_ = atoi(argv[3]);
		  SOURCE_SERVER_ADR_  = argv[2];
		  THIS_SERVER_PORT_   = atoi(argv[1]);
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
    			blobCoordSize_,1,'W',0,'X','X','X',blobCoordSize_,0,'\0');
    	sock->send(echoMetaData,strlen(echoMetaData));
    }else if(!(strncmp("GET_IMAGE_DATA",revBuffer,strlen(GET_IMAGE_DATA)))){
    	// send image data
		do{
			if(!accessBlocked_){
				sock->send(blobCoord_,blobCoordSize_);
				break;
			}else{
				//cout << "access blocked continue request.\n";
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

/*

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
	  bool correct;

	  do{
	    try{
	    	recvMsgSize = sock->recv(revBuffer, revBUFFER_SIZE);
	    	if(recvMsgSize < 1) break;
	    	revBuffer[recvMsgSize]='\0';
	    }catch(...){
	    	break;
	    };

	    printf(" msg: %s \n", revBuffer);

	    if(!(strncmp("NUMBER_BYTES",revBuffer,strlen("NUMBER_BYTES")))){
	    	correct=true;
	    	// send size
	    	char echo[64];
	    	sprintf(echo,"%d=[w=%d,h=%d]",(imageWidth_*imageHeight_), imageWidth_,imageHeight_);
	    	sock->send(echo,strlen(echo));
	    }else if(!(strncmp("FILTER_DATA",revBuffer,strlen("FILTER_DATA")))){
			do{
				if(!accessBlocked_){
					sock->send(sumFilterData_,(imageWidth_*imageHeight_));
					break;
				}else{
					cout << "\t\t access blocked continue request.\n";
				};
			}while(true);
	    }else{
	    	correct=false;
			sock->send("-1001", 20);
	    };
	  }while(true);

	  delete sock;

};
*/

void *runServer(void * genericPtr){
	//cout << "+WAIT"; cout.flush();
	while(bStop_){;};  // wait
	pthread_yield();

	//cout << "YES left \n"; cout.flush();
	int i = 0;
	try{
		do{   // Run forever
			HandleTCPClient(thisServer_->accept());       // Wait for a client to connect
	    }while(!bStop_);
	}catch (SocketException &e) {
	    cerr << e.what() << endl;
	};
	// Relenquish the CPU for another thread
	pthread_yield();

	delete thisServer_;
	bStop_ = true;
	return NULL;
};






