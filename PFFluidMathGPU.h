/*
the fluid operations class
*/

//#include <ioapi.h>

namespace PFActions {

class PFFluidMathGPU
{
public:
    enum FLUID_TYPE
    {
        FT_SMOKE =0,
        FT_FIRE  =1,
        FT_LIQUID=2
    };
	int GetDimX                 ( void )    { if(!m_pGrid) return 0; return m_pGrid->GetDimX(); };
	int GetDimY                 ( void )    { if(!m_pGrid) return 0; return m_pGrid->GetDimY(); };
	int GetDimZ                 ( void )    { if(!m_pGrid) return 0; return m_pGrid->GetDimZ(); };

	PFFluidMathGPU();
	~PFFluidMathGPU();

private:
	static const int N=20;
	static const int size = (N+2)*(N+2)*(N+2);
	float u[size] , v[size], w[size], u_prev[size], v_prev[size], w_prev[size];
	float dens[size], dens_prev[size];
	//float s[size];

};
} //end of namespace