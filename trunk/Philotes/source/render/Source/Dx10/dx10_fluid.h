//----------------------------------------------------------------------------
// dx10_fluid.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#define USE_OBSTACLE_TEXTURE
#define USE_GAUSSIAN_BLOCKER

#include <fstream>

#define MAX_INTERACTING_UNITS 50

PRESULT dx10_AddFluidRTDescs();

//temp: untill dxC_VertexDeclarations.h starts producing these automatically
struct VS_FLUID_SIMULATION_INPUT_STRUCT
{
    D3DXVECTOR2 Position;   //: POSITION;
	D3DXVECTOR3 Tex0;       //: TEXCOORD0;
    D3DXVECTOR4 Tex1;       //: TEXCOORD1;
	D3DXVECTOR4 Tex2;       //: TEXCOORD2;
	D3DXVECTOR4 Tex3;       //: TEXCOORD3;
	D3DXVECTOR3 Tex7;       //: TEXCOORD7;
};

struct VS_FLUID_RENDERING_INPUT_STRUCT
{
    D3DXVECTOR4 Position;
    D3DXVECTOR3 Tex; 
};


enum FluidField
{
  fluidColor,
  fluidVelocity,
  fluidTemp1,
  fluidPressure,
  fluidTemp3,
  fluidTempVelocity,
};



class CFluid;
class VolumeRenderer;


class Grid
{
   public:
       Grid( void ) {};  
      ~Grid( void );

      void Initialize( int gridWidth, int gridHeight, int gridDepth );

      void createTexture( int& texture );

	  void drawSlice         ( D3D_EFFECT* pEffect, UINT iPassIndex, int sliceNumber, int numSlices = 1);
      void drawSlices        ( D3D_EFFECT* pEffect, UINT iPassIndex  );
      void drawForRender     ( D3D_EFFECT* pEffect, UINT iPassIndex  );
	  
 
#ifdef USE_OBSTACLE_TEXTURE
      void drawBoundaries    ( D3D_EFFECT* pEffect, UINT iPassIndex, bool forceDraw = false );
#else
      void drawBoundaries    ( D3D_EFFECT* pEffect, UINT iPassIndex, bool forceDraw = false );
#endif


      int dim[3];
      int texDim[2];
      int rows, cols;
      int maxDim;

      //ID3D10Buffer* renderQuadBuffer;
	  //ID3D10Buffer* slicesBuffer;
	  //ID3D10Buffer* boundarySlicesBuffer;
	  //ID3D10Buffer* boundaryLinesBuffer;
	  //ID3D10Buffer* boundaryPointsBuffer;
	  
	  D3D_VERTEX_BUFFER_DEFINITION renderQuadBufferDesc;
	  D3D_VERTEX_BUFFER_DEFINITION slicesBufferDesc;
	  D3D_VERTEX_BUFFER_DEFINITION boundarySlicesBufferDesc;
	  D3D_VERTEX_BUFFER_DEFINITION boundaryLinesBufferDesc;
	  D3D_VERTEX_BUFFER_DEFINITION boundaryPointsBufferDesc;


      VS_FLUID_SIMULATION_INPUT_STRUCT* renderQuad;
	  VS_FLUID_SIMULATION_INPUT_STRUCT* slices;
	  VS_FLUID_SIMULATION_INPUT_STRUCT* boundarySlices;
	  VS_FLUID_SIMULATION_INPUT_STRUCT* boundaryLines;
	  VS_FLUID_SIMULATION_INPUT_STRUCT* boundaryPoints;
   
	  int numVerticesRenderQuad;
	  int numVerticesSlices;
	  int numVerticesBoundarySlices;
	  int numVerticesBoundaryLines;
	  int numVerticesBoundaryPoints;

	  void createRenderQuad(VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index );
      void createBoundaryPoints(VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index );
      void createBoundaryLines( VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index );
      void createBoundaryQuads( VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index );
      void createSlice       ( int z, int texZOffset, VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index);
      void createPoint       ( float x, float y, int z,
                               float texXOffset,
                               float texYOffset,
                               int   texZOffset, VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index );
      void createLine        ( float x1, float y1,
                               float x2, float y2, int z,
                               float texXOffset,
                               float texYOffset,
                               int   texZOffset, VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index);
      void createSliceQuads  ( VS_FLUID_SIMULATION_INPUT_STRUCT** vertices, int& index );
      void drawSliceQuadsBoundaries( void );
      void createVertexBuffers(  );

};





