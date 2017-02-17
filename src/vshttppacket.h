/** 
Project: VsHttpClient
Author : vison0300
Email  : vison0300@163.com
*/
#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
using namespace std;

#include <stdarg.h>
#include "VsHttpStream.h"

#include <Winsock2.h>
#pragma comment(lib, "ws2_32.lib")  

struct CWinsockInit
{
	CWinsockInit()
	{
		WSADATA WSAData = {0};
		WORD wVersionRequested = MAKEWORD(2, 2);
	
		if (0 != WSAStartup(wVersionRequested, &WSAData))
		{
			std::cout<<"WSAStartup error"<<std::endl;
		}
	}

	~CWinsockInit()
	{
		if (0 != WSACleanup())
		{
			std::cout<<"WSACleanup error"<<std::endl;
		}
	}
};

static CWinsockInit IniObj;

class CVsHttpPacket
{
	const static int BUFFER_SIZE = 4096;
	typedef std::pair<std::string, std::string> String2StringPair;
	typedef std::vector<String2StringPair> String2StringPairVector;
public:
	CVsHttpPacket(IVsHttpStream* pStreamContent, bool bDeleteStream)
		: m_pStreamContent(pStreamContent)
		, m_bDeleteStreamContent(bDeleteStream)
		, m_strHttpVersion("HTTP/1.1")
	{
	}

	virtual ~CVsHttpPacket()
	{
		if (m_pStreamContent)
		{
			if (m_bDeleteStreamContent)
			{
				delete m_pStreamContent;
			}

			m_pStreamContent = NULL;
		}
	}

	virtual void Clear()
	{
		m_strHttpVersion.clear();
		m_vHeader.clear();

		if (m_pStreamContent)
		{
			m_pStreamContent->Clear();
		}
	}

	void SetHttpVersion(const std::string& strHttpVersion)
	{
		m_strHttpVersion = strHttpVersion;
	}

	const std::string& GetHttpVersion() const 
	{
		return m_strHttpVersion;
	}

	void SetHeader(const std::string& strName, const char* szValueFormat, ...) 
	{
		if (strName.empty())
		{
			return;
		}

		char buf[1024] = {0};

		va_list args;
		va_start(args, szValueFormat);
		vsprintf_s(buf, sizeof(buf), szValueFormat, args);
		va_end(args);

		std::string strValue = buf;
		String2StringPairVector::iterator itr = m_vHeader.begin();

		for (; itr != m_vHeader.end(); itr++)
		{
			if (itr->first ==strName)
			{
				itr->second = strValue;
				return;
			}
		}

		m_vHeader.push_back(String2StringPair(strName, strValue));
	}

	const std::string& GetHeader(const std::string& strName) const
	{
		String2StringPairVector::const_iterator itr = m_vHeader.begin();

		for (; itr != m_vHeader.end(); itr++)
		{
			if (itr->first ==strName)
			{
				return itr->second;
			}
		}

		static const std::string strNull;
		return strNull;
	}


	IVsHttpStream* GetContent()
	{
		return m_pStreamContent;
	}

	bool toStream(IVsHttpStream& s) const 
	{
		s.Seek(0);
		std::string strTemp = BuildFirstLine() + "\r\n";
		s.Write(strTemp.c_str(), strTemp.size());
		String2StringPairVector::const_iterator itr = m_vHeader.begin();

		for (; itr != m_vHeader.end(); itr++)
		{
			strTemp = itr->first + ": " + itr->second + "\r\n";
			s.Write(strTemp.c_str(), strTemp.size());
		}

		if (m_pStreamContent->GetSize() > 0)
		{
			if (GetHeader("Content-Length").empty())
			{
				std::stringstream ss;
				ss<<m_pStreamContent->GetSize();
				
				strTemp = "Content-Length: " + ss.str() + "\r\n";
				s.Write(strTemp.c_str(), strTemp.size());
			}
		}

		strTemp = "\r\n";
		s.Write(strTemp.c_str(), strTemp.size());

		if (m_pStreamContent->GetSize() > 0)
		{
			std::vector<unsigned char> vBuf(m_pStreamContent->GetSize());
			m_pStreamContent->Seek(0);
			m_pStreamContent->Read(&vBuf[0], vBuf.size());
			m_pStreamContent->Seek(0);

			s.Write(&vBuf[0], vBuf.size());
		}

		s.Seek(0);
		return true;
	}

	bool fromStream(const IVsHttpStream& ts)
	{
		IVsHttpStream& s = (IVsHttpStream&)ts;

		char buf[BUFFER_SIZE] = {0};
		s.Read(buf, sizeof(buf), "\r\n");

		if (!ParseFirstLine(buf))
		{
			return false;
		}
		
		while (s.Read(buf, sizeof(buf), "\r\n"))
		{
			if (!ParseHeader(buf))
			{
				return false;
			}

			memset(buf, 0, sizeof(buf));
		}

		std::string strContentLength = GetHeader("Content-Length");
		int nContentLength = atoi(strContentLength.c_str());

		if (m_pStreamContent && nContentLength > 0)
		{
			size_t sz = 0;
			memset(buf, 0, sizeof(buf));
			m_pStreamContent->Seek(0);

			while (sz = s.Read(buf, sizeof(buf)))
			{
				m_pStreamContent->Write(buf, sz);
			}

			m_pStreamContent->Seek(0);
		}

		return true;
	}

protected:
	virtual std::string BuildFirstLine() const = 0;
	virtual bool ParseFirstLine(const std::string& strLine) = 0;

	virtual bool ParseHeader(const std::string str)
	{
		size_t pos = str.find(":");

		if (std::string::npos == pos)
		{
			return false;
		}

		std::string key = str.substr(0, pos);
		std::string value = str.substr(str.find_first_not_of(' ', pos + 1));
		SetHeader(key.c_str(), value.c_str());
		return true;
	}

private:
	std::string m_strHttpVersion;
	String2StringPairVector m_vHeader;
	IVsHttpStream* m_pStreamContent;
	bool m_bDeleteStreamContent;
};