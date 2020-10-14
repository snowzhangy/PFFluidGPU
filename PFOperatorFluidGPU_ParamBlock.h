/**********************************************************************
 *<
	FILE:			PFOperatorFluidGPU_ParamBlock.h

	DESCRIPTION:	ParamBlocks2 for SimpleSpeed Operator (declaration)
											 
	CREATED BY:		David C. Thompson

	HISTORY:		created 01-15-02

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/

#ifndef  _PFOPERATORSIMPLESPEED_PARAMBLOCK_H_
#define  _PFOPERATORSIMPLESPEED_PARAMBLOCK_H_

#include "max.h"
#include "iparamm2.h"

#include "PFOperatorFluidGPUDesc.h"

namespace PFActions {

// block IDs
enum { kpFlowFuild_mainPBlockIndex };


// param IDs
enum {	kpFlowFuild_speed, kpFlowFuild_viscosity,  kpFlowFuild_diffusion,  kpFlowFuild_mass,kpFlowFuild_direction,kpFlowFuild_temperature,
		kpFlowFuild_reverse, kpFlowFuild_divergence, kpFlowFuild_seed };

// directions
enum {	kSS_Along_Icon_Arrow,
		kSS_Icon_Center_Out,
		kSS_Icon_Arrow_Out,
		kSS_Rand_3D,
		kSS_Rand_Horiz,
		kSS_Inherit_Prev = 5 };

// user messages
enum { kpFlowFuild_RefMsg_NewRand = REFMSG_USER + 13279,
kpFlowFuild_RefMsg_CleanSim};

class PFOperatorSimpleSpeedDlgProc : public ParamMap2UserDlgProc
{
	void SetCtlByDir(IParamMap2* map, HWND hWnd);
public:
	INT_PTR DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() {}
};

extern PFOperatorFluidGPUDesc ThePFOperatorSimpleSpeedDesc;
extern ParamBlockDesc2 simpleSpeed_paramBlockDesc;
extern FPInterfaceDesc simpleSpeed_action_FPInterfaceDesc;
extern FPInterfaceDesc simpleSpeed_operator_FPInterfaceDesc;
extern FPInterfaceDesc simpleSpeed_PViewItem_FPInterfaceDesc;


} // end of namespace PFActions

#endif // _PFOPERATORSIMPLESPEED_PARAMBLOCK_H_