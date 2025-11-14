#########################################################################
# FILE NAME Makefile
#           copyright 2006 by University of Wales Aberystwyth
#
# BRIEF MODULE DESCRIPTION
#           Makefile for the standard image server 
#
# History:
#
#  01.09.2009 - initial version 
#
#
#########################################################################

CC = g++


CFLAGS= -std=c++11 $(shell pkg-config --cflags opencv) 
LIBS=$(shell pkg-config --libs opencv) 



TARGETS = stdImgDataServerSim  testClient stdImgDataServerLapCam stdImgDataServerClientColorFilter stdImgDataServerClientBlobDetector testClientBlobDetector testOppBlobDetector


all:	$(TARGETS)


Socket.o:	Socket.C
	$(CC) $(CFLAGS) $(INCL) -g -DLINUX -D__LINUX__ -DUNIX -c $<

stdImgDataServerSim.o:	stdImgDataServerSim.C  StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT -c $<

stdImgDataServerLapCam.o:	stdImgDataServerLapCam.C  StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT -c $<

stdImgDataServerClientColorFilter.o:	stdImgDataServerClientColorFilter.C  StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT -c $<
	
stdImgDataServerClientBlobDetector.o:	stdImgDataServerClientBlobDetector.C  StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT -c $<	

BlobDetector.o:	BlobDetector.C  BlobDetector.H StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT -c $<	

testClient.o:	testClient.C  StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT   -c $<

testClientBlobDetector.o:	testClientBlobDetector.C  StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT   -c $<
	
testOppBlobDetector.o:	testOppBlobDetector.C BlobDetector.C BlobDetector.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT   -c $<

stdImgDataServerSim: Socket.o stdImgDataServerSim.o StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL) -I/usr/local/lib   \
	-lpthread -D_REENTRANT \
	-lm -lstdc++  Socket.o  -lpthread \
	stdImgDataServerSim.o -o stdImgDataServerSim
	
stdImgDataServerLapCam: Socket.o stdImgDataServerLapCam.o StdImgDataServerProtocol.H
	$(CC) $(CFLAGS)  stdImgDataServerLapCam.o -o stdImgDataServerLapCam   \
	$(LIBS) -lpthread -D_REENTRANT \
	-lm -lstdc++  Socket.o  -lpthread  
		

stdImgDataServerClientColorFilter: Socket.o stdImgDataServerClientColorFilter.o StdImgDataServerProtocol.H
	$(CC) $(CFLAGS)  stdImgDataServerClientColorFilter.o -o stdImgDataServerClientColorFilter   \
	$(LIBS) -lpthread -D_REENTRANT \
	-lm -lstdc++  Socket.o  -lpthread 

stdImgDataServerClientBlobDetector: Socket.o stdImgDataServerClientBlobDetector.o StdImgDataServerProtocol.H
	$(CC) $(CFLAGS)  stdImgDataServerClientBlobDetector.o -o stdImgDataServerClientBlobDetector   \
	$(LIBS) -lpthread -D_REENTRANT \
	-lm -lstdc++  Socket.o  -lpthread 

testClient: testClient.o Socket.o testClient.C StdImgDataServerProtocol.H
	$(CC) $(CFLAGS)  testClient.o Socket.o -o testClient $(LIBS) -ldl -lstdc++ -lm -std=c++11 \
	
testClientBlobDetector:	testClientBlobDetector.o Socket.o testClientBlobDetector.C StdImgDataServerProtocol.H
	$(CC) $(CFLAGS)  testClientBlobDetector.o Socket.o -o testClientBlobDetector $(LIBS) -ldl -lstdc++ -lm -std=c++11 \

testOppBlobDetector:	testOppBlobDetector.o Socket.o BlobDetector.o StdImgDataServerProtocol.H
	$(CC) $(CFLAGS)  testOppBlobDetector.o Socket.o BlobDetector.o  -o testOppBlobDetector $(LIBS) -ldl -lstdc++ -lm -std=c++11 \



#cleaning up
clean:
	rm -r *.o  $(TARGETS)
