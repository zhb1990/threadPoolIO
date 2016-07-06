#pragma once
#include <windows.h>

class CCriSec
{
public:
	CCriSec() { ::InitializeCriticalSection(&m_crisec); }
	~CCriSec() { ::DeleteCriticalSection(&m_crisec); }

	void Lock() { ::EnterCriticalSection(&m_crisec); }
	void Unlock() { ::LeaveCriticalSection(&m_crisec); }

private:
	CCriSec(const CCriSec& cs);
	CCriSec operator = (const CCriSec& cs);

private:
	CRITICAL_SECTION    m_crisec;
};

template<int _iSpinCount>
class CCriSecSpinCount
{
public:
	CCriSecSpinCount() { ::InitializeCriticalSectionAndSpinCount(&m_crisec, _iSpinCount); }
	~CCriSecSpinCount() { ::DeleteCriticalSection(&m_crisec); }

	void Lock() { ::EnterCriticalSection(&m_crisec); }
	void Unlock() { ::LeaveCriticalSection(&m_crisec); }

private:
	CCriSecSpinCount(const CCriSecSpinCount& cs);
	CCriSecSpinCount operator = (const CCriSecSpinCount& cs);

private:
	CRITICAL_SECTION    m_crisec;
};

template<class CLockObj>
class CLocalLock
{
public:
	CLocalLock(CLockObj& obj) : m_lock(obj) { m_lock.Lock(); }
	~CLocalLock() { m_lock.Unlock(); }
private:
	CLockObj& m_lock;
};


typedef CCriSecSpinCount<400> CCriSec400;