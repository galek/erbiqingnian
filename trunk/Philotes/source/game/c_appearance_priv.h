#ifndef _DEFINITION_HEADER_INCLUDED_
#include "definition.h"
#endif

#include "c_appearance.h"
#include "unittypes.h"
#include "c_animation.h"
#include "c_attachment.h"

#ifndef _UNITS_H_
#include "units.h"
#endif

#include "e_texture.h"

#ifndef __LOD__
#include "lod.h"	// CHB 2006.11.28
#endif

#define NUM_APPEARANCE_INVENTORY_VIEWS	2
#define DEFAULT_ANIMATION_GROUP			0

#define APPEARANCE_DEFINITION_FLAG_TWO_SIDED					MAKE_MASK(0)
#define APPEARANCE_DEFINITION_FLAG_UNUSED						MAKE_MASK(1)
#define APPEARANCE_DEFINITION_FLAG_TWIST_NECK					MAKE_MASK(2)
#define APPEARANCE_DEFINITION_FLAG_ADD_ALL_ANIMGROUPS			MAKE_MASK(3)
#define APPEARANCE_DEFINITION_FLAG_WARDROBE_USES_INVENTORY		MAKE_MASK(4)
#define APPEARANCE_DEFINITION_FLAG_SCROLL_LIGHTMAPS				MAKE_MASK(5)
#define APPEARANCE_DEFINITION_FLAG_MONITOR_STANCE				MAKE_MASK(6)
#define APPEARANCE_DEFINITION_FLAG_WARDROBE_USES_COLORMASK		MAKE_MASK(7)
#define APPEARANCE_DEFINITION_FLAG_ALWAYS_ADJUST_SCALE			MAKE_MASK(8)
#define APPEARANCE_DEFINITION_FLAG_ALWAYS_PLAY_SUBGROUP_ANIMS	MAKE_MASK(9)
#define APPEARANCE_DEFINITION_FLAG_TURN_AND_POINT				MAKE_MASK(10)
#define APPEARANCE_DEFINITION_FLAG_USES_TEXTURE_OVERRIDES		MAKE_MASK(11)
#define APPEARANCE_DEFINITION_FLAG_DEF_COPY						MAKE_MASK(12)
#define APPEARANCE_DEFINITION_FLAG_LENGTHEN_POS_X_AXIS			MAKE_MASK(13)
#define APPEARANCE_DEFINITION_FLAG_NO_SHADOW					MAKE_MASK(14)
#define APPEARANCE_DEFINITION_FLAG_DONT_DISTANCE_CULL			MAKE_MASK(15)

#define APPEARANCE_DEFINITION_VIEW_FLAG_IS_WEAPON		MAKE_MASK(0)

struct INVENTORY_VIEW_INFO
{
	VECTOR vCamFocus;
	float fCamRotation;
	float fCamPitch;
	float fCamDistance;
	float fCamFOV;
	char pszEnvName[ MAX_XML_STRING_LENGTH ];
	char pszBoneName[ MAX_XML_STRING_LENGTH ];
	int nBone;
};

enum
{
	CAMERA_INVENTORY = 0,			
	CAMERA_HEAD = 1,				
	CAMERA_CHARACTER_SCREEN = 2,	
};

enum
{
	WEAPON_BONE_GENERIC = 0,
	WEAPON_BONE_FOCUS,

	NUM_WEAPON_BONE_TYPES
};

struct APPEARANCE_METRICS
{
	int nSkeletonStepCountPerFrame;
	int nWorldStepCountPerFrame;
	int nSampleAndCombineCountPerFrame;
};

#pragma warning(push)
#pragma warning(disable:4324)	// warning C4324: 'APPEARANCE_DEFINITION' : structure was padded due to __declspec(align())
struct APPEARANCE_DEFINITION 
{ 
	DEFINITION_HEADER tHeader;
	DWORD dwFlags;
	int nInitialized;
	DWORD pdwModeBlendMasks[ DWORD_FLAG_SIZE( MAX_UNITMODES ) ];
	char pszFullPath[ MAX_XML_STRING_LENGTH ];
	char pszModel   [ MAX_XML_STRING_LENGTH ];
	char pszRagdoll [ MAX_XML_STRING_LENGTH ];
	char pszHavokShape [ MAX_XML_STRING_LENGTH ];
	char pszSkeleton[ MAX_XML_STRING_LENGTH ];
#ifdef HAVOKFX_ENABLED
	class hkShape * pHavokShape;
	class hkMeshBinding * pMeshBinding;
#endif
	granny_model_instance * pGrannyInstance;   // Granny's instance of this model 
	struct granny_model * pGrannyModel;
	MATRIX mInverseSkinTransform;
	int nBoneCount;

	char pszNeck		[ MAX_XML_STRING_LENGTH ];
	char pszSpineTop	[ MAX_XML_STRING_LENGTH ];
	char pszSpineBottom	[ MAX_XML_STRING_LENGTH ];
	char pszAimBone  [ MAX_XML_STRING_LENGTH ];
	char pszLeftFootBone [ MAX_XML_STRING_LENGTH ];
	char pszRightFootBone[ MAX_XML_STRING_LENGTH ];
	char pszRightWeaponBone[ NUM_WEAPON_BONE_TYPES ][ MAX_XML_STRING_LENGTH ];
	char pszLeftWeaponBone [ NUM_WEAPON_BONE_TYPES ][ MAX_XML_STRING_LENGTH ];
	VECTOR vAimOffset;
	VECTOR vNeckAim;
	VECTOR pvMuzzleOffset[ MAX_WEAPONS_PER_UNIT ];
	int nNeck;
	int nSpineTop;
	int nSpineBottom;
	int nAimBone;
	int nLeftFootBone;
	int nRightFootBone;
	int pnWeaponBones[ NUM_WEAPON_BONE_TYPES ][ MAX_WEAPONS_PER_UNIT ];
	int nHeadTurnBoneLimitDegrees;
	int nHeadTurnTotalLimitDegrees;
	float fHeadTurnBoneLimit;
	float fHeadTurnTotalLimit;
	float fHeadTurnSpeed;
	float fHeadTurnPercentVsTorso;
	float fTurnSpeed;
	float fSpinSpeed;
	float fMaxRopeBendXY;
	float fMaxRopeBendZ;

	struct INVENTORY_VIEW_INFO * pInventoryViews;
	int nInventoryViews;

	struct granny_file			*pGrannyFile;
	int nWardrobeBaseId;
	int pnWardrobeLayerIds[ MAX_WARDROBE_LAYERS_PER_APPEARANCE ];
	int pnWardrobeLayerParams[ MAX_WARDROBE_LAYERS_PER_APPEARANCE ];
	int nWardrobeAppearanceGroup;

	char pszTextureOverrides[ NUM_TEXTURE_TYPES ][ MAX_XML_STRING_LENGTH ];
	int pnTextureOverrides[ NUM_TEXTURE_TYPES ][ LOD_COUNT ];	// CHB 2006.11.28
	float fScrollLightmapRate[ 2 ];
	float fScrollLightmapTile[ 2 ];

	int	nModelDefId;

	ANIMATION_DEFINITION tInitAnimation;

#ifdef HAVOK_ENABLED
	class hkLoader * pLoader;
#endif

	struct ANIMATION_DEFINITION *	pAnimations;
	int								nAnimationCount;

	struct ANIM_GROUP * pAnimGroups;
	int nAnimGroupCount;

	char * pszBoneNames;
	int nWeightGroupCount;
	char * pszWeightGroups;

	int  nWeightCount; // this is the total number of floats = nWeightGroupCount * nBoneCount
	float * pfWeights;

	int pnWeightGroupTypes[ NUM_WEIGHT_GROUP_TYPES ];

	//float fLODDistance;		// CHB 2006.10.20 - Distance from eye at which low LOD is used.
	float fLODScreensize;

	// preview data - this stuff is only used for the tools
	char pszCopyTemplate[ MAX_XML_STRING_LENGTH ];
	char pszAnimationFilePrefix[ MAX_XML_STRING_LENGTH ];
	DWORD dwViewFlags;
	int nWeaponAnimationGroupId;
	int nViewCameraMode;
	int nGrid_FileNameOffset;
	int nPreviewWardrobeBody;
	int nPreviewHeight;
	int nPreviewWeight;
	float fPreviewScale;
};
#pragma warning(pop)

ANIMATION_DEFINITION * AppearanceDefinitionGetAnimation ( 
	GAME* pGame, 
	int nAppearanceDefId,
	int nAnimationGroup,
	UNITMODE eMode );

int AppearanceDefinitionFilterAnimationGroup( 
	GAME * pGame,
	int nAppearanceDefId, 
	int nAnimationGroup );

VECTOR AppearanceDefinitionGetBoundingBoxOffset(
	int nAppearanceDef );

VECTOR AppearanceDefinitionGetBoundingBoxSize(
	int nAppearanceDef );

const struct BOUNDING_BOX * AppearanceDefinitionGetBoundingBox(
	int nAppearanceDef );

BOOL c_AppearanceCheckOtherAnimsForSameStance( 
	int nAppearanceId, 
	const ANIMATION_DEFINITION * pAnimDef );

void c_AppearanceGetDebugMetrics(
	APPEARANCE_METRICS & tMetrics);

BOOL c_AppearanceIsAdjustingContext(
	int nAppearanceId );
