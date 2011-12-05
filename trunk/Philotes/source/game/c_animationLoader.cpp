/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  This software contains code, techniques and know-how
 * which is confidential and proprietary to Havok.  Not for disclosure or distribution without prior written consent.
 * Havok Game Dynamics SDK (C) Copyright 1999/2004 Telekinesys Research Limited. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#include "prime.h"
#include "c_animationLoader.h"

hkAnimationLoader::hkAnimationLoader()
{
	m_session = new hkSerializerSession(HK_NULL);

	// We register all the aniamtion classes
	// Animation depends on scene data such as Meshes for the skin
	// and we load scene data too, so those classes must be registered too.
	hkxSceneDataClasses::registerClasses( m_session );

	m_session->registerClass(&hkInterleavedAnimationCinfoClass);
	m_session->registerClass(&hkWaveletCompressedAnimationCinfoClass);
	m_session->registerClass(&hkDeltaCompressedAnimationCinfoClass);
	m_session->registerClass(&hkBoneClass);
	m_session->registerClass(&hkBoneAttachmentClass);
	m_session->registerClass(&hkSkeletonClass);
	m_session->registerClass(&hkAnimationBindingClass);
	m_session->registerClass(&hkMeshBindingClass);
	m_session->registerClass(&hkDefaultExtractedMotionCinfoClass);
	m_session->registerClass(&hkAnimationTrackClass);
	m_session->registerRawPointer(HK_NULL, "NULL"); // so that any NULL references will be null.

	// Disable duplicate animation name warning
	hkError::getInstance().setEnabled(0x24bc7aab, false);

}

hkAnimationLoader::~hkAnimationLoader()
{
	{
		for ( int i = 0; i < m_loadedObjects.getSize(); ++i )
		{
			for (int j = 0; j < m_loadedObjects[i].getSize(); ++ j )
			{
				m_loadedObjects[i][j]->removeReference();
			}
		}
	}
	{
		for ( int i = 0; i < m_loadedReflecteds.getSize(); ++i )
		{
			for (int j = 0; j < m_loadedReflecteds[i].getSize(); ++ j )
			{
				m_loadedReflecteds[i][j]->removeReference();
			}
		}
	}

	for (int rp = 0; rp < m_rawPackets.getSize(); ++rp )
	{
		m_rawPackets[rp]->removeReference();
	}
	m_rawPackets.setSize(0);

	delete m_session;
}

hkUint32 hkAnimationLoader::getNumObjectsOfType(const hkClass* c)
{
	int type;
	if ( m_serializableTypes.get( (hkUlong)c, (hkUlong*)&type ) == HK_SUCCESS )
	{
		return m_loadedObjects[ type ].getSize();
	}
	else if ( m_reflectedTypes.get( (hkUlong)c, (hkUlong*)&type ) == HK_SUCCESS )
	{
		return m_loadedReflecteds[ type ].getSize();
	}
	else
	{
		return 0;
	}
}

const void* hkAnimationLoader::getObjectOfType(const hkClass* c, hkUint32 index)
{
	int type;
	if ( m_serializableTypes.get( (hkUlong)c, (hkUlong*)&type ) == HK_SUCCESS )
	{
		return m_loadedObjects[ type ][index];
	}
	else if ( m_reflectedTypes.get( (hkUlong)c, (hkUlong*)&type ) == HK_SUCCESS )
	{
		return m_loadedReflecteds[ type ][index]->getData();
	}
	else
	{
		return HK_NULL;
	}
}

void hkAnimationLoader::loadFile( const char* filename )
{
	hkStreamReader* sb = hkStreambufFactory::getInstance().openReader(filename);
	if( sb->isOk() == false )
	{
		HK_WARN(0x74807fd1, "Unable to open " << filename << ", skipping");
		return ;
	}

	hkSerializerReader* serializer = new hkXmlSerializerReader(sb, m_session->getStringPool());

	// The serializer now owns the streambuff object
	sb->removeReference(); 

	{
		while( 1 )
		{
			// We read a packet 
			hkPacket* packet = m_session->loadSinglePacket(*serializer);

			if( packet != HK_NULL )
			{
				// OBJECT packets : create an hkserializable object
				if( packet->getType() == hkPacket::OBJECT )
				{
					hkObjectPacket* opacket = static_cast<hkObjectPacket*>(packet);

					const hkClass* klass = opacket->getPacketClass();

					hkReflected* reflected = const_cast<hkReflected*>(static_cast<const hkReflected*>(opacket->getData()));
					reflected->m_class = klass;

					// check if the klass has a pointer to become a serializable, and if so, create one
					if (klass)
					{
						if (klass->getCinfoFunctions()->createFunc != HK_NULL)
						{
							hkSerializable* s = m_session->createObject( *opacket );

							if (s)
							{
								int index = (int)m_serializableTypes.getWithDefault( (hkUlong) s->getClass(), (hkUlong) m_loadedObjects.getSize() );
								if (index == m_loadedObjects.getSize())
								{
									m_loadedObjects.expandBy(1);
									m_serializableTypes.insert((hkUlong) s->getClass(), (hkUlong) index);
								}
								m_loadedObjects[index].pushBack(s);
								s->addReference();
							}
						}
						else
						{
							// otherwise simply convert the pointers of the packet, keep a reference to the data, and add
							// the new reflected to the map

							if( m_session->convertNamesToPointers(*opacket, HK_NULL) == HK_SUCCESS )
							{
								m_session->registerRawPointer( const_cast<void*>(opacket->getData()), opacket->getName().cString() );
							}

							int index = (int)m_reflectedTypes.getWithDefault( (hkUlong) klass, (hkUlong) m_loadedReflecteds.getSize() );
							if (index == m_loadedReflecteds.getSize())
							{
								m_loadedReflecteds.expandBy(1);
								m_reflectedTypes.insert((hkUlong) klass, (hkUlong) index);
							}
							m_loadedReflecteds[index].pushBack(opacket);

							opacket->addReference();
						}
					}
				}
				else if( packet->getType() == hkPacket::RAW)
				{	
					// Keep all raw data in the file in case we need to reference it (mopp code, inplace textures, etc)
					hkRawPacket* rpacket = static_cast<hkRawPacket*>(packet);
					const char* name = m_session->getStringPool()->insert(rpacket->getName().cString());
					m_session->registerRawPointer( (void*)(rpacket->getData()), name );
					m_rawPackets.pushBack(rpacket);
					rpacket->addReference();
				}
				packet->removeReference();
			}
			else 
			{
				break;
			}
		}
	}
	serializer->removeReference();
}

void hkAnimationLoader::getAllObjects( hkArray<hkSerializable*>& objs)
{
	int numTypes = m_loadedObjects.getSize();
	int totalSize = 0;
	int i,j;
	for (i=0; i < numTypes; ++i)
		totalSize += m_loadedObjects[i].getSize();

	objs.reserveExactly( totalSize );

	for (i=0; i < numTypes; ++i)
	{
		int s = m_loadedObjects[i].getSize();
		for (j=0; j < s; ++j)
			objs.pushBack( (m_loadedObjects[i])[j] );
	}
}

void hkAnimationLoader::getAllReflecteds( hkArray<hkReflected*>& objs)
{
	int numTypes = m_loadedReflecteds.getSize();
	int totalSize = 0;
	int i,j;
	for (i=0; i < numTypes; ++i)
		totalSize += m_loadedReflecteds[i].getSize();

	objs.reserveExactly( totalSize );

	for (i=0; i < numTypes; ++i)
	{
		int s = m_loadedReflecteds[i].getSize();
		for (j=0; j < s; ++j)
		{
			objs.pushBack( const_cast<hkReflected*>(static_cast<const hkReflected*>( (m_loadedReflecteds[i])[j]->getData() ) ) );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static hkAnimationLoader * sgpAnimationLoader = NULL;
hkAnimationLoader * c_AnimationLoaderGet()
{
	if ( ! sgpAnimationLoader )
		sgpAnimationLoader = new hkAnimationLoader;
	return sgpAnimationLoader;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void AnimationLoaderClose()
{
	delete sgpAnimationLoader;
}

/*
* Havok Game Dynamics SDK - DEMO RELEASE, BUILD(#20040819)
*
* (C) Copyright 1999/2004 Telekinesys Research Limited. All Rights Reserved. The Telekinesys
* Logo, Havok and the Havok buzzsaw logo are trademarks of Telekinesys
* Research Limited.  Title, ownership
* rights, and intellectual property rights in the Software remain in
* Telekinesys Research Limited and/or its suppliers.
*
* Use of this software is subject to and indicates acceptance of the End User licence
* Agreement for this product. A copy of the license is included with this software and
* is also available from info@havok.com.
*
*/ 
