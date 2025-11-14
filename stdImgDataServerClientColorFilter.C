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
 * \file stdImgDataserverClientColorFilter.C
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

const int CAMERA_COLOR_ = 0;


void *runServer(void * genericPtr);
void HandleTCPClient(TCPSocket *sock);



unsigned char *rawImageData_;
unsigned char *sumFilterData_;
int rawImageDataSize_;
int imageWidth_;
int imageHeight_;

// view and filters
char* winNameRawRGB_;
IplImage *openCvImageRawRGB_;

char* winNameSumFilter_;
IplImage *openCvImageSumFilter_;

char* winNameBfilter_;
IplImage *openCvImageBfilter_;

char* winNameRfilter_;
IplImage *openCvImageRfilter_;

char* winNameGfilter_;
IplImage *openCvImageGfilter_;

int rFilterThreshR_ = 162;
int rFilterThreshG_ = 143;
int rFilterThreshB_ = 255;
int gFilterThreshR_ = 126;
int gFilterThreshG_ = 140;
int gFilterThreshB_ = 209;
int bFilterThreshR_ = 87;
int bFilterThreshG_ = 255;
int bFilterThreshB_ = 149;

int sumPartB_ = 0;
int sumPartG_ = 0;
int sumPartR_ = 0;
int sumAdd_   = 0;



void printInfo(int argc, char *argv[]);
int  sizeRawImageData(TCPSocket *socket, int *s, int *w, int *h);
bool updateImageData(TCPSocket *socket,unsigned char *storageImageData, int size);
void updateRawImageView(IplImage *openCvImageRaw, unsigned char *imgD);

void createColorFilterWin(char color, char* winName,IplImage *openCvImg);
void updateFilters(IplImage *openCvImgR,IplImage *openCvImgG,IplImage *openCvImgB,IplImage *openCvImgSum);

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
	sizeRawImageData(dataSource_, &rawImageDataSize_, &imageWidth_,&imageHeight_);
	if(rawImageDataSize_ < 1){
		cout << "No data available, check image data server.\n";
		exit(1);
	};
	printf("Receive image data %i x %i  total bytes %i \n", imageWidth_, imageHeight_, rawImageDataSize_);

	rawImageData_ = new unsigned char[rawImageDataSize_];
	sumFilterData_ = new unsigned char[imageWidth_*imageHeight_]; // no RGB, just grey values

	//view
	winNameRfilter_ = new char[16]; sprintf(winNameRfilter_,"%c filter",'R');
	openCvImageRfilter_ = cvCreateImage(cvSize(imageWidth_,imageHeight_),IPL_DEPTH_8U,1);
	createColorFilterWin('R',winNameRfilter_,openCvImageRfilter_);

	winNameGfilter_ = new char[16]; sprintf(winNameGfilter_,"%c filter",'G');
	openCvImageGfilter_ = cvCreateImage(cvSize(imageWidth_,imageHeight_),IPL_DEPTH_8U,1);
	createColorFilterWin('G',winNameGfilter_,openCvImageGfilter_);

	winNameBfilter_ = new char[16]; sprintf(winNameBfilter_,"%c filter",'B');
	openCvImageBfilter_ = cvCreateImage(cvSize(imageWidth_,imageHeight_),IPL_DEPTH_8U,1);
	createColorFilterWin('B',winNameBfilter_,openCvImageBfilter_);

	winNameSumFilter_ = new char[16]; sprintf(winNameSumFilter_,"%cummed filter outputs",'S');
	openCvImageSumFilter_ = cvCreateImage(cvSize(imageWidth_,imageHeight_),IPL_DEPTH_8U,1);
	createColorFilterWin('S',winNameSumFilter_,openCvImageSumFilter_);


	openCvImageRawRGB_ = cvCreateImage(cvSize(imageWidth_,imageHeight_),IPL_DEPTH_8U,3);
	winNameRawRGB_ = new char[32];
	strcpy(winNameRawRGB_, "Received Image");
	cvNamedWindow(winNameRawRGB_, 0);



    // organize threads
    pthread_t serverID;
    pthread_create(&serverID,NULL,runServer,NULL);
    bStop_ = false; // start threads



	int i=0;
	while(updateImageData(dataSource_,rawImageData_,rawImageDataSize_)){

		updateRawImageView(openCvImageRawRGB_,rawImageData_);
		accessBlocked_ = true;
		updateFilters(openCvImageRfilter_,openCvImageGfilter_,openCvImageBfilter_,openCvImageSumFilter_);
		accessBlocked_ = false;
		cvShowImage(winNameRawRGB_,openCvImageRawRGB_);
		cvShowImage(winNameRfilter_,openCvImageRfilter_);
		cvShowImage(winNameGfilter_,openCvImageGfilter_);
		cvShowImage(winNameBfilter_,openCvImageBfilter_);
		cvShowImage(winNameSumFilter_,openCvImageSumFilter_);
		cvWaitKey(2);
	};

	pthread_yield();
	bStop_ = true;
	pthread_join(serverID, NULL);


	delete dataSource_;
	delete [] rawImageData_;
	delete [] sumFilterData_;
	exit(0);
};