class CFluid
{

  public:
    CFluid( void ) {};  
    ~CFluid( void );

	void Initialize( int width, int height, int depth, 
		int nDensityTextureId, int nVelocityTextureId, int nObstructorTextureId,
		int densityTextureIndex, int velocityTextureIndex, int obstructorTextureIndex, 	
		float renderScale,D3DXMATRIX* psysWorld, //VECTOR vPosition,
		D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique  );

    void update( D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique, VECTOR focusUnitPosition, float focusUnitRadius, float focusUnitHeight, VECTOR windDir, float windAmount, float windInfluence, float timestep, float velocityMultiplier, float velocityClamp, float sizeMultiplier, bool useBFECC, VECTOR* monsterPositions, VECTOR* monsterHeightAndRadiusAndSpeed, VECTOR* monsterDirection );
    void render3D(  FluidField field, D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique, D3DXMATRIX *View, D3DXMATRIX *Projection, D3DXMATRIX * psysWorld, VECTOR vOffsetPosition, float color[]);
    void draw( FluidField field, D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique );
    void reset( void );
    void addFocusUnits( VECTOR focusUnitPosition, float focusUnitRadius, float focusUnitHeight, VECTOR focusUnitDirection, float focusUnitSpeed, float velocityMultiplier, float velocityClamp, float sizeMultiplier, D3DXMATRIX worldInv, D3DXMATRIX Scale,
		 			    D3DXVECTOR4* FocusUnitPositionsInGrid, D3DXVECTOR4* FocusUnitHeightAndRadiusInGrid, D3DXVECTOR4* focusUnitVelocities, int& numFocusUnits );

    void GetCenterAndRadius(VECTOR &center, float &radius);


    int textureWidth( void );
    int textureHeight( void );


    int setCylinderBlocker( D3DXVECTOR3& center, float height, float radius );
    void setFluidType( );

  private:
    void advectVelocity           (  D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique );
    void advectColorBFECC         (  D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique );
	void advectColor              (  D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique );
#ifdef USE_GAUSSIAN_BLOCKER
    void applyBlocker(  D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique, float maxHeight);
#endif
    void applyExternalForces      ( D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique, D3DXVECTOR4 windDir, float windAmount );
    void applyVorticityConfinement( D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique  );
    void computeVelocityDivergence(  D3D_EFFECT* pEffect );
    void computePressure          (  D3D_EFFECT* pEffect );
    void projectVelocity          (  D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique );

    void createOffsetTable( D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique );
    void updateDisplayTexture( FluidField field  );
	void renderObstacles( D3D_EFFECT* pEffect, int nGridObstructorTextureIndex );

    bool NVAPICheck();


	int ColorTextureNumber; //important! for pingponging color texture

    //for textures
	bool firstUpdate;
	int velocityPatternTextureId;
	int densityPatternTextureId;
	int obstacleTextureId;
	int obstructorTextureIndex;

	int frame;
	int numGPUs;

    float saturation;
    float decay;
    bool emit;
    bool addDensity;  //whether to add smoke density or just velocity
	bool mustRenderObstacles;

	float smokeColor[4];
	int     nGridDensityTextureIndex;
	int 	nGridVelocityTextureIndex;
	float windModifier;

    bool lastPositionInitialized;
    VECTOR lastPosition;
   
    static const UINT passIndexAdvect1 =            0; //Advect1 //0
	static const UINT passIndexAdvect2 =            1; //Advect2 //1
    static const UINT passIndexAdvectBFECC =        2;//AdvectBFECC //2
    static const UINT passIndexAdvectVel =          3;//AdvectVel //3
	static const UINT passIndexVorticity =          4;//Vorticity //4
	static const UINT passIndexConfinementBFECC =   5;//ConfinementBFECC //5
	static const UINT passIndexGaussian =           6;//Gaussian //6
	static const UINT passIndexDivergence =         7;//Divergence //7
	static const UINT passIndexJacobi1 =            8;//Jacobi1 //8
	static const UINT passIndexJacobi2 =            9;//Jacobi2 //9
	static const UINT passIndexProject =           10;//Project ;10
	//DiffDivergence //11
	static const UINT passIndexDrawTexture =       12; //DrawTexture //12
	static const UINT passIndexDrawVelocity =      13;//DrawVelocity //13
	static const UINT passIndexDrawWhite =         14;//DrawWhite //14
	static const UINT passIndexConfinementNormal = 15;//ConfinementNormal //15
	static const UINT passIndexGaussianFlat =      23; 
	static const UINT passIndexDensityTexture =    20;
	static const UINT passIndexVelocityTexture =   29; //29

