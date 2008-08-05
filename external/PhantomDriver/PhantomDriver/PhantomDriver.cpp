//===========================================================================
/*
    This file is part of the CHAI 3D visualization and haptics libraries.
    Copyright (C) 2003-2004 by CHAI 3D. All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.

    For using the CHAI 3D libraries with software that can not be combined
    with the GNU GPL, and for taking advantage of the additional benefits
    of our support services, please contact CHAI 3D about acquiring a
    Professional Edition License.

    \author:    <http://www.chai3d.org>
    \author:    Federico Barbagli
    \version    1.4
    \date       06/2003
	\date		04/2004
*/
//===========================================================================

#include "stdafx.h"
#include <stdio.h>
#include <windows.h>
#include <wincon.h>
#include <stdlib.h>
#include <iostream.h>
#include "PhantomDriver.h"
#include <string>
#include <vector>
#include <algorithm>
#define PHANTOM_REGISTRY_KEY "SYSTEM\\CurrentControlSet\\Services\\PHANToM_IO\\Parameters"


//===============================================================================
//  Constants
//===============================================================================

const int GST_EFFECT_ACCESS = 0;
const int DEVICE_IO_ACCESS = 1;


//===============================================================================
//  Global variables
//===============================================================================

//! int lib_ver - Ghost library version
int lib_ver;

//! int type_access - type of access you would like to have from the Phantom
//! type_access = GST_EFFECT_ACCESS is the standard gstEffect, which works for both ghost3 and ghost4
//! type_access = DEVICE_IO_ACCESS is the deviceIO based access. This will work only if ghost4 is installed
int type_access = GST_EFFECT_ACCESS;

//! HINSTANCE hDLL - Handle to DLL.
HINSTANCE hDLL;         

//! int m_totalPhantomNumber - number of phantoms registered in the system (see Control Panel Phantom icon)
//! Note: when enabling the dual configuration, two more redundant phantoms appear in the Phantom Registry.
//! We do not consider these, i.e. these phantoms are not part of the list "m_totalPhantomList" and do not affect m_totalPhantomNumber;
int m_totalPhantomNumber = 0;

//! BOOL lib_loaded - Flag to avoid loading a libary more than once
BOOL lib_loaded = false;

//! BOOL deviceReady - Flag that tells us if the device is ready to operate
//! any error in the process of loading libraries and DLLs will cause this flag to be false
//! starts true. If it gets to the end of the process still being true everything went well
BOOL deviceReady = true;

//! std::vector<std::string> m_totalPhantomList - vector of strings containing all the names of the phantoms in the system
std::vector<std::string> m_totalPhantomList;


//===========================================================================================
//  Typedefs for functions and CallBacks
//===========================================================================================

typedef int (CALLBACK* LPFNDLLOpenPhantom)(char *);
typedef void (CALLBACK* LPFNDLLClosePhantoms)();
typedef int (CALLBACK* LPFNDLLResetPhantomEncoders)(int);
typedef int (CALLBACK* LPFNDLLStartCommunication)(int);
typedef int (CALLBACK* LPFNDLLStopCommunication)(int);
typedef int (CALLBACK* LPFNDLLReadPosition)(int,double&,double&,double&);
typedef int (CALLBACK* LPFNDLLReadNormalizedPosition)(int, double&,double&,double&);
typedef int (CALLBACK* LPFNDLLSetForce)(int, const double&, const double&, const double&);
typedef int (CALLBACK* LPFNDLLSetForceTorque)(int, const double&, const double&, const double&, const double&, const double&, const double&);
typedef int (CALLBACK* LPFNDLLReadOrientMat3DOF)(int, double *);
typedef int (CALLBACK* LPFNDLLReadOrientMat2DOF)(int, double *);
typedef int (CALLBACK* LPFNDLLReadSwitch)(int);
typedef double (CALLBACK* LPFNDLLGetMaxForce)(int);
typedef int (CALLBACK* LPFNDLLReadVelocity)(int,double&,double&,double&);


