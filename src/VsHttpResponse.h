/** 
Project: VsHttpClient
Author : vison0300
Email  : vison0300@163.com
*/
#pragma once

#include "VsHttpPacket.h"
#include "VsHttpByteStream.h"

class CVsHttpResponse : public CVsHttpPacket
{
public:
	CVsHttpResponse(IVsHttpStream* pStreamContent = new CVsHttpByteStream(), bool bDeleteStream = true)
		: CVsHttpPacket(pStreamContent, bDeleteStream)
	{
	}

	virtual ~CVsHttpResponse()
	{
	}

	void SetCode(int code)
	{
		m_nCode = code;
	}

	int GetCode() const
	{
		return m_nCode;
	}

	void SetMessage(const std::string& strMessage)
	{
		m_strMessage = strMessage;
	}

	std::string GetMessage() const
	{
		return m_strMessage;
	}

protected:
	virtual std::string BuildFirstLine() const
	{
		std::stringstream ss;
		ss<<GetHttpVersion();
		ss<<" ";
		ss<<GetCode();
		ss<<" ";
		ss<<GetMessage();

		return ss.str();
	}


	bool ParseFirstLine(const std::string& str)
	{
		size_t pos = str.find(" ");

		if (std::string::npos == pos)
		{
			return false;
		}
		else
		{
			SetHttpVersion(str.substr(0, pos));
		}

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

		std::string strCode = str.substr(pos, end - pos);
		m_nCode = atoi(strCode.c_str());
		
		pos = end + 1;
		end = str.find("\r\n", pos);

		if (std::string::npos != end)
		{
			m_strMessage = str.substr(pos, end - pos);
		}
		else
		{
			m_strMessage = str.substr(pos);
		}

		return true;
	}

private:
	int m_nCode;
	std::string m_strMessage;
};