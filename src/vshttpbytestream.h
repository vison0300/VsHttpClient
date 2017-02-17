/** 
Project: VsHttpClient
Author : vison0300
Email  : vison0300@163.com
*/
#pragma once
#include "VsHttpStream.h"

#include <vector>
#include <string>
#include <algorithm>
typedef unsigned char byte_t;
typedef std::vector<byte_t> ByteVector;

class CVsHttpByteStream : public CVsHttpStream
{
public:
	CVsHttpByteStream()
		: m_offset(0)
	{
	}

	CVsHttpByteStream(const std::string& str)
		: m_offset(0)
	{
		Write((byte_t*)&str[0], str.size());
		m_offset = 0;
	}

	CVsHttpByteStream(const byte_t* data, size_t length)
		: m_offset(0)
	{
		Write(data, length);
		m_offset = 0;
	}

	virtual ~CVsHttpByteStream()
	{
	}

	virtual void Clear()
	{
		m_bv.clear();
	}

	virtual void Seek(long offset, SeekDir_t dir = SeekDir_Begin)
	{
		switch (dir)
		{
		case SeekDir_Begin:
			{
				m_offset = max((long)0, offset);
			}
			break;
		case SeekDir_Current:
			{
				m_offset += offset;
			}
			break;
		case SeekDir_End:
			{
				m_offset = GetSize() + offset;
			}
			break;
		}
	}

	virtual size_t Tell() const
	{
		return m_offset;
	}

	virtual size_t GetSize() const
	{
		return m_bv.size();
	}

	virtual bool Write(const void* data, size_t size, size_t total_size = -1)
	{
		if (byte_t* p = (byte_t*)data)
		{
			m_bv.insert(m_bv.begin() + m_offset, p, p + size);
			m_offset += size;
			OnWrite(data, size, total_size);
			return true;
		}

		return false;
	}

	virtual size_t Read(void* data, size_t size, const char* szDelimiter = NULL)
	{
		byte_t* p = (byte_t*)data;
		size_t count = min(size, m_bv.size() - m_offset);
		size_t dlen = 0;

		if (szDelimiter)
		{
			dlen = strlen(szDelimiter);
		}

		for (size_t i = m_offset; i < m_offset + count; i++)
		{
			if (dlen > 0 && i + dlen < m_offset + count)
			{
				if (0 == memcmp(&m_bv[i], szDelimiter, dlen))
				{
					size_t sz = i - m_offset;
					m_offset = i + dlen;
					OnRead(data, sz, szDelimiter);
					return sz;
				}
			}

			*p++ = m_bv[i];
		}

		m_offset += count;
		OnRead(data, count, szDelimiter);
		return count;
	}

private:
	size_t m_offset;
	ByteVector m_bv;
};