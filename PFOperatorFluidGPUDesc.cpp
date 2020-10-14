/**********************************************************************
 *<
	FILE:			PFOperatorFluidGPUDesc.cpp

	DESCRIPTION:	SimpleSpeed Operator Class Descriptor (definition)
											 
	CREATED BY:		David C. Thompson

	HISTORY:		created 01-15-02

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/

#include "resource.h"

#include "PFOperatorFluidGPUDesc.h"

#include "PFActions_GlobalVariables.h"
#include "PFActions_GlobalFunctions.h"
#include "PFActions_SysUtil.h"
#include "PFOperatorFluidGPU.h"
#include "PFClassIDs.h"
#include "IPFAction.h"
#define PFOperatorFluid_Class_ID				Class_ID(0x74f93b2a, PFActionClassID_PartB)// Yang

namespace PFActions {

HBITMAP PFOperatorFluidGPUDesc::m_depotIcon = NULL;
HBITMAP PFOperatorFluidGPUDesc::m_depotMask = NULL;

PFOperatorFluidGPUDesc::~PFOperatorFluidGPUDesc()
{
	if (m_depotIcon != NULL) {
		DeleteObject(m_depotIcon);
		m_depotIcon = NULL;
	}
	if (m_depotMask != NULL) {
		DeleteObject(m_depotMask);
		m_depotMask = NULL;
	}
}

int PFOperatorFluidGPUDesc::IsPublic()
{
	return 0;
}

void* PFOperatorFluidGPUDesc::Create(BOOL loading) 
{
	return new PFOperatorFluidGPU();
}

const TCHAR* PFOperatorFluidGPUDesc::ClassName() 
{
	return GetString(IDS_OPERATOR_SIMPLESPEED_CLASS_NAME);
}

SClass_ID PFOperatorFluidGPUDesc::SuperClassID() 
{
	return HELPER_CLASS_ID;
}

Class_ID PFOperatorFluidGPUDesc::ClassID()
{
	return PFOperatorFluid_Class_ID;
}

Class_ID PFOperatorFluidGPUDesc::SubClassID()
{
	return PFOperatorSubClassID;
}

const TCHAR* PFOperatorFluidGPUDesc::Category() 
{
	return GetString(IDS_PARTICLEFLOW_CATEGORY);
}

const TCHAR* PFOperatorFluidGPUDesc::InternalName()
{
	//return GetString(IDS_OPERATOR_SIMPLESPEED_INTERNAL_NAME);
	return _T("Speed");
}

HINSTANCE PFOperatorFluidGPUDesc::HInstance()
{
	return hInstance;
}

INT_PTR PFOperatorFluidGPUDesc::Execute(int cmd, ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3)
{
	TCHAR** res = NULL;
	HBITMAP* depotIcon;
	HBITMAP* depotMask;
	switch( cmd )
	{
	case kPF_GetActionDescription:
		if (arg1 == NULL) return 0;
		res = (TCHAR**)arg1;
		*res = GetString(IDS_OPERATOR_SIMPLESPEED_DESCRIPTION);
		break;
	case kPF_PViewCategory:
		if (arg1 == NULL) return 0;
		res = (TCHAR**)arg1;
		*res = GetString(IDS_PF_OPERATOR_CATEGORY);
		break;
	case kPF_PViewDepotIcon:
		if (arg1 == NULL) return 0;
		depotIcon = (HBITMAP*)arg1;
		if (arg2 == NULL) return 0;
		depotMask = (HBITMAP*)arg2;
		if (m_depotIcon == NULL)
			m_depotIcon = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_SIMPLESPEED_DEPOTICON));
		if (m_depotMask == NULL)
			m_depotMask = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_DEPOTICONMASK_OPERATOR));
		*depotIcon = m_depotIcon;
		*depotMask = m_depotMask;
		break;
	default:
		return 0;
	}
	return 1;
}



} // end of namespace PFActions