/** 
Project: VsHttpClient
Author : vison0300
Email  : vison0300@163.com
*/
#pragma once
#include <vector>
#include <algorithm>

struct IVsHttpStream
{
	struct OnWriteListener
	{
		virtual void OnWrite(const void* data, size_t size, size_t total_size = -1) = 0;
	};

	virtual void AddOnWriteListener(OnWriteListener* pListener) = 0;
	virtual void RemoveOnWriteListener(OnWriteListener* pListener) = 0;

	struct OnReadListener
	{
		virtual void OnRead(void* data, size_t size, size_t total_size = -1, const char* szDelimter = NULL) = 0;
	};

	virtual void AddOnReadListener(OnReadListener* pListener) = 0;
	virtual void RemoveOnReadListener(OnReadListener* pListener) = 0;

	enum SeekDir_t
	{
		SeekDir_Begin,
		SeekDir_Current,
		SeekDir_End
	};

	virtual void Clear() = 0;
	virtual void Seek(long offset, SeekDir_t dir = SeekDir_Begin) = 0;
	virtual size_t Tell() const = 0;
	virtual size_t GetSize() const = 0;

	virtual bool Write(const void* data, size_t size, size_t total_size = -1) = 0;
	virtual size_t Read(void* data, size_t size, const char* szDelimter = NULL) = 0;
};

class CVsHttpStream : public IVsHttpStream
{
	typedef std::vector<OnReadListener*> OnReadListenerVector;
	typedef std::vector<OnWriteListener*> OnWriteListenerVector;

public:
	virtual void AddOnReadListener(OnReadListener* pListener)
	{
		if (m_vOnReadListener.end() == std::find(
			m_vOnReadListener.begin(), m_vOnReadListener.end(), pListener))
		{
			m_vOnReadListener.push_back(pListener);
		}
	}

	virtual void RemoveOnReadListener(OnReadListener* pListener)
	{
		OnReadListenerVector::iterator itr = std::find(
			m_vOnReadListener.begin(), m_vOnReadListener.end(), pListener);

		if (m_vOnReadListener.end() == itr)
		{
			m_vOnReadListener.erase(itr);
		}
	}

	virtual void AddOnWriteListener(OnWriteListener* pListener)
	{
		if (m_vOnWriteListener.end() == std::find(
			m_vOnWriteListener.begin(), m_vOnWriteListener.end(), pListener))
		{
			m_vOnWriteListener.push_back(pListener);
		}
	}

	virtual void RemoveOnWriteListener(OnWriteListener* pListener)
	{
		OnWriteListenerVector::iterator itr = std::find(
			m_vOnWriteListener.begin(), m_vOnWriteListener.end(), pListener);

		if (m_vOnWriteListener.end() == itr)
		{
			m_vOnWriteListener.erase(itr);
		}
	}

protected:
	virtual void OnWrite(const void* data, size_t size, size_t total_size = -1) 
	{
		OnWriteListenerVector::iterator itr = m_vOnWriteListener.begin(); 

		for (; itr != m_vOnWriteListener.end(); itr++)
		{
			(*itr)->OnWrite(data, size, total_size);
		}
	}

	virtual void OnRead(void* data, size_t size, const char* szDelimter = NULL)
	{
		size_t total_size = GetSize();
		OnReadListenerVector::iterator itr = m_vOnReadListener.begin(); 

		for (; itr != m_vOnReadListener.end(); itr++)
		{
			(*itr)->OnRead(data, size, total_size, szDelimter);
		}
	}

private:
	OnReadListenerVector m_vOnReadListener;
	OnWriteListenerVector m_vOnWriteListener;
};

