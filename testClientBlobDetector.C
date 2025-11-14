/*
    This program is an implementation of a client connecting
    to a standard image data server getting and displaying
    the image data provided by the server.

    Copyright (C) 2009  Martin Huelse,
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


#include "Socket.H"  // For Socket, ServerSocket, and SocketException
#include <iostream>           // For cerr and cout
#include <cstdlib>            // For atoi()

#include "StdImgDataServerProtocol.H"

//opencv
#include <opencv2/opencv.hpp>

using namespace cv;

TCPSocket     *dataSource_ = NULL;
unsigned short SOURCE_SERVER_PORT_;
char          *SOURCE_SERVER_ADR_;

unsigned char *recvImageData_;
int recvImageDataSize_;
int imageWidth_;
int imageHeight_;
int color_;
char imageOrg_;
char ch1_;
char ch2_;
char ch3_;
int nmbBytesTimeStamp_;



void printInfo(int argc, char *argv[]);


bool updateImageData(TCPSocket *socket, unsigned char *storageImageData, int size);
//void updateImageView(IplImage *openCvImageRaw, char *imgD, int color);

// just some interactive text outputs
void printInfo(int argc, char *argv[]);
void printCompleteLicense(int argc, char* args[]);
void printLicense(int argc, char* args[]);



/**
 *
 * @param argc number of command line parameter
 * @param *argv[] list of parameters
 */
int main(int argc, char *argv[]){

    printf("%s\r\n", CV_VERSION);
    printf("%u.%u.%u\r\n", CV_MAJOR_VERSION, CV_MINOR_VERSION, CV_SUBMINOR_VERSION);

	printInfo(argc,argv);

	try{
		SOURCE_SERVER_ADR_  = argv[1];
		SOURCE_SERVER_PORT_ = atoi(argv[2]);
	}catch(...){
		cerr << "Can't read command line parameters, terminate process.\n";
		exit(0);
	};

	// client for image data
	try{
		dataSource_ = new TCPSocket(SOURCE_SERVER_ADR_, SOURCE_SERVER_PORT_);
	}catch(...){
		cout << "No server for image data.\n" << SOURCE_SERVER_ADR_ << endl << endl;
		exit(1);
	};


	// get meta data
	int sizeEchoBufferMetaData = 64;
	char echoBufferMetaData[sizeEchoBufferMetaData];
	int bytesReceived = 0;

	try{
		dataSource_->send(GET_VERSION,strlen(GET_VERSION));
	}catch(...){
		cerr << "Error while sending: " << GET_VERSION << endl;
		exit(0);
	};

	if( (bytesReceived = dataSource_->recv(echoBufferMetaData,sizeEchoBufferMetaData)) <= 0){
		cerr << "Can't get Standard Image Data Server Version, terminate process.\n";
		exit(0);
	};
	echoBufferMetaData[bytesReceived]='\0';
	cout << "Server version: " << echoBufferMetaData << endl;

	echoBufferMetaData[0]='\0';
	try{
		dataSource_->send(GET_META_DATA,strlen(GET_META_DATA));
	}catch(...){
		cerr << "Error while sending: " << GET_META_DATA << endl;
		exit(0);
	};

	if( (bytesReceived = dataSource_->recv(echoBufferMetaData,sizeEchoBufferMetaData)) <= 0){
		cerr << "Can't get image meta data, terminate process.\n";
		exit(0);
	};
	echoBufferMetaData[bytesReceived]='\0';
	cout << "META_DATA received: " << echoBufferMetaData << endl;



	// interpret received data
	int nmbTokens = sscanf(echoBufferMetaData,"[W=%d,H=%d,O=%c,C=%d,X=%c%c%c,B=%d,BTS=%d]",
			&imageWidth_,&imageHeight_,&imageOrg_,&color_,&ch1_,&ch2_,&ch3_,&recvImageDataSize_,&nmbBytesTimeStamp_);
	if(nmbTokens != 9){
		cerr << "Can't interpret image meta data, terminate process.\n";
		exit(0);
	};
	// allocate memory for the image data with might include some bytes
	// at the end containing the time stamp
	recvImageData_ = new unsigned char[recvImageDataSize_ + nmbBytesTimeStamp_];

	int bloobCoordWidth = 0;
	int bloobCoordHight = 0;

	while(updateImageData(dataSource_,recvImageData_,(recvImageDataSize_ + nmbBytesTimeStamp_))){
		bloobCoordWidth = 0;
		bloobCoordHight = 0;

		bloobCoordWidth = (int) (recvImageData_[1]);
		bloobCoordWidth = bloobCoordWidth << 8;
		bloobCoordWidth = bloobCoordWidth   | ((int) recvImageData_[0]);

		bloobCoordHight = (int) (recvImageData_[3]);
		bloobCoordHight = bloobCoordHight << 8;
		bloobCoordHight = bloobCoordHight   | ((int) recvImageData_[2]);


		cout << "width: " << bloobCoordWidth  << "    height: " << bloobCoordHight << endl;
	};


	delete dataSource_;
	delete [] recvImageData_;
	exit(0);
};



bool updateImageData(TCPSocket *socket, unsigned char *storageImageData, int size){
	int rcvBufferSize = size;
	int idx = 0;
	int bytesReceived = 0;
	int totalBytesReceived = 0;
	try{
		socket->send(GET_IMAGE_DATA,strlen(GET_IMAGE_DATA));
	}catch(SocketException &e){
		cout << e.what();
		return (false);
	}catch(...){
		cerr << "Error while sending: " << GET_IMAGE_DATA << endl;
		return (false);
	};

	do{
		bytesReceived = socket->recv(&(storageImageData[idx]),rcvBufferSize);
		totalBytesReceived += bytesReceived;
		idx = totalBytesReceived;
	}while(totalBytesReceived < size);
	return (true);
};




void printInfo(int argc, char *argv[]){
		  if (argc == 3){
			  return;
		  }else if (argc == 2){
			  printCompleteLicense(argc,argv);
		  }else{     // Test for correct number of arguments
		    cerr << "Usage of " << argv[0] << " : \n\n"
		         << argv[0] << " <server host> <server port> " << endl;
		    cerr << "\n"
		    	 << "<server host>   hostname and port number of the \n"
		    	 << "<server port>   running image data server \n";
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
    cout << "" << "'" << args[0] << " license' for details.\n\n";
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




