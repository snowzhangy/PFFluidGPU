/**********************************************************************
 *<
	FILE:			DLLMain.cpp

	DESCRIPTION:	Defines the entry point for the DLL application.
											 
	CREATED BY:		Oleg Bayborodin

	HISTORY:		created 10-22-01

 *>	Copyright (c) 2001, All Rights Reserved.
 **********************************************************************/


#include "Max.h"
#include "resource.h"

#include "PFActions_GlobalVariables.h"
#include "PFActions_GlobalFunctions.h"

BOOL APIENTRY DllMain( HINSTANCE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			// Hang on to this DLL's instance handle.
			PFActions::hInstance = hModule;
         DisableThreadLibraryCalls(hModule);
			break;
	}

	return(TRUE);
}

// This function returns the number of plug-in classes this DLL implements
__declspec( dllexport ) int LibNumberClasses() 
{
	return 2;
}

// This function return the ith class descriptor
__declspec( dllexport ) ClassDesc* 
LibClassDesc(int i) 
{
	switch(i) 
	{
			// operators
		case 0:		return PFActions::GetPFOperatorSimpleSpeedDesc();
		default:	return 0;
	}
}

__declspec( dllexport ) const TCHAR *
LibDescription() 
{ 
	return _T(PFActions::GetString(IDS_PLUGINDESCRIPTION)); 
}

// Return version so can detect obsolete DLLs
__declspec( dllexport ) ULONG LibVersion() 
{ 
	return VERSION_3DSMAX; 
}

// don't defer plug-in loading since it may interfere with maxscript creation [bayboro 04-07-2003]
// Let the plug-in register itself for deferred loading
//__declspec( dllexport ) ULONG CanAutoDefer()
//{
//	return 1;
//}





