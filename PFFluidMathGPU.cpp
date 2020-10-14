/*
the actual fluid calculation class
*/

#include "PFFluidMathGPU.h"
namespace PFActions {

PFFluidMathGPU::PFFluidMathGPU(){
}
PFFluidMathGPU::~PFFluidMathGPU(){
}
void PFFluidMathGPU::clearArray(){
	for (int i=0;i<size;i++) 
	{
		dens[i] = u[i] = v[i]  =w[i] = dens_prev[i] = u_prev[i]=v_prev[i]=w_prev[i] = 0.0f;
	}
}
void PFFluidMathGPU::Update( float timestep, float fInputVorticityConfinementScale, float fInputDensityDecay, float fInputDensityViscosity, float fInputFluidViscosity, float fInputImpulseSize, float InputRandCenterScale)
{
    // emit position and bounding box
    m_vImpulsePos.x = float(m_pGrid->GetDimX()) * 0.1f;
    m_vImpulsePos.y = float(m_pGrid->GetDimY()) * 0.9f;
    m_vImpulsePos.z = float(m_pGrid->GetDimZ()) * 0.25f;
    float impulseStrength = 0.8f;
    m_vImpulseDir.x = 0.5f * impulseStrength;
    m_vImpulseDir.y = -0.5f * impulseStrength;
    m_vImpulseDir.z = 0.8f * impulseStrength;

    m_vObstBoxVelocity = (m_vObstBoxPos - m_vObstBoxPrevPos) / timestep;
    m_vObstBoxVelocity *= 1.5f;
    m_vObstBoxVelocity.x *= m_pGrid->GetDimX(); m_vObstBoxVelocity.y *= m_pGrid->GetDimY(); m_vObstBoxVelocity.z *= m_pGrid->GetDimZ();
    m_evObstBoxVelocity->SetFloatVector(m_vObstBoxVelocity);
    m_vObstBoxPrevPos = m_vObstBoxPos;
	///////////////////////////////////
	//parameter setup
	m_fVorticityConfinementScale = fInputVorticityConfinementScale;
	m_fDensityDecay = fInputDensityDecay;
	m_fDensityViscosity = fInputDensityViscosity;
	m_fFluidViscosity =  fInputFluidViscosity;
    if(m_eFluidType == FT_FIRE)
    {
        m_fVorticityConfinementScale = 0.03f;
        m_fDensityDecay = 0.9995f;
    }
    else if( m_eFluidType == FT_LIQUID )
	{
		 m_fDensityDecay = 1.0f;
		 m_fDensityViscosity = 0.0f;
	}

    // we want the rate of decay to be the same with any timestep (since we tweaked with timestep = 2.0 we use 2.0 here)
    double dDensityDecay = pow((double)m_fDensityDecay, (double)(timestep/2.0));
    float fDensityDecay = (float) dDensityDecay;

    m_evFluidType->SetInt( m_eFluidType );

    // Advection of Density or Level set
    bool bAdvectAsTemperature = (m_eFluidType == FT_FIRE);
    Advect(  timestep, 1.0f, false, bAdvectAsTemperature, RENDER_TARGET_TEMPVECTOR, currentSrcVelocity, m_currentSrcScalar, RENDER_TARGET_OBSTACLES, m_currentSrcScalar );
    Advect( -timestep, 1.0f, false, bAdvectAsTemperature, RENDER_TARGET_TEMPSCALAR, currentSrcVelocity, RENDER_TARGET_TEMPVECTOR, RENDER_TARGET_OBSTACLES, m_currentSrcScalar );
    AdvectMACCORMACK( timestep, fDensityDecay, false, bAdvectAsTemperature, m_currentDstScalar, currentSrcVelocity, m_currentSrcScalar, RENDER_TARGET_TEMPSCALAR, RENDER_TARGET_OBSTACLES, m_currentSrcScalar );

    // Advection of Velocity 
    bool bAdvectAsLiquidVelocity = (m_eFluidType == FT_LIQUID) && m_bTreatAsLiquidVelocity;
    Advect(  timestep, 1.0f, bAdvectAsLiquidVelocity, false, RENDER_TARGET_TEMPVECTOR, currentSrcVelocity, currentSrcVelocity, RENDER_TARGET_OBSTACLES, m_currentDstScalar );
    Advect( -timestep, 1.0f, bAdvectAsLiquidVelocity, false, RENDER_TARGET_TEMPVECTOR1, currentSrcVelocity, RENDER_TARGET_TEMPVECTOR, RENDER_TARGET_OBSTACLES, m_currentDstScalar );
    AdvectMACCORMACK( timestep, 1.0f, bAdvectAsLiquidVelocity, false, currentDstVelocity, currentSrcVelocity, currentSrcVelocity, RENDER_TARGET_TEMPVECTOR1, RENDER_TARGET_OBSTACLES, m_currentDstScalar );


    // Diffusion of Density for smoke
    if( (m_fDensityViscosity > 0.0f) && (m_eFluidType == FT_SMOKE) )
    {
        swap(m_currentDstScalar, m_currentSrcScalar);
        Diffuse( timestep, m_fDensityViscosity, m_currentDstScalar, m_currentSrcScalar, RENDER_TARGET_TEMPSCALAR, RENDER_TARGET_OBSTACLES );
    }
    // Diffusion of Velocity field
    if( m_fFluidViscosity > 0.0f )
    {
        swap(currentDstVelocity, currentSrcVelocity);
        Diffuse( timestep, m_fFluidViscosity, currentDstVelocity, currentSrcVelocity, RENDER_TARGET_TEMPVECTOR, RENDER_TARGET_OBSTACLES );
    }
    // Vorticity confinement
    if( m_fVorticityConfinementScale > 0.0f )
    {
        ComputeVorticity( RENDER_TARGET_TEMPVECTOR, currentDstVelocity);
        ApplyVorticityConfinement( timestep, currentDstVelocity, RENDER_TARGET_TEMPVECTOR, m_currentDstScalar, RENDER_TARGET_OBSTACLES);
    }

    // TODO: this is not really working very well, some flickering occurrs for timesteps less than 2.0
    // Addition of external density/liquid and external forces
    static float elapsedTimeSinceLastUpdate = 2.0f;
    if( elapsedTimeSinceLastUpdate >= 2.0f )
    {
        AddNewMatter( timestep, m_currentDstScalar, RENDER_TARGET_OBSTACLES );   
        if(bUseSuppliedParameters)
			AddExternalForces( timestep, currentDstVelocity, RENDER_TARGET_OBSTACLES, m_currentDstScalar,fInputImpulseSize, InputRandCenterScale );
		else
			AddExternalForces( timestep, currentDstVelocity, RENDER_TARGET_OBSTACLES, m_currentDstScalar,m_fImpulseSize, 1 );

        elapsedTimeSinceLastUpdate = 0.0f;
    }
    elapsedTimeSinceLastUpdate += timestep;

    // Pressure projection
    ComputeVelocityDivergence( RENDER_TARGET_TEMPVECTOR, currentDstVelocity, RENDER_TARGET_OBSTACLES, RENDER_TARGET_OBSTVELOCITY);
    ComputePressure( RENDER_TARGET_PRESSURE, RENDER_TARGET_TEMPSCALAR, RENDER_TARGET_TEMPVECTOR, RENDER_TARGET_OBSTACLES, m_currentDstScalar);
    ProjectVelocity( currentSrcVelocity, RENDER_TARGET_PRESSURE, currentDstVelocity, RENDER_TARGET_OBSTACLES, RENDER_TARGET_OBSTVELOCITY, m_currentDstScalar);

    if( m_eFluidType == FT_LIQUID )
    {
        RedistanceLevelSet(m_currentSrcScalar, m_currentDstScalar, RENDER_TARGET_TEMPSCALAR);
        ExtrapolateVelocity(currentSrcVelocity, RENDER_TARGET_TEMPVECTOR, RENDER_TARGET_OBSTACLES, m_currentSrcScalar);
    }
    else
    {
        // Swap scalar textures (we ping-pong between them for advection purposes)
        swap(m_currentDstScalar, m_currentSrcScalar);
    }

}

} //end of namespace