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


Socket.o:	./src/Socket.cpp
	$(CC) $(CFLAGS) $(INCL) -g -DLINUX -D__LINUX__ -DUNIX -c $<

stdImgDataServerSim.o:	./src/stdImgDataServerSim.cpp  ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT -c $<

stdImgDataServerLapCam.o:	./src/stdImgDataServerLapCam.cpp  ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT -c $<

stdImgDataServerClientColorFilter.o:	./src/stdImgDataServerClientColorFilter.cpp  ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT -c $<
	
stdImgDataServerClientBlobDetector.o:	./src/stdImgDataServerClientBlobDetector.cpp  ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT -c $<	

BlobDetector.o:	./src/BlobDetector.cpp  ./include/BlobDetector.H ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT -c $<	

testClient.o:	./src/testClient.cpp  ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT   -c $<

testClientBlobDetector.o:	./src/testClientBlobDetector.cpp  ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT   -c $<
	
testOppBlobDetector.o:	./src/testOppBlobDetector.cpp ./src/BlobDetector.cpp ./include/BlobDetector.H
	$(CC) $(CFLAGS) $(INCL)   -g -DLINUX -D__LINUX__ -DUNIX -D_REENTRANT   -c $<

stdImgDataServerSim: Socket.o stdImgDataServerSim.o ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS) $(INCL) -I/usr/local/lib   \
	-lpthread -D_REENTRANT \
	-lm -lstdc++  Socket.o  -lpthread \
	stdImgDataServerSim.o -o stdImgDataServerSim
	
stdImgDataServerLapCam: Socket.o stdImgDataServerLapCam.o ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS)  stdImgDataServerLapCam.o -o stdImgDataServerLapCam   \
	$(LIBS) -lpthread -D_REENTRANT \
	-lm -lstdc++  Socket.o  -lpthread  
		

stdImgDataServerClientColorFilter: Socket.o stdImgDataServerClientColorFilter.o ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS)  stdImgDataServerClientColorFilter.o -o stdImgDataServerClientColorFilter   \
	$(LIBS) -lpthread -D_REENTRANT \
	-lm -lstdc++  Socket.o  -lpthread 

stdImgDataServerClientBlobDetector: Socket.o stdImgDataServerClientBlobDetector.o ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS)  stdImgDataServerClientBlobDetector.o -o stdImgDataServerClientBlobDetector   \
	$(LIBS) -lpthread -D_REENTRANT \
	-lm -lstdc++  Socket.o  -lpthread 

testClient: testClient.o Socket.o ./src/testClient.cpp ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS)  testClient.o Socket.o -o testClient $(LIBS) -ldl -lstdc++ -lm -std=c++11 \
	
testClientBlobDetector:	testClientBlobDetector.o Socket.o ./src/testClientBlobDetector.cpp ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS)  testClientBlobDetector.o Socket.o -o testClientBlobDetector $(LIBS) -ldl -lstdc++ -lm -std=c++11 \

testOppBlobDetector:	testOppBlobDetector.o Socket.o BlobDetector.o ./include/StdImgDataServerProtocol.H
	$(CC) $(CFLAGS)  testOppBlobDetector.o Socket.o BlobDetector.o  -o testOppBlobDetector $(LIBS) -ldl -lstdc++ -lm -std=c++11 \



#cleaning up
clean:
	rm -r *.o  $(TARGETS)
