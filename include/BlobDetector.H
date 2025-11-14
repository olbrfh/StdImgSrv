/*
 * BlobDetector.H
 *
 *  Created on: 29.08.2018
 *      Author: aml
 */

#ifndef BLOBDETECTOR_H_
#define BLOBDETECTOR_H_

#include "Socket.H"  // For Socket, ServerSocket, and SocketException
#include "StdImgDataServerProtocol.H"

/**
 *
 * namespace
 *
 */
namespace BlobDetector{



/**
 *
 *  \class IBlobDetector
 *
 * \brief Interface class
 *
 */
class IBlobDetector{
public:
	/**
	 *
	 * \brief Connects...
	 */
	virtual void   connect(char *host, unsigned short port) = 0;

	/**
	 *
	 * \brief Closes...
	 *
	 */
	virtual void   close() = 0;

	/**
	 *
	 * \brief Delivers the curret servo motor position
	 * of the pan motor.
	 *
	 * \@return int servo-motor position in degree within the range [-90;90]
	 *
	 * \throws string "no com" if serial communication is not available
	 */
	virtual void  getBlobCoord(int *X, int *Y) = 0;

};


class BlobDetector{
public:
	BlobDetector();
	~BlobDetector();
	virtual void connect(char *host, unsigned short port);
	virtual void close();
	virtual void getBlobCoord(int *X, int *Y);

	string version();

private:
	void receiveMetaDataStdImgSrv(char *host, unsigned short port);
	void updateImageData(TCPSocket *socket, unsigned char *storageImageData, int size);

	TCPSocket     *dataSource_ = NULL;
	string         stdImgSrvVersion_;
	int            dataSize_;
	unsigned char *receivedData_;
};

}  // end namespace BlobDetector




#endif /* BLOBDETECTOR_H_ */
