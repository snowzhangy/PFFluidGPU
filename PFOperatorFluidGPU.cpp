/**********************************************************************
 *<
	FILE:			PFOperatorFluidGPU.cpp

	DESCRIPTION:	SimpleSpeed Operator implementation
					Operator to effect speed unto particles

	CREATED BY:		David C. Thompson

	HISTORY:		created 01-15-02
					modified 07-23-02 [o.bayborodin]

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/


#include "resource.h"

#include "PFActions_SysUtil.h"
#include "PFActions_GlobalFunctions.h"
#include "PFActions_GlobalVariables.h"

#include "PFOperatorFluidGPU.h"

#include "PFOperatorFluidGPU_ParamBlock.h"
#include "PFClassIDs.h"
#include "IPFSystem.h"
#include "IParticleObjectExt.h"
#include "IParticleChannels.h"
#include "IChannelContainer.h"
#include "IPViewManager.h"
#include "PFMessages.h"
#define PFOperatorFluid_Class_ID				Class_ID(0x74f93b2a, PFActionClassID_PartB)// Yang
namespace PFActions {

PFOperatorFluidGPU::PFOperatorFluidGPU()
{ 
	pfluid=new PFFluidMathGPU();
	pfluid->clearArray();
	GetClassDesc()->MakeAutoParamBlocks(this); 
}

FPInterfaceDesc* PFOperatorFluidGPU::GetDescByID(Interface_ID id)
{
	if (id == PFACTION_INTERFACE) return &simpleSpeed_action_FPInterfaceDesc;
	if (id == PFOPERATOR_INTERFACE) return &simpleSpeed_operator_FPInterfaceDesc;
	if (id == PVIEWITEM_INTERFACE) return &simpleSpeed_PViewItem_FPInterfaceDesc;
	return NULL;
}

void PFOperatorFluidGPU::GetClassName(TSTR& s)
{
	s = GetString(IDS_OPERATOR_SIMPLESPEED_CLASS_NAME);
}

Class_ID PFOperatorFluidGPU::ClassID()
{
	return PFOperatorFluid_Class_ID;
}

void PFOperatorFluidGPU::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
	GetClassDesc()->BeginEditParams(ip, this, flags, prev);
}

void PFOperatorFluidGPU::EndEditParams(IObjParam *ip,	ULONG flags,Animatable *next)
{
	GetClassDesc()->EndEditParams(ip, this, flags, next );
}

RefResult PFOperatorFluidGPU::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget,
														PartID& partID, RefMessage message)
{
	switch(message) {
		case REFMSG_CHANGE:
			if ( hTarget == pblock() ) {
				int lastUpdateID = pblock()->LastNotifyParamID();
				UpdatePViewUI(lastUpdateID);
			}
			break;
		case kpFlowFuild_RefMsg_NewRand:
			theHold.Begin();
			NewRand();
			theHold.Accept(GetString(IDS_NEWRANDOMSEED));
			return REF_STOP;
		case kpFlowFuild_RefMsg_CleanSim:
			pfluid->clearArray();
			return REF_STOP;
	}

	return REF_SUCCEED;
}

RefTargetHandle PFOperatorFluidGPU::Clone(RemapDir &remap)
{
	PFOperatorFluidGPU* newOp = new PFOperatorFluidGPU();
	newOp->ReplaceReference(0, remap.CloneRef(pblock()));
	BaseClone(this, newOp, remap);
	return newOp;
}

TCHAR* PFOperatorFluidGPU::GetObjectName()
{
	return GetString(IDS_OPERATOR_SIMPLESPEED_OBJECT_NAME);
}

const ParticleChannelMask& PFOperatorFluidGPU::ChannelsUsed(const Interval& time) const
{
								//  read						 &	write channels
	static ParticleChannelMask mask(PCU_New|PCU_Time|PCU_Position|PCU_Amount, PCU_Speed);
	return mask;
}

//+--------------------------------------------------------------------------+
//|							From IPViewItem									 |
//+--------------------------------------------------------------------------+
HBITMAP PFOperatorFluidGPU::GetActivePViewIcon()
{
	if (activeIcon() == NULL)
		_activeIcon() = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_SIMPLESPEED_ACTIVEICON),IMAGE_BITMAP,
									kActionImageWidth, kActionImageHeight, LR_SHARED);
	return activeIcon();
}

//+--------------------------------------------------------------------------+
//|							From IPViewItem									 |
//+--------------------------------------------------------------------------+
HBITMAP PFOperatorFluidGPU::GetInactivePViewIcon()
{
	if (inactiveIcon() == NULL)
		_inactiveIcon() = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_SIMPLESPEED_INACTIVEICON),IMAGE_BITMAP,
									kActionImageWidth, kActionImageHeight, LR_SHARED);
	return inactiveIcon();
}

//+--------------------------------------------------------------------------+
//|							From IPViewItem									 |
//+--------------------------------------------------------------------------+

bool PFOperatorFluidGPU::Proceed(IObject* pCont, 
									 PreciseTimeValue timeStart, 
									 PreciseTimeValue& timeEnd,
									 Object* pSystem,
									 INode* pNode,
									 INode* actionNode,
									 IPFIntegrator* integrator)
{
	IPFSystem* pfSys = GetPFSystemInterface(pSystem);
	// acquire all necessary channels, create additional if needed
	IParticleChannelNewR* chNew = GetParticleChannelNewRInterface(pCont);
	if(chNew == NULL) return false;
	IParticleChannelPTVR* chTime = GetParticleChannelTimeRInterface(pCont);
	if(chTime == NULL) return false;
	IParticleChannelAmountR* chAmount = GetParticleChannelAmountRInterface(pCont);
	if(chAmount == NULL) return false;
	// the position channel may not be present. For some option configurations it is okay
	IParticleChannelPoint3R* chPos = GetParticleChannelPositionRInterface(pCont);

	IChannelContainer* chCont;
	chCont = GetChannelContainerInterface(pCont);
	if (chCont == NULL) return false;

	// the channel of interest
	bool initSpeed = false;
	IParticleChannelPoint3W* chSpeed = (IParticleChannelPoint3W*)chCont->EnsureInterface(PARTICLECHANNELSPEEDW_INTERFACE,
																			ParticleChannelPoint3_Class_ID,
																			true, PARTICLECHANNELSPEEDR_INTERFACE,
																			PARTICLECHANNELSPEEDW_INTERFACE, true,
																			actionNode, (Object*)NULL, &initSpeed);
	IParticleChannelPoint3R* chSpeedR = GetParticleChannelSpeedRInterface(pCont);

	IParticleChannelFloatW* chFloatW =(IParticleChannelFloatW*)chCont->EnsureInterface(PARTICLECHANNELFLOATW_INTERFACE,
																			ParticleChannelFloat_Class_ID,
																			true, PARTICLECHANNELFLOATR_INTERFACE,
																			PARTICLECHANNELFLOATW_INTERFACE, true,
																			actionNode, (Object*)NULL, &initSpeed);
	IParticleChannelFloatR*  chFloatR= (IParticleChannelFloatR*)chCont->EnsureInterface(PARTICLECHANNELFLOATR_INTERFACE,
																			ParticleChannelFloat_Class_ID,
																			true, PARTICLECHANNELFLOATR_INTERFACE,
																			PARTICLECHANNELFLOATW_INTERFACE, true,
																			actionNode, (Object*)NULL, &initSpeed);
	IParticleChannelPoint3W* chPoint3W=(IParticleChannelPoint3W*)chCont->EnsureInterface(PARTICLECHANNELPOINT3W_INTERFACE,
																			ParticleChannelPoint3_Class_ID,
																			true, PARTICLECHANNELPOINT3R_INTERFACE,
																			PARTICLECHANNELPOINT3W_INTERFACE, true,
																			actionNode, (Object*)NULL, &initSpeed);
	IParticleChannelPoint3R*  chPoint3R= (IParticleChannelPoint3R*)chCont->EnsureInterface(PARTICLECHANNELPOINT3R_INTERFACE,
																			ParticleChannelPoint3_Class_ID,
																			true, PARTICLECHANNELPOINT3R_INTERFACE,
																			PARTICLECHANNELPOINT3W_INTERFACE, true,
																			actionNode, (Object*)NULL, &initSpeed);

	if ((chSpeed == NULL) || (chSpeedR == NULL) || (chFloatR == NULL)||(chFloatW == NULL)) return false;
	float fUPFScale = 1.0f/TIME_TICKSPERSEC; // conversion units per seconds to units per tick

	Point3 pt3SpeedVec;
	Point3 pPos,relPos;
	float gridSize,pTemp;
	float pfVisc=_pblock()->GetFloat(kpFlowFuild_viscosity, timeStart);
	float pfDiff=_pblock()->GetFloat(kpFlowFuild_diffusion, timeStart);
	float pfTemp=_pblock()->GetFloat(kpFlowFuild_temperature, timeStart);
	RandGenerator* prg = randLinker().GetRandGenerator(pCont);
	int iQuant = chAmount->Count();
	bool wasIgnoringEmitterTMChange = IsIgnoringEmitterTMChange();
	if (!wasIgnoringEmitterTMChange) SetIgnoreEmitterTMChange();
	Tab<float> emitterDimensions;
	for(int i = 0; i < iQuant; i++) {
		TimeValue tv = chTime->GetValue(i).TimeValue();
		pfSys->GetEmitterDimensions(tv, emitterDimensions);
		Matrix3 nodeTM = pNode->GetObjectTM(tv);
		pPos=chPos->GetValue(i);
		relPos=GetGridPos(pPos,emitterDimensions[1],pfluid->getDim());
		gridSize=emitterDimensions[1]/pfluid->getDim();
		float fSpeedParam =GetPFFloat(pblock(), kpFlowFuild_speed, tv);
		float fMassParam =GetPFFloat(pblock(), kpFlowFuild_mass, tv);
		if(chNew->IsNew(i)) { // apply only to new particles
				pfluid->fill_source(relPos.x,relPos.y,relPos.z, fMassParam, 1);
				chFloatW->SetValue(i, pfTemp);
				pt3SpeedVec=chSpeedR->GetValue(i)/float (fUPFScale*gridSize);
				pfluid->fill_source(relPos.x,relPos.y,relPos.z, pt3SpeedVec.x, 2);
				pfluid->fill_source(relPos.x,relPos.y,relPos.z, pt3SpeedVec.y, 3);
				pfluid->fill_source(relPos.x,relPos.y,relPos.z, pt3SpeedVec.z, 4);
				
				//pfluid->fill_source(relPos.x,relPos.y,relPos.z, pfTemp,1);
				float fDiv = GetPFFloat(pblock(), kpFlowFuild_divergence, tv);
				chPoint3W->SetValue(i, DivergeVectorRandom(Normalize(pt3SpeedVec), prg, fDiv));
		}
		else
		{
			pTemp=chFloatR->GetValue(i);
			pTemp=(pTemp>0.0)? pTemp/2.0:0.0;
			pfluid->fill_source(relPos.x,relPos.y,relPos.z, 5.0*pTemp/float (100.0*gridSize), 5);
			chFloatW->SetValue(i, pTemp);
			pt3SpeedVec.x=pfluid->load_source(relPos.x,relPos.y,relPos.z,2);
			pt3SpeedVec.y=pfluid->load_source(relPos.x,relPos.y,relPos.z,3);
			pt3SpeedVec.z=pfluid->load_source(relPos.x,relPos.y,relPos.z,4);
			//pt3SpeedVec+=chPoint3R->GetValue(i);
		}
		pt3SpeedVec=pt3SpeedVec*float(fSpeedParam*gridSize*fUPFScale*30.0);
		chSpeed->SetValue(i, pt3SpeedVec);
	}
	pfluid->do_sim(pfVisc,pfDiff,1.0/30.0);
	return true;
}

Point3 PFOperatorFluidGPU::GetGridPos(Point3 ParPos,  float xdim, int gSize)
{
	Point3 norPos=(ParPos+Point3(0.5*xdim,0.5*xdim,1.0))/(xdim/gSize);
	int newx= norPos.x<gSize?abs(norPos.x):gSize+1;
	newx= newx>1?newx:0;
	int newy= norPos.y<gSize?abs(norPos.y):gSize+1;
	newy= newy>1?newy:0;
	int newz= norPos.z<gSize?abs(norPos.z):gSize+1;
	newz= newz>1?newz:0;
	return Point3(newx,newy,newz);
}

ClassDesc* PFOperatorFluidGPU::GetClassDesc() const
{
	return GetPFOperatorSimpleSpeedDesc();
}

int PFOperatorFluidGPU::GetRand()
{
	return pblock()->GetInt(kpFlowFuild_seed);
}

void PFOperatorFluidGPU::SetRand(int seed)
{
	_pblock()->SetValue(kpFlowFuild_seed, 0, seed);
}

} // end of namespace PFActions