LPFNDLLOpenPhantom Open; 
LPFNDLLClosePhantoms Close;
LPFNDLLResetPhantomEncoders Reset;
LPFNDLLStartCommunication Start;
LPFNDLLStopCommunication Stop;
LPFNDLLReadPosition ReadP;
LPFNDLLReadNormalizedPosition ReadNormalized;
LPFNDLLSetForce Set;
LPFNDLLSetForceTorque SetFT;
LPFNDLLReadOrientMat3DOF ReadOr3;
LPFNDLLReadOrientMat2DOF ReadOr2;
LPFNDLLReadSwitch ReadS;
LPFNDLLGetMaxForce GetMF;
LPFNDLLReadVelocity GetV;





//===============================================================================
//  Global Functions
//===============================================================================

//! bool my_str_comp(std::string s1, std::string s2) - String compare for the phantom names: comparison is based on the names in the
//! phantom control panel all taken as lower case words (this is the way they are shown in the phantom control panel itself)
bool my_str_comp(std::string s1, std::string s2)
{
	std::string ss1 = s1;
	std::string ss2 = s2;
	std::transform (ss2.begin(),ss2.end(), ss2.begin(), tolower);
	std::transform (ss1.begin(),ss1.end(), ss1.begin(), tolower);
	if (ss1<ss2)
		return true;
	else
		return false;
}



//!  LoadLib loads the right version of the Phantom library, depending on what version of ghost is installed on the user's PC
//!		In case ghost3.1 is installed, LoadLib loads the gstEffect based DLL compiled against Ghost3.1
//!		In case ghost4.0 is installed, LoadLib loads, by default, the gstEffect based DLL compiled against Ghost4
//!		However, by using the PhantomAccess function one can demand the driver to attempt to load the DeviceIO
//!		based DLL compiled against Ghost4.0, if that is a real option.
//!
//!	 LoadLib returns TRUE if lib loads successfully, FALSE otherwise

BOOL LoadLib()
{

	HINSTANCE checkVal = NULL;
	// load the right library
	if (lib_ver == 50)
	{
		// 50 is the Touch3d Library
		checkVal = LoadLibrary("hd.dll");
		if (checkVal)
			// load the phantomHD.dll
			hDLL = LoadLibrary("phantomHD.dll");

	}
	else 
		if (lib_ver  == 31)
		{
			// double check that ghost40 is really installed in the system
			checkVal = LoadLibrary("ghost31.dll");
			if (checkVal)
				// load the phantomGstEffect31.dll
				hDLL = LoadLibrary("phantomGstEffect31.dll");
		}
		else 
			if (type_access == DEVICE_IO_ACCESS)
			{
				// double check that ghost40 is really installed in the system
				checkVal = LoadLibrary("GHOST40.dll");
				if (checkVal)
					// load the phantomDeviceIO40.dll
					hDLL = LoadLibrary("phantomDeviceIO40.dll");
	
			}
			else
			{
				// double check that ghost40 is really installed in the system
				checkVal = LoadLibrary("GHOST40.dll");
				if (checkVal)
					// load the phantomGstEffect40.dll
					hDLL = LoadLibrary("phantomGstEffect40.dll");

			}

	// if DLL was not loaded correctly exit returning FALSE
	if (!hDLL)
	{
		return FALSE;
	}

	// otherwise load all the function we need dynamically
	Open = (LPFNDLLOpenPhantom)GetProcAddress(hDLL,"OpenPhantom"); 
	Close = (LPFNDLLClosePhantoms)GetProcAddress(hDLL,"ClosePhantoms"); 
	Reset = (LPFNDLLResetPhantomEncoders)GetProcAddress(hDLL,"ResetPhantomEncoders");
	Start = (LPFNDLLStartCommunication)GetProcAddress(hDLL, "StartCommunication");
	Stop = (LPFNDLLStopCommunication)GetProcAddress(hDLL, "StopCommunication");
	ReadP  = (LPFNDLLReadPosition)GetProcAddress(hDLL, "ReadPosition");
	ReadNormalized = (LPFNDLLReadNormalizedPosition)GetProcAddress(hDLL, "ReadNormalizedPosition");
	Set = (LPFNDLLSetForce)GetProcAddress(hDLL, "SetForce");
	SetFT = (LPFNDLLSetForceTorque)GetProcAddress(hDLL, "SetForceTorque");
	ReadOr3 = (LPFNDLLReadOrientMat3DOF)GetProcAddress(hDLL, "ReadOrientMat3DOF");
	ReadOr2 = (LPFNDLLReadOrientMat2DOF)GetProcAddress(hDLL, "ReadOrientMat2DOF");
	ReadS = (LPFNDLLReadSwitch)GetProcAddress(hDLL, "ReadSwitch");
	GetMF = (LPFNDLLGetMaxForce)GetProcAddress(hDLL, "GetMaxForce");
	GetV = (LPFNDLLReadVelocity)GetProcAddress(hDLL, "ReadVelocity");

	// in case of error free the DLL and return false
	if (!Open || !Close|| !Reset || !Start || !Stop || !ReadP || !ReadNormalized || !Set || !SetFT || !ReadOr3 || !ReadS || !GetMF || !GetV) 
    { 
		FreeLibrary(hDLL); 
		return FALSE;
	} 

	return true;

}



