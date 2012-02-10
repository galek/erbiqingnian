
#pragma once

_NAMESPACE_BEGIN

class NameGenerator
{
protected:
	String mPrefix;
	unsigned long long int mNext;

public:
	NameGenerator(const NameGenerator& rhs)
		: mPrefix(rhs.mPrefix), mNext(rhs.mNext) {}

	NameGenerator(const String& prefix) : mPrefix(prefix), mNext(1) {}

	/// Generate a new name
	String generate()
	{
		String s;
		s.Format("%s%d",mPrefix.c_str(),mNext++);
		return s;
	}

	/// Reset the internal counter
	void reset()
	{
		mNext = 1ULL;
	}

	/// Manually set the internal counter (use caution)
	void setNext(unsigned long long int val)
	{
		mNext = val;
	}

	/// Get the internal counter
	unsigned long long int getNext() const
	{
		// lock even on get because 64-bit may not be atomic read
		return mNext;
	}
};

_NAMESPACE_END