/**********************************************************************
 *<
	FILE:			PFActions_GlobalFunctions.h

	DESCRIPTION:	Collection of useful functions (declaration).
											 
	CREATED BY:		Oleg Bayborodin

	HISTORY:		created 10-22-01

 *>	Copyright (c) 2001, All Rights Reserved.
 **********************************************************************/

#ifndef _PFACTIONS_GLOBALFUNCTIONS_H_
#define _PFACTIONS_GLOBALFUNCTIONS_H_

#include "max.h"
#include "iparamb2.h"

#include "RandGenerator.h"
#include "PreciseTimeValue.h"

namespace PFActions {

TCHAR* GetString(int id);
Point3 RandSphereSurface(RandGenerator* randGen);
Point3 DivergeVectorRandom(Point3 vec, RandGenerator* randGen, float maxAngle);
Matrix3 SpeedSpaceMatrix(Point3 speedVec);
BOOL IsControlAnimated(Control* control);


ClassDesc* GetPFOperatorSimpleSpeedDesc();


INode* GetHighestClosedGroupNode(INode *iNode);
bool IsExactIntegrationStep(PreciseTimeValue time, Object* pSystem);
void MacrorecObjects(ReferenceTarget* rtarg, IParamBlock2* pblock, int paramID, TCHAR* paramName);


class GeomObjectValidatorClass : public PBValidator
{
	public:
	GeomObjectValidatorClass() { action=NULL; paramID=-1; }
	BOOL Validate(PB2Value &v);

	ReferenceMaker *action;
	int paramID; // parameter id in the paramblock for the list of the included items
				// list of all items that were already included is used to avoid duplicate items
};

enum {	kLocationType_pivot = 0,
		kLocationType_vertex,
		kLocationType_edge,
		kLocationType_surface,
		kLocationType_volume,
		kLocationType_selVertex,
		kLocationType_selEdge,
		kLocationType_selFaces,
		kLocationType_undefined = -1
};


struct LocationPoint
{
	INode* node;
	Point3 location;
	int pointIndex;
	bool init;
};

// apply the same material index to all mesh faces
void ApplyMtlIndex(Mesh* mesh, int mtlIndex);

// utilities for synchronization of real refObject list and mxs refObject list
// update mxs refObjects info on basis of real refObjects info
bool UpdateFromRealRefObjects(IParamBlock2* pblock, int kRealRefObjectsID, int kMXSRefObjectsID, 
							  bool& gUpdateInProgress);
// update real refObjects info on basis of mxs refObjects info
bool UpdateFromMXSRefObjects(IParamBlock2* pblock, int kRealRefObjectsID, int kMXSRefObjectsID, 
							 bool& gUpdateInProgress, HitByNameDlgCallback* callback);

} // end of namespace PFActions

#endif // _PFACTIONS_GLOBALFUNCTIONS_H_