//!  int get_phantom_count() 
//!		reads from the registry how many Phantoms have been initialized under the Phantom Control Panel, saves it in m_totalPhantomNumber

int get_phantom_count() 
{

  HKEY hKey;

  // Open our registry key
  long result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(PHANTOM_REGISTRY_KEY),
    0, KEY_QUERY_VALUE, &hKey);

  if (result != ERROR_SUCCESS) {
    return -1;
  }

  DWORD numvalues;

  // Ask how many values are defined within this key
  result = RegQueryInfoKey(hKey,0,0,0,0,0,0,&numvalues,0,0,0,0);

  if (result != ERROR_SUCCESS) {
    return -1;
  }

  RegCloseKey(hKey);

  return numvalues;
}



//!  int get_phantom_name() 
//!		Populates the list of phantom names available in the system and saves it in m_totalPhantomList.
//!		Returns -1 for an error, 0 if all goes well.
//!		Note: this function checks if the dual configuration is enabled and, in that case, decreases m_totalPhantomNumber by
//!		two and does not save the DualPHANToM1 and DualPHANToM2 entries in the m_totalPhantomList list.

int get_phantom_name() 
{

  HKEY hKey;

  // Open our registry key
  long result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(PHANTOM_REGISTRY_KEY),
    0, KEY_QUERY_VALUE, &hKey);

  if (result != ERROR_SUCCESS) {
    return -1;
  }

  DWORD numvalues;

  // Ask how many values are defined within this key
  result = RegQueryInfoKey(hKey,0,0,0,0,0,0,&numvalues,0,0,0,0);

  if (result != ERROR_SUCCESS) {
    return -1;
  }

  
  bool DUAL_CONFIG = false;  
  for (int i=0; i<m_totalPhantomNumber; i++) 
  {
     unsigned char data[1000];
     unsigned long data_len = 1000;
	 char value_name[1000];
	 unsigned long value_len = 1000;
	 DWORD type;
	 
	 result = RegEnumValue(hKey,i,value_name,&value_len,0,&type,data,&data_len);
     if (result != ERROR_SUCCESS) 
	 {
       return -1;
	 }

	 if (type != REG_SZ) 
	 {
		return -1;
	 }
	 
	 // if dual config is ON then there will be two more phantom names in the register
	 // ignore them
	 if (value_name[0] == 'D')
		 DUAL_CONFIG = true;
	 else
	 {
		 char* cdata = (char*)(data);
	     std::string name_s = std::string(cdata);
		 m_totalPhantomList.push_back(name_s);
	 }
  }
	
  // order the list of phantom names
  std::vector<std::string>::iterator i1 = m_totalPhantomList.begin();
  std::vector<std::string>::iterator i2 = m_totalPhantomList.end();
  std::sort(i1, i2, my_str_comp);
  // check if dual config is on or off
  if (DUAL_CONFIG)
	m_totalPhantomNumber = m_totalPhantomNumber - 2;

  RegCloseKey(hKey);

  return 0;

}






