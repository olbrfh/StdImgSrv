/*
 * testOppBlobDetector.C
 *
 *  Created on: 29.08.2018
 *      Author: aml
 */

#include "Socket.H"  // For Socket, ServerSocket, and SocketException
#include <iostream>           // For cerr and cout
#include <cstdlib>            // For atoi()

#include "StdImgDataServerProtocol.H"
#include "BlobDetector.H"

unsigned short SOURCE_SERVER_PORT_;
char          *SOURCE_SERVER_ADR_;

void printInfo(int argc, char *argv[]);
void printCompleteLicense(int argc, char* args[]);
void printLicense(int argc, char* args[]);



/**
 *
 * @param argc number of command line parameter
 * @param *argv[] list of parameters
 */
int main(int argc, char *argv[]){

	printInfo(argc,argv);

	try{
		SOURCE_SERVER_ADR_  = argv[1];
		SOURCE_SERVER_PORT_ = atoi(argv[2]);
	}catch(...){
		cerr << "Can't read command line parameters, terminate process.\n";
		exit(0);
	};

	BlobDetector::BlobDetector bd;
	try{
		bd.connect(SOURCE_SERVER_ADR_, SOURCE_SERVER_PORT_);
		cout << bd.version() << endl;

		int x, y;
		while(1){
			bd.getBlobCoord(&x,&y);
			if((x != 0) && (y != 0)){
				cout << "(x,y): " << x << ":" << y << endl;
			}
		}

	}catch(string msg){
		bd.close();
		cerr << msg << endl;
		exit(0);
	}
}


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

