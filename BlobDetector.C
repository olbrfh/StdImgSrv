/*
 * BlobDetector.C
 *
 *  Created on: 29.08.2018
 *      Author: aml
 */

#include "BlobDetector.H"

#include <string>
#include <iostream>

using namespace std;

namespace BlobDetector{

BlobDetector::BlobDetector(){
	stdImgSrvVersion_ = string("not connected yet");
	dataSize_ = 4;
	receivedData_ =  new unsigned char[dataSize_];
}

BlobDetector::~BlobDetector(){
	this->close();
	delete [] receivedData_;
}

void BlobDetector::connect(char *host, unsigned short port){
	if(dataSource_ != NULL){
		return;
	}

	// client for image data
	try{
		dataSource_ = new TCPSocket(host, port);
	}catch(...){
		dataSource_ = NULL;
		string msg("No server for blob data.\n");
		msg += string(host);
		msg += string("\n\n");
		throw msg;
	};

	try{
		receiveMetaDataStdImgSrv(host,port);
	}catch(string msg){
		this->close();
		throw msg;
	}catch(...){
		this->close();
		throw string("unknown error");
	}

}

void BlobDetector::close(){
	if(dataSource_ != NULL){
		dataSource_->cleanUp();
		stdImgSrvVersion_ = string("not connected yet");
		delete dataSource_;
		dataSource_ = NULL;
	}
}

void BlobDetector::updateImageData(TCPSocket *socket, unsigned char *storageImageData, int size){
	int rcvBufferSize = size;
	int idx = 0;
	int bytesReceived = 0;
	int totalBytesReceived = 0;
	try{
		socket->send(GET_IMAGE_DATA,strlen(GET_IMAGE_DATA));
	}catch(SocketException &e){
		throw string(e.what());
	}catch(...){
		string msg("Error while sending: ");
		msg += string(GET_IMAGE_DATA);
		throw msg;
	};

	do{
		bytesReceived = socket->recv(&(storageImageData[idx]),rcvBufferSize);
		totalBytesReceived += bytesReceived;
		idx = totalBytesReceived;
	}while(totalBytesReceived < size);

	return;
};


void BlobDetector::getBlobCoord(int *X, int *Y){
	if(dataSource_ == NULL){
		throw string("not connected yet");
	}

	//
	try{
		this->updateImageData(dataSource_, receivedData_, dataSize_);
	}catch(string msg){
		throw msg;
	}catch(...){
		throw string("unknown error while reading image data.");
	}

	int bloobCoordWidth = 0;
	int bloobCoordHight = 0;

	bloobCoordWidth = (int) (receivedData_[1]);
	bloobCoordWidth = bloobCoordWidth << 8;
	bloobCoordWidth = bloobCoordWidth   | ((int) receivedData_[0]);

	bloobCoordHight = (int) (receivedData_[3]);
	bloobCoordHight = bloobCoordHight << 8;
	bloobCoordHight = bloobCoordHight   | ((int) receivedData_[2]);

	*X = bloobCoordWidth;
	*Y = bloobCoordHight;

	return;
}


string BlobDetector::version(){
	if(dataSource_ == NULL){
		throw string("not connected yet");
	}
	return stdImgSrvVersion_;
}


void BlobDetector::receiveMetaDataStdImgSrv(char *host, unsigned short port){
	int sizeEchoBufferMetaData = 64;
	char echoBufferMetaData[sizeEchoBufferMetaData];
	int bytesReceived = 0;

	try{
		dataSource_->send(GET_VERSION,strlen(GET_VERSION));
	}catch(...){
		string msg("Error while sending: ");
		msg += string(GET_VERSION);
		msg += string("\n\n");
		throw msg;
	};

	if( (bytesReceived = dataSource_->recv(echoBufferMetaData,sizeEchoBufferMetaData)) <= 0){
		string msg("Can't get Standard Image Data Server Version, terminate process.\n");
		throw msg;
	};

	echoBufferMetaData[bytesReceived]='\0';
	stdImgSrvVersion_ = string(echoBufferMetaData);

	try{
		dataSource_->send(GET_META_DATA,strlen(GET_META_DATA));
	}catch(...){
		string msg("Error while sending: ");
		msg += string(GET_META_DATA);
		msg += string("\n\n");
		throw msg;
	};

	echoBufferMetaData[0]='\0';
	if( (bytesReceived = dataSource_->recv(echoBufferMetaData,sizeEchoBufferMetaData)) <= 0){
		throw string("Can't get image meta data.\n");
	};

	// interpret received data
	int  imageWidth_, imageHeight_;
	char imageOrg_;
	int  color_;
	char ch1_, ch2_, ch3_;
	int  recvImageDataSize_;
	int  nmbBytesTimeStamp_;

	int nmbTokens = sscanf(echoBufferMetaData,"[W=%d,H=%d,O=%c,C=%d,X=%c%c%c,B=%d,BTS=%d]",
			&imageWidth_,&imageHeight_,&imageOrg_,&color_,&ch1_,&ch2_,&ch3_,&recvImageDataSize_,&nmbBytesTimeStamp_);
	if(nmbTokens != 9){
		throw string("Can't interpret image meta data, terminate process.\n");
	};

	if(color_ > 0){
		throw string("Received data are not grey valued.");
	}

	if((imageHeight_ != 1) || (imageWidth_ != 4)){
		throw string("Received data have not not 1x4 format.");
	}

}


} // end namespace BlobDetector