//===========================================================================================
//  Entry point in the DLL
//===========================================================================================

//!  This function: 
//!		finds the version of ghost
//!		reads the number of phantoms in the system
//!		extracts all the names of the phantom and orders them as in the control panel

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{

	// get ghost version installed on the machine
	char *libvarGhost;
    libvarGhost = getenv( "GHOST_LIB_VERSION" );
	char *libvar3Dtouch;
    libvar3Dtouch = getenv( "3DTOUCH_BASE" );
    if ((libvarGhost == NULL) && (libvar3Dtouch == NULL))
		deviceReady = false;
	else if (libvar3Dtouch!= NULL)
		lib_ver = 50;
	else
	{
		if (strcmp(libvarGhost, "40") == 0)
			lib_ver = 40;
		else if (strcmp(libvarGhost, "31") == 0)
			lib_ver = 31;
		else
			deviceReady = false;
	}


	// get number of phantoms
	m_totalPhantomNumber = get_phantom_count();
	if (m_totalPhantomNumber <= 0)
		deviceReady = false;


	// get all names for phantoms
	int result = get_phantom_name();
	if (result < 0) 
		deviceReady = false;

    return TRUE;
}



//===============================================================================
//  Phantom functions called from outside
//===============================================================================


//==========================================================================
/*!
	If access_type = 1 the system tries to load the deviceIO based phantom access. This will
	only take place, however, if ghost4 is installed. This function does not need to be called. If not called
	the system loads the gstEffect phantom access by default.
	Note: access modes for all phantoms are the same.

      \fn       void PhantomAcces(int access_type)
	  \param	int num
*/
//===========================================================================
FUNCTION void __stdcall   PhantomAcces(int access_type)
{
	// this is the variable used when trying to decide what version of the ghost 4 libs to load.
	type_access = access_type;
}


//==========================================================================
/*!
	 This function opens a Phantom device port. The phantom is specified by 
	 an integer (n>0) that represents the nth phantom in the the Phantom Control 
	 Panel list. If the phantom is opened successfully the function returns a handle 
	 (non-negative integer) for the phantom. If something goes wrong a negative value
	 is returned.

      \fn       int OpenPhantom(int num)
	  \param	int num
*/
//===========================================================================
FUNCTION int __stdcall   OpenPhantom(int num)
{
	if (!lib_loaded)
		if (!LoadLib())
		{
			deviceReady = false;
			return PH_DLL_PROBLEM;
		}
		else
			lib_loaded = true;

	if ((num < 0) || (num>=m_totalPhantomNumber) || (deviceReady == false))
		return PH_DLL_PROBLEM;
	else
	{	
		char name[100];
		strcpy(name, m_totalPhantomList[num].c_str());
		return Open(name);
	}

	
}

//==========================================================================
/*!
	 Last function to be called: closes all the phantoms. In case no phantom currently
	exists the value returned is negative, 1 in case of success.
	

      \fn       int ClosePhantoms()
	  \param	int num
*/
//===========================================================================

FUNCTION void __stdcall   ClosePhantoms()
{
	if ((lib_loaded) && (deviceReady))
	{
		Close();
		lib_loaded = false;
		FreeLibrary(hDLL);
	}
}

 
//==========================================================================
/*!
	 Reset Phantom number num. Returns a negative value is operation failed, 
	 1 in case of success.

      \fn       int ResetPhantomEncoders(int num)
	  \param	int num
*/
//===========================================================================
FUNCTION int __stdcall   ResetPhantomEncoders(int num)
{
	if ((lib_loaded) && (deviceReady))
		return Reset(num);
	else 
		return PH_DLL_PROBLEM;
}

