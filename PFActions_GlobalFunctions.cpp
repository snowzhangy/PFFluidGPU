/**********************************************************************
 *<
	FILE:			PFActions_GlobalFunctions.cpp

	DESCRIPTION:	Collection of useful functions (definition).
											 
	CREATED BY:		Oleg Bayborodin

	HISTORY:		10-22-01

 *>	Copyright (c) 2001, All Rights Reserved.
 **********************************************************************/


#include "PFActions_GlobalFunctions.h"
#include "PFActions_GlobalVariables.h"
#include "IPViewManager.h"
#include "IPFSystem.h"

#include "meshadj.h"
#include "macrorec.h"

namespace PFActions {

TCHAR* GetString(int id)
{
	enum { kBufSize = 1024 };
	static TCHAR buf1[kBufSize], buf2[kBufSize], 
				 buf3[kBufSize], buf4[kBufSize];
	static TCHAR* bp[4] = {buf1, buf2, buf3, buf4};
	static int active = 0;
	TCHAR* result = NULL;
	if (hInstance)
		result = LoadString(hInstance, id, bp[active], sizeof(buf1)) ? bp[active] : NULL;
	active = (active+1)%4; // twiddle between buffers to help multi-getstring users (up to 4)
	return result;
}

// generate random point of the surface of sphere with radius 1
Point3 RandSphereSurface(RandGenerator* randGen)
{
	float theta = randGen->Rand01() * TWOPI;
	float z = randGen->Rand11();
	float r = sqrt(1 - z*z);
	float x = r * cos(theta);
	float y = r * sin(theta);
	return Point3( x, y, z);
}

// generate a random vector with the same length as the given vector but direction
// differs for no more than maxAngle
Point3 DivergeVectorRandom(Point3 vec, RandGenerator* randGen, float maxAngle)
{
	float lenSq = LengthSquared(vec);
	if (lenSq <= 0) return vec;

	if (maxAngle <= 0.0f)
		return vec; // no divergence

	Point3 p3RS = RandSphereSurface(randGen);
	Point3 p3Orth = p3RS - vec * DotProd(vec, p3RS) / lenSq;
	while (LengthSquared(p3Orth) <= 0.0f) {
		p3RS = RandSphereSurface(randGen);
		p3Orth = p3RS - vec * DotProd(vec, p3RS) / lenSq;
	}
	Point3 p3Norm = Length(vec) * Normalize(p3Orth);
//	float fh = 1.0f - sin(maxAngle) * randGen->Rand01();
	float fh = cos(maxAngle * randGen->Rand01());
	float fq = sqrt(1.0f - fh * fh);
	return (fh*vec + fq*p3Norm);
}

// create matrix for speed space. Axis X is the same as the speed vector, axis Z is
// perp to axis x and the most upward
Matrix3 SpeedSpaceMatrix(Point3 speedVec)
{
	if (LengthSquared(speedVec) > 0.0f) {
		speedVec.Unify();
		float xComp = sqrt(1.0f - speedVec.z*speedVec.z);
		Point3 z;
		if (xComp > 0.0f) {
			z = (speedVec.z < 0.0f) ? speedVec : -speedVec;
			z *= (float)fabs(speedVec.z/xComp);
			z.z = xComp;
		} else {
			z = Point3::XAxis;
		}
		Point3 y = CrossProd(z, speedVec);
		return Matrix3( speedVec, y, z, Point3::Origin);
	}
	return Matrix3(Point3::XAxis, Point3::YAxis, Point3::ZAxis, Point3::Origin);
}

BOOL IsControlAnimated(Control* control)
{
	if (control == NULL) return FALSE;
	return control->IsAnimated();
}


ClassDesc* GetPFOperatorSimpleSpeedDesc()
{ return &ThePFOperatorSimpleSpeedDesc; }


INode* GetHighestClosedGroupNode(INode *iNode)
{
	if (iNode == NULL) return NULL;

	INode *node = iNode;
	while (node->IsGroupMember()) {
		node = node->GetParentNode();
		while (!(node->IsGroupHead())) node = node->GetParentNode();
		if (!(node->IsOpenGroupHead())) iNode = node;
	}
	return iNode;
}

bool GetHighestClosedGroupNode(PreciseTimeValue time, Object* pSystem)
{
	bool exactStep = false;
	IPFSystem* iSystem = PFSystemInterface(pSystem);
	if (iSystem == NULL) return false;
	TimeValue stepSize = iSystem->GetIntegrationStep();
	if (time.tick % stepSize == 0) {
		if (fabs(time.fraction) < 0.0001) exactStep = true;
	}
	return exactStep;
}

BOOL IsGEOM(Object *obj)
{ if (obj!=NULL) 
  { if (GetPFObject(obj)->IsParticleSystem()) return FALSE;
    if (obj->SuperClassID()==GEOMOBJECT_CLASS_ID)
    { if (obj->IsSubClassOf(triObjectClassID)) 
        return TRUE;
      else 
	  { if (obj->CanConvertToType(triObjectClassID)) 
	  	return TRUE;			
	  }
	}
  }
  return FALSE;
}

BOOL GeomObjectValidatorClass::Validate(PB2Value &v) 
{
	if (action == NULL) return FALSE;
	INode *node = (INode*) v.r;
	if (node == NULL) return FALSE;
	if (node->TestForLoop(FOREVER,action)!=REF_SUCCEED) return FALSE;

	// check if the item is already included
	if (paramID >= 0) {
		IParamBlock2* pblock = action->GetParamBlock(0);
		if (pblock != NULL) {
			int numItems = pblock->Count(paramID);
			for(int i=0; i<numItems; i++)
				if (pblock->GetINode(paramID, 0, i) == node)
					return FALSE;
		}
	}

	if (node->IsGroupMember())		// get highest closed group node
		node = GetHighestClosedGroupNode(node);
	v.r = node;

	Tab<INode*> stack;
	stack.Append(1, &node, 10);
	TimeValue time = GetCOREInterface()->GetTime();
	while (stack.Count())
	{
		INode* iNode = stack[stack.Count()-1];
		stack.Delete(stack.Count()-1, 1);

		Object* obj = iNode->EvalWorldState(time).obj;
//		if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) // <-- picks up splines which is not good
		if (IsGEOM(obj))
			return TRUE;

		// add children to the stack
		for (int i=0; i<iNode->NumberOfChildren(); i++) {
			INode *childNode = iNode->GetChildNode(i);
			if (childNode) stack.Append(1, &childNode, 10);
		}
	}

	return FALSE;
};


// fast measurement how close two vectors are (angle)
// the higher the value the closer two vectors are (1 is maximum, -1 is minimum)
float CustomDeviation(const Point3& pointDif, const Point3& faceNormal)
{
	double fn2 = double(faceNormal.x*faceNormal.x) + double(faceNormal.y*faceNormal.y) + double(faceNormal.z*faceNormal.z);
	if (fn2 <= 0.0) return -1.0f;
	double pd2 = double(pointDif.x*pointDif.x) + double(pointDif.y*pointDif.y) + double(pointDif.z*pointDif.z);
	if (pd2 <= 0.0) return 1.0f;
	float pf2 = double(pointDif.x*faceNormal.x) + double(pointDif.y*faceNormal.y) + double(pointDif.z*faceNormal.z);
	if (pf2 < 0) return (-(pf2*pf2)/(fn2*pd2));
	return float((pf2*pf2)/(fn2*pd2));
}


bool VerifyEmittersMXSSync(IParamBlock2* pblock, int kRealRefObjectsID, int kMXSRefObjectsID)
{
	if (pblock == NULL) return true;
	int count = pblock->Count(kRealRefObjectsID);
	if (count != pblock->Count(kMXSRefObjectsID)) return false;
	for(int i=0; i<count; i++) {
		if (pblock->GetINode(kRealRefObjectsID, 0, i) != 
			pblock->GetINode(kMXSRefObjectsID, 0, i)) 
			return false;
	}
	return true;
}

bool UpdateFromRealRefObjects(IParamBlock2* pblock, int kRealRefObjectsID, int kMXSRefObjectsID, 
							  bool& gUpdateInProgress)
{
	if (pblock == NULL) return false;
	if (gUpdateInProgress) return false;
	if (VerifyEmittersMXSSync(pblock, kRealRefObjectsID, kMXSRefObjectsID)) return false;
	gUpdateInProgress = true;
	pblock->ZeroCount(kMXSRefObjectsID);
	for(int i=0; i<pblock->Count(kRealRefObjectsID); i++) {
		INode* refNode = pblock->GetINode(kRealRefObjectsID, 0, i);
		pblock->Append(kMXSRefObjectsID, 1, &refNode);
	}
	gUpdateInProgress = false;
	return true;
}

bool UpdateFromMXSRefObjects(IParamBlock2* pblock, int kRealRefObjectsID, int kMXSRefObjectsID, 
							 bool& gUpdateInProgress, HitByNameDlgCallback* callback)
{
	if (pblock == NULL) return false;
	if (gUpdateInProgress) return false;
	if (VerifyEmittersMXSSync(pblock, kRealRefObjectsID, kMXSRefObjectsID)) return false;
	gUpdateInProgress = true;
	pblock->ZeroCount(kRealRefObjectsID);
	for(int i=0; i<pblock->Count(kMXSRefObjectsID); i++) {
		INode* node = pblock->GetINode(kMXSRefObjectsID, 0, i);
		if (node == NULL) {
			pblock->Append(kRealRefObjectsID, 1, &node);
		} else {
			if (callback->filter(node) == TRUE) {
				pblock->Append(kRealRefObjectsID, 1, &node);
			} else {
				node = NULL;
				pblock->Append(kRealRefObjectsID, 1, &node);
				pblock->SetValue(kMXSRefObjectsID, 0, node, i);
			}
		}
	}
	gUpdateInProgress = false;
	return true;
}

void MacrorecObjects(ReferenceTarget* rtarg, IParamBlock2* pblock, int paramID, TCHAR* paramName)
{
	if ((pblock == NULL) || (rtarg == NULL)) return;
	TSTR selItemNames = _T("(");
	int numItems = pblock->Count(paramID);
	bool nonFirst = false;
	for(int i=0; i<numItems; i++) {
		INode* selNode = pblock->GetINode(paramID, 0, i);
		if (selNode == NULL) continue;
		if (nonFirst) selItemNames = selItemNames + _T(", ");
		selItemNames = selItemNames + _T("$'");
		selItemNames = selItemNames + selNode->GetName();
		selItemNames = selItemNames + _T("'");
		nonFirst = true;
	}
	selItemNames = selItemNames + _T(")");
	macroRec->EmitScript();
	macroRec->SetProperty(rtarg, paramName, mr_name, selItemNames.data());
}

} // end of namespace PFActions

