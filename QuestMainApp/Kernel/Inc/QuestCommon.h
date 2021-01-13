#pragma once

class CScopedCriticalSection
{
public:
	CScopedCriticalSection()
	{
		::InitializeCriticalSection(&cs);
	}

	~CScopedCriticalSection()
	{
		::DeleteCriticalSection(&cs);
	}

	void Enter()
	{
		::EnterCriticalSection(&cs);
	}

	void Leave()
	{
		::LeaveCriticalSection(&cs);
	}

	bool TryEnter()
	{
		return ::TryEnterCriticalSection(&cs);
	}

private:
	CRITICAL_SECTION cs;
};