//==========================================================================
/*!
	This function starts the Communication with the phantom i. Returns 1 if everything
	went OK, a negative value otherwise.

      \fn       int StartCommunicationPhantom(int i)
	  \param	int num
*/
//===========================================================================
FUNCTION int __stdcall   StartCommunicationPhantom(int i)
{
	if ((lib_loaded) && (deviceReady))
		return Start(i);
	else 
		return PH_DLL_PROBLEM;
}

//==========================================================================
/*!
	StopCommunicationPhantom(int i), Stops the effect of phantom i, basically disabling forces and position
	reading for such phantom. Note that the overall servoloop will still be running since the other phantom
	may be not disabled.
	The function returns 1 if everything went ok, a negative value otherwise (check list of errors)
    
	  \fn       int StopCommunicationPhantom(int i)
	  \param	int i
*/
//===========================================================================
FUNCTION int __stdcall   StopCommunicationPhantom(int i)
{
	if ((lib_loaded) && (deviceReady))
		return Stop(i);
	else 
		return PH_DLL_PROBLEM;
}

//==========================================================================
/*!
	ReadPositionPhantom(int num,double &iPosX,double &iPosY,double &iPosZ); reads tip position for phantom num
	the function returns 1 if everything went ok, a negative value otherwise (check list of errors). 
	Position values are returned in the three iPos variables.
	Note that positions are expressed in mm with respect to a Ghost reference frame (X: right, Y: up, Z: toward user)
	

  	  \fn       int ReadPositionPhantom
	  \param	int num
	  \param	int iPosX
	  \param	int iPosY
	  \param	int iPosZ
*/
//===========================================================================
FUNCTION int __stdcall   ReadPositionPhantom(int num, 
									  double &iPosX,
									  double &iPosY,
									  double &iPosZ)
{
	if ((lib_loaded) && (deviceReady))
		return ReadP(num, iPosX, iPosY, iPosZ);
	else 
		return PH_DLL_PROBLEM;
}


//==========================================================================
/*!
	ReadNormalizedPositionPhantom(int num,double &iPosX,double &iPosY,double &iPosZ); reads tip position for phantom num
	the function returns 1 if everything went ok, a negative value otherwise (check list of errors).
	Position values are returned in the three iPos variables.
	Note that positions are expressed with a value included in the interval [-1,1] for a cube centered in the device's workspace center.
	This is to ensure that a same demo may be used using different devices without having to change any of the code
	Note that positions are expressed in mm with respect to a Ghost reference frame (X: right, Y: up, Z: toward user)
	

  	  \fn       int ReadNormalizedPositionPhantom
	  \param	int num
	  \param	int iPosX
	  \param	int iPosY
	  \param	int iPosZ
*/
//===========================================================================
FUNCTION int __stdcall   ReadNormalizedPositionPhantom(int num, 
									  double &iPosX,
									  double &iPosY,
									  double &iPosZ)
{
	if ((lib_loaded) && (deviceReady))
		return ReadNormalized(num, iPosX, iPosY, iPosZ);
	else 
		return PH_DLL_PROBLEM;
}


//==========================================================================
/*!
	SetForcePhantom(int num,const double &iForceX,const double &iForceY,const double &iForceZ); writes force to phantom num
	the function returns 1 if everything went ok, a negative value otherwise (check list of errors)
	Note that forces are expressed in Newtons with respect to a Ghost reference frame (X: right, Y: up, Z: toward user)
	Note: no safety features are implemented other than the standard Ghost ones.
	
	

  	  \fn       int SetForcePhantom
	  \param	int num
	  \param	int iForceX
	  \param	int iForceY
	  \param	int iForceZ
*/
//===========================================================================
FUNCTION int __stdcall   SetForcePhantom(int num, 
								  const double &iForceX,
				                  const double &iForceY,
								  const double &iForceZ)
{
	if ((lib_loaded) && (deviceReady))
		return Set(num, iForceX, iForceY, iForceZ);
	else 
		return PH_DLL_PROBLEM;
}