    static const UINT passIndexVorticity_Obstacles  = 24;//24
    static const UINT passIndexDivergence_Obstacles = 25; //25
    static const UINT passIndexJacobi1_Obstacles    = 26;  //26
    static const UINT passIndexJacobi2_Obstacles    = 27; //27
    static const UINT passIndexProject_Obstacles    = 28; //28 
	//30
	static const UINT passIndexDrawObstacles        = 31; //draw obstacles into obstacle texture 
    
    RENDER_TARGET_INDEX RENDER_TARGET_FLUID_VELOCITY;
    RENDER_TARGET_INDEX RENDER_TARGET_FLUID_COLOR0;
    RENDER_TARGET_INDEX RENDER_TARGET_FLUID_COLOR1;
    RENDER_TARGET_INDEX RENDER_TARGET_FLUID_PRESSURE;
	RENDER_TARGET_INDEX RENDER_TARGET_FLUID_OBSTACLES;

    float impulseX, impulseY, impulseZ;
    float impulseDx, impulseDy, impulseDz;


    int renderW, renderH;

    //offset table
	float* texels;
	SPD3DCSHADERRESOURCEVIEW textureRviewOffset;
	SPD3DCTEXTURE2D textureOffsetTable;
    
	Grid *grid;
    VolumeRenderer* renderer;

};



//does a volume rendering of a box using specified voxel data
class VolumeRenderer
{
public:
    
	VolumeRenderer( void );  
	~VolumeRenderer( void );


	void Initialize(int gridWidth, int gridHeight, int gridDepth,
                    float inputRenderScale, D3DXMATRIX * psysWorld,
					D3D_EFFECT* pEffect, EFFECT_TECHNIQUE* pTechnique);

    void draw( D3D_EFFECT* pEffect,  EFFECT_TECHNIQUE* pTechnique , D3DXMATRIX *View, D3DXMATRIX *Projection);

	
    D3DXMATRIX World; 
    float mRenderScale;
    float timeIncrement;
    float sizeMultiplier;

    void UpdateWorldMatrix(D3DXMATRIX * psysWorld, VECTOR vOffsetPosition);

private:
    void drawBoxDepth( D3D_EFFECT* pEffect, UINT iPassIndex );
    void drawScreenQuad(D3D_EFFECT* pEffect, UINT iPassIndex );
    void CalculateRayCastSizes();
    void UpdateTexturesAndRenderTargets(); 

    int gridDim[3];
    float maxDim;
    int RayCastWidth;
    int RayCastHeight;
    int BackBufferWidth;
    int BackBufferHeight;

    void createBox (void);
    void createGridBox (void);
    void createScreenQuad(void);

    RENDER_TARGET_INDEX RENDER_TARGET_RAYCAST;
    RENDER_TARGET_INDEX RENDER_TARGET_EDGEDATA;
    RENDER_TARGET_INDEX RENDER_TARGET_RAYDATASMALL;

	D3D_VERTEX_BUFFER_DEFINITION  BoxVertexBufferDesc;
    D3D_VERTEX_BUFFER_DEFINITION  GridBoxVertexBufferDesc;
    D3D_VERTEX_BUFFER_DEFINITION  ScreenQuadVertexBufferDesc;
	D3D_INDEX_BUFFER_DEFINITION   GridBoxIndexBufferDesc;
    D3D_INDEX_BUFFER_DEFINITION   BoxIndexBufferDesc;

	static const UINT passIndexRayDataFront                 = 16;
	static const UINT passIndexRayDataBack                  = 17;
    static const UINT passIndexQuadDownSampleRayDataTexture = 18;
    static const UINT passIndexQuadRaycast                  = 22;  
    static const UINT passIndexQuadRaycastCopy              = 21;  
    static const UINT passIndexQuadEdgeDetect               = 30;

};