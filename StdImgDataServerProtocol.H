/*
    Declarations for the protocol of a standard image data server.

    Copyright (C) 2009  Martin Huelse,
    http://aml.somebodyelse.de


	This file is part of the IRG Standard Image Data Server Library.

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


#ifndef STDIMGDATASERVERPROTOCOL_H_
#define STDIMGDATASERVERPROTOCOL_H_


// commands
static char* GET_VERSION    = (char *)"GET_VERSION\0";
static char* GET_META_DATA  = (char *)"GET_META_DATA\0";
static char* GET_IMAGE_DATA = (char *)"GET_IMAGE_DATA\0";

// responses
static char* CURRENT_VERSION = (char *)"IRG STD IMG SRV 1.0.0\0";
static char* UNKNOWN_COMMAND = (char *)"UNKNOWN COMMAND\0";



#endif /* STDIMGDATASERVERPROTOCOL_H_ */