//==========================================================================
/*!
	SetForceTorquePhantom(int num, const double &iForceX, const double &iForceY, const double &iForceZ, const double &iTorqueX, const double &iTorqueY, const double &iTorqueZ);
	writes Forces and Torques to phantom num.
	the function returns 1 if everything went ok, a negative value otherwise (check list of errors)
	Note that forces are expressed in Newtons and torques are expressed in Newtons Meter with respect to a Phantom reference frame (X: right, Y: up, Z: toward user)
	Note: no safety features are implemented other than the standard Ghost ones.
	
	

  	  \fn       int SetForceTorquePhantom
	  \param	int num
	  \param	int iForceX
	  \param	int iForceY
	  \param	int iForceZ
	  \param	int iTorqueX
	  \param	int iTorqueY
	  \param	int iTorqueZ
*/
//===========================================================================
FUNCTION int __stdcall   SetForceTorquePhantom(int num, 
								  const double &iForceX,
				                  const double &iForceY,
								  const double &iForceZ,
								  const double &iTorqueX,
				                  const double &iTorqueY,
								  const double &iTorqueZ)
{
	if ((lib_loaded) && (deviceReady))
		return SetFT(num, iForceX, iForceY, iForceZ, iTorqueX, iTorqueY, iTorqueZ);
	else 
		return PH_DLL_PROBLEM;
}



//==========================================================================
/*!
	ReadOrientMat3DOFPhantom(int num, double *m);, reads the orientation matrix of the stylus for a 3dof wristed
	phantom device and returns it in a Phantom coordinate frame.
	The function returns 1 if everything went ok, a negative value otherwise (check list of errors). 
	

  	  \fn       int ReadOrientMat3DOFPhantom
	  \param	int num
	  \param	double *m
*/
//===========================================================================
FUNCTION int __stdcall   ReadOrientMat3DOFPhantom(int num, 
 									  double *m)
{
	if ((lib_loaded) && (deviceReady))
		return ReadOr3(num, m);
	else 
		return PH_DLL_PROBLEM;
}



//==========================================================================
/*!
	ReadSwitchPhantom(int num);, reads the switch from phantom num
	

  	  \fn       int ReadSwitchPhantom
	  \param	int num
	  
*/
//===========================================================================
FUNCTION int __stdcall   ReadSwitchPhantom(int num)
{
	if ((lib_loaded) && (deviceReady))
		return ReadS(num);
	else 
		return PH_DLL_PROBLEM;
}


//==========================================================================
/*!
	double GetMaxForcePhantom(int num),	reads what the max force is for Phantom num


  	  \fn       double GetMaxForcePhantom
	  \param	int num
	  
*/
//===========================================================================
FUNCTION double __stdcall   GetMaxForcePhantom(int num)
{
	if ((lib_loaded) && (deviceReady))
		return GetMF(num);
	else 
		return PH_DLL_PROBLEM;

}



//==========================================================================
/*!
	ReadVelocityPhantom(int num,double &iVelX,double &iVelY,double &iVelZ); reads tip velocity for phantom num
	the function returns 1 if everything went ok, a negative value otherwise (check list of errors)
	Note that velocity is expressed in mm/sec with respect to a Ghost reference frame (X: right, Y: up, Z: toward user)
	

  	  \fn       int ReadVelocityPhantom
	  \param	int num
	  \param	int iVelX
	  \param	int iVelY
	  \param	int iVelZ
*/
//===========================================================================
FUNCTION int __stdcall   ReadVelocityPhantom(int num, 
									  double &iVelX,
									  double &iVelY,
									  double &iVelZ)
{
	if ((lib_loaded) && (deviceReady))
		return GetV(num, iVelX, iVelY, iVelZ);
	else 
		return PH_DLL_PROBLEM;

}