void updateRawImageView(IplImage *openCvImageRaw, unsigned char *imgD){
	for(int i = 0; i < imageHeight_; i++){
		for(int j = 0; j < imageWidth_; j++){
				((uchar *)(openCvImageRaw->imageData + i*openCvImageRaw->widthStep))[j*openCvImageRaw->nChannels + 0] =
					imgD[3*((imageWidth_*i) + j) + 2]; // B
				((uchar *)(openCvImageRaw->imageData + i*openCvImageRaw->widthStep))[j*openCvImageRaw->nChannels + 1] =
					imgD[3*((imageWidth_*i) + j) + 1]; // G
				((uchar *)(openCvImageRaw->imageData + i*openCvImageRaw->widthStep))[j*openCvImageRaw->nChannels + 2] =
					imgD[3*((imageWidth_*i) + j) + 0]; // R
		};
	};

}
void updateFilters(IplImage *openCvImgR,IplImage *openCvImgG,IplImage *openCvImgB,IplImage *openCvImgSum){
	unsigned int valueR, valueB, valueG, sum;
	unsigned int bFilteredValue, rFilteredValue, gFilteredValue;
	float relSumPartB, relSumPartG, relSumPartR;

	int i;
	for(int w = 0; w < imageWidth_; w++){
		for(int h = 0; h < imageHeight_; h++){

			i = ((imageWidth_*h) + w);
			valueR = (unsigned int) rawImageData_[3*i];
			valueG = (unsigned int) rawImageData_[3*i+1];
			valueB = (unsigned int) rawImageData_[3*i+2];

			if( ( valueR > rFilterThreshR_ ) &&
					( valueG < rFilterThreshG_ ) &&
					( valueB < rFilterThreshB_ )    ){
				rFilteredValue = valueR ;
			}else{
				rFilteredValue = 0;
			};
			if( ( valueG > gFilterThreshG_ ) &&
					( valueR < gFilterThreshR_ ) &&
					( valueB < gFilterThreshB_ )    ){
				gFilteredValue = valueG ;
			}else{
				gFilteredValue = 0;
			};
			if( ( valueB > bFilterThreshB_ ) &&
					( valueG < bFilterThreshG_ ) &&
					( valueR < bFilterThreshR_ )    ){
				bFilteredValue = valueB ;
			}else{
				bFilteredValue = 0;
			};

			((uchar *)(openCvImgR->imageData + h*openCvImgR->widthStep))[w] = rFilteredValue;
			((uchar *)(openCvImgG->imageData + h*openCvImgG->widthStep))[w] = gFilteredValue;
			((uchar *)(openCvImgB->imageData + h*openCvImgB->widthStep))[w] = bFilteredValue;

			relSumPartB = (((float) sumPartB_)/100.0 )*((float) bFilteredValue);
			relSumPartG = (((float) sumPartG_)/100.0 )*((float) gFilteredValue);
			relSumPartR = (((float) sumPartR_)/100.0 )*((float) rFilteredValue);
			sum = (unsigned int) (relSumPartB + relSumPartG + relSumPartR + sumAdd_);
			if(sum > 255) sum = 255;

			((uchar *)(openCvImgSum->imageData + h*openCvImgSum->widthStep))[w] = sum;
			sumFilterData_[i] = (unsigned char) sum;
		};
	};
};

void createColorFilterWin(char color, char* winName,IplImage *openCvImg){
	cvNamedWindow(winName, 0);
	cvShowImage(winName,openCvImg);
	if(color == 'R'){
		cvCreateTrackbar("R >",winName, &rFilterThreshR_, 255, NULL );
		cvCreateTrackbar("G <",winName, &rFilterThreshG_, 255, NULL );
		cvCreateTrackbar("B <",winName, &rFilterThreshB_, 255, NULL );
	}else if(color == 'G'){
		cvCreateTrackbar("G >",winName, &gFilterThreshG_, 255, NULL );
		cvCreateTrackbar("R <",winName, &gFilterThreshR_, 255, NULL );
		cvCreateTrackbar("B <",winName, &gFilterThreshB_, 255, NULL );
	}else if(color == 'B'){
		cvCreateTrackbar("B >",winName, &bFilterThreshB_, 255, NULL );
		cvCreateTrackbar("R <",winName, &bFilterThreshR_, 255, NULL );
		cvCreateTrackbar("G <",winName, &bFilterThreshG_, 255, NULL );
	}else{
		cvCreateTrackbar("rel. R-part" ,winName, &sumPartR_, 100, NULL );
		cvCreateTrackbar("rel. G-part" ,winName, &sumPartG_, 100, NULL );
		cvCreateTrackbar("rel. B-part" ,winName, &sumPartB_, 100, NULL );
		cvCreateTrackbar("abs. Off-set",winName, &sumAdd_,   255, NULL );
	};
};

int  sizeRawImageData(TCPSocket *socket, int *s, int *w, int *h){
	int rcvBufferSize = 124;
	char echoBuffer[rcvBufferSize];
	int bytesReceived = 0;
	socket->send("GET_META_DATA",strlen("GET_META_DATA"));
	if( (bytesReceived = socket->recv(echoBuffer,rcvBufferSize)) <= 0){
		return (-1);
	};

	// interprete received data
	char ch1,ch2,ch3,imageOrg;
	int color, recvImageDataSize, nmbBytesTimeStamp;
	int nmbTokens = sscanf(echoBuffer,"[W=%d,H=%d,O=%c,C=%d,X=%c%c%c,B=%d,BTS=%d]",
			w,h,&imageOrg,&color,&ch1,&ch2,&ch3,&recvImageDataSize,&nmbBytesTimeStamp);
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
    			imageWidth_,imageHeight_,'W',CAMERA_COLOR_,'R','G','B',imageWidth_*imageHeight_,0,'\0');
    	sock->send(echoMetaData,strlen(echoMetaData));
    }else if(!(strncmp("GET_IMAGE_DATA",revBuffer,strlen(GET_IMAGE_DATA)))){
    	// send image data
		do{
			if(!accessBlocked_){
				sock->send(sumFilterData_,(imageWidth_*imageHeight_));
				break;
			}else{
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






