
#include "gearsInputKeyboard.h"

using namespace Philo;

//----------------------------------------------------------------------//
void GearKeyboard::setTextTranslation( TextTranslationMode mode )
{
	mTextMode = mode;
}

//----------------------------------------------------------------------//
bool GearKeyboard::isModifierDown( Modifier mod )
{
#pragma warning (push)
#pragma warning (disable : 4800)
	return (mModifiers & mod);
#pragma warning (pop)
}
