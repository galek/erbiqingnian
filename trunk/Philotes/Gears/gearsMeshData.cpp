
#include "gearsMeshData.h"

_NAMESPACE_BEGIN

MeshVertex::MeshVertex( void )
{
	mPos	= Vector3::ZERO;
	mNormal = Vector3::ZERO;
	mColor	= 0xFFFFFFFF;

	mTexel1 = Vector2::ZERO;
	mTexel2 = Vector2::ZERO;
	mTexel3 = Vector2::ZERO;
	mTexel4 = Vector2::ZERO;

	mInterp1 = Quaternion::ZERO;
	mInterp2 = Quaternion::ZERO;
	mInterp3 = Quaternion::ZERO;
	mInterp4 = Quaternion::ZERO;
	mInterp5 = Quaternion::ZERO;
	mInterp6 = Quaternion::ZERO;
	mInterp7 = Quaternion::ZERO;
	mInterp8 = Quaternion::ZERO;

	mTangent = Vector3::ZERO;
	mBiNormal= Vector3::ZERO;

	mWeight	 = Vector4::ZERO;

	mRadius = 0; // 用于布料模拟
	mBone[0] = mBone[1] = mBone[2] = mBone[3] = 0;
}

bool MeshVertex::operator==( const MeshVertex &v ) const
{
	bool ret = false;

	if ( memcmp(this,&v,sizeof(MeshVertex)) == 0 ) 
	{
		ret = true;
	}

	return ret;
}

MeshBone::MeshBone( void )
{
	mParentIndex = -1;
	mName = "";
	Identity();
}

void MeshBone::Set( const char *name,int32 parentIndex,const Vector3& pos,const Quaternion& rot,const Vector3& scale )
{
	mName = name;
	mParentIndex = parentIndex;
	mPosition = pos;
	mOrientation = rot;
	mScale = scale;
}

void MeshBone::Identity( void )
{
	mPosition = Vector3::ZERO;
	mOrientation = Quaternion::IDENTITY;
	mScale = Vector3::UNIT_SCALE;
}

MeshAnimPose::MeshAnimPose( void )
{
	mPos = Vector3::ZERO;
	mQuat = Quaternion::IDENTITY;
	mScale = Vector3::UNIT_SCALE;
}

void MeshAnimPose::SetPose( const Vector3& pos, const Quaternion& quat, const Vector3& scale )
{
	mPos = pos;
	mQuat = quat;
	mScale = scale;
}

void MeshAnimPose::Sample( Vector3& pos, Quaternion& quat, Vector3& scale ) const
{
	pos = mPos;
	quat = mQuat;
	scale = mScale;
}

void MeshAnimPose::getAngleAxis( Radian& angle,Vector3& axis ) const
{
	mQuat.ToAngleAxis(angle,axis);
}

MeshAnimTrack::MeshAnimTrack( void )
{
	mName = "";
	mFrameCount = 0;
	mDuration = 0;
	mDtime = 0;
	mPose = 0;
}

void MeshAnimTrack::SetName( const char *name )
{
	mName = name;
}

void MeshAnimTrack::SetPose( int32 frame,const Vector3& pos,const Quaternion& quat,const Vector3& scale )
{
	if ( frame >= 0 && frame < mFrameCount )
	{
		mPose[frame].SetPose(pos,quat,scale);
	}
}

void MeshAnimTrack::SampleAnimation( int32 frame,Vector3& pos,Quaternion& quat,Vector3& scale ) const
{
	mPose[frame].Sample(pos,quat,scale);
}

MeshAnimation::MeshAnimation( void )
{
	mName = "";
	mTrackCount = 0;
	mFrameCount = 0;
	mDuration = 0;
	mDtime = 0;
	mTracks = 0;
}

void MeshAnimation::SetTrackName( int32 track,const char *name )
{
	mTracks[track]->SetName(name);
}

void MeshAnimation::SetTrackPose( int32 track,int32 frame,const Vector3& pos,const Quaternion& quat,const Vector3& scale )
{
	mTracks[track]->SetPose(frame,pos,quat,scale);
}

const MeshAnimTrack* MeshAnimation::LocateTrack( const char *name ) const
{
	const MeshAnimTrack *ret = 0;
	for (int32 i=0; i<mTrackCount; i++)
	{
		const MeshAnimTrack *t = mTracks[i];
		if ( stricmp(t->GetName(),name) == 0 )
		{
			ret = t;
			break;
		}
	}
	return ret;
}

int32 MeshAnimation::GetFrameIndex( scalar t ) const
{
	t = fmodf( t, mDuration );
	int32 index = int32(t / mDtime);
	return index;
}

MeshAnimTrack * MeshAnimation::GetTrack( int32 index )
{
	MeshAnimTrack *ret = 0;
	if ( index >= 0 && index < mTrackCount )
	{
		ret = mTracks[index];
	}
	return ret;
}

_NAMESPACE_END


