/** 
Project: VsHttpClient
Author : vison0300
Email  : vison0300@163.com
*/
#pragma once
#include "VsHttpPacket.h"
#include "VsHttpByteStream.h"

class CVsHttpRequest : public CVsHttpPacket
{
public:
	CVsHttpRequest(IVsHttpStream* pStreamContent = new CVsHttpByteStream(), bool bDeleteStream = true)
		: CVsHttpPacket(pStreamContent, bDeleteStream)
	{
	}

	virtual void Clear()
	{
		__super::Clear();
		m_strMethod.clear();
		m_strUrl.clear();
	}

	void SetMethod(const std::string& strMethod) 
	{
		m_strMethod = strMethod;
		std::transform(m_strMethod.begin(), m_strMethod.end(), m_strMethod.begin(), toupper);
	}

	const std::string& GetMethod() const 
	{
		return m_strMethod;
	}

	void SetUrl(const std::string& strUrl)
	{
		m_strUrl = strUrl;
		std::string strHostName;

		if (GetHostNameByUrl(strUrl, strHostName))
		{
			SetHeader("Host", strHostName.c_str());
		}
	}

	const std::string& GetUrl() const
	{
		return m_strUrl;
	}

	static bool GetHostNameByUrl(const std::string& strUrl, std::string& strHostName)
	{
		size_t pos = strUrl.find("://");

		if (std::string::npos == pos)
		{
			return false;
		}

		pos += strlen("://");
		size_t end = strUrl.find("/", pos);

		if (std::string::npos == end)
		{
			end = strUrl.find(" ", pos);

			if (std::string::npos == end)
			{
				end = strUrl.find("\r\n", pos);
			}
		}

		strHostName = strUrl.substr(pos, end - pos);
		return !strHostName.empty();
	}

	static bool GetIPAndPortByHostName(const std::string& strHostName, std::string& strIPAddress, int& nPort)
	{
		if (strHostName.empty())
		{
			return false;
		}

		size_t pos = 0;
		size_t end = strHostName.find(":", pos);

		if (std::string::npos != end)
		{
			strIPAddress = strHostName.substr(pos, end - pos);

			if (strIPAddress.empty())
			{
				return false;
			}

			std::string strPort = strHostName.substr(end + 1);

			if (strPort.empty())
			{
				return false;
			}

			nPort = atoi(strPort.c_str());
			return true;
		}
		else
		{
			if (struct hostent* phe = gethostbyname(strHostName.c_str()))
			{
				if (phe->h_length > 0)
				{
					std::string strAddr = inet_ntoa(*(in_addr*)phe->h_addr_list[0]);

					if (!strAddr.empty())
					{
						nPort = 80;
						strIPAddress = strAddr;
						return true;
					}
				}
			}
		}

		return false;
	}
protected:
	virtual std::string BuildFirstLine() const
	{
		std::stringstream ss;
		ss<<GetMethod();
		ss<<" ";
		ss<<GetUrl();
		ss<<" ";
		ss<<GetHttpVersion();

		return ss.str();
	}

	bool ParseFirstLine(const std::string& str)
	{
		size_t pos = str.find(" ");

		if (std::string::npos == pos)
		{
			return false;
		}

		SetMethod(str.substr(0, pos));

		pos++;
		size_t end = str.find(" ", pos);

		if (std::string::npos == end)
		{
			end = str.find("\r\n", pos);

			if (std::string::npos == end)
			{
				return false;
			}
		}

		SetUrl(str.substr(pos, end - pos));

		pos++;
		end = str.find("\r\n", pos);

		if (std::string::npos == end)
		{
			return false;
		}

		SetHttpVersion(str.substr(pos, end - pos));
		return true;
	}


private:
	std::string m_strUrl;
	std::string m_strMethod;
};