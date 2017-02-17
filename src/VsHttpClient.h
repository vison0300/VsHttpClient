/** 
Project: VsHttpClient
Author : vison0300
Email  : vison0300@163.com
*/
#pragma once
#include "VsHttpRequest.h"
#include "VsHttpResponse.h"
#include "VsHttpByteStream.h"

class CVsHttpClient
{
public:
	CVsHttpClient()
	{
		m_fd = INVALID_SOCKET;
	}

	virtual ~CVsHttpClient()
	{
	}

	virtual bool Send(const CVsHttpRequest& request, CVsHttpResponse& response)
	{
		if (!SendRequest(request))
		{
			return false;
		}

		int bytesRecv = 0;
		int bytesOffset = 0;
		int bufOffset = 0;
		int frameSize = 4096;

		int nTimeout = 10000;
		setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&nTimeout, sizeof(nTimeout));

		std::vector<char> recvbuf(1024 * 1024);
		char* pRecvBuffer = &recvbuf[0];

		bytesRecv = recv(m_fd, pRecvBuffer, frameSize, 0);
		std::cout<<std::endl<<"recvbuf[size = "<<bytesRecv<<"] : \r\n"<<pRecvBuffer<<std::endl;

		static const std::string str2CRFL = "\r\n\r\n";
		char* pContentStart = strstr(pRecvBuffer, str2CRFL.c_str());

		int data_size = 0;
		int data_offset = bytesRecv;

		if (pContentStart)
		{
			pContentStart += str2CRFL.size();
			data_offset = pContentStart - pRecvBuffer;
			data_size = bytesRecv - data_offset;
		}

		response.fromStream(CVsHttpByteStream((byte_t*)pRecvBuffer, data_offset));
		IVsHttpStream* ps = (IVsHttpStream*)response.GetContent();

		if (NULL == ps)
		{
			closesocket(m_fd);
			return false;
		}

		bool bOK = true;
		std::string strContentLength = response.GetHeader("Content-Length");

		if (!strContentLength.empty())
		{
			int nContentLength = atoi(strContentLength.c_str());

			if (ps->GetSize() >= nContentLength)
			{
				bOK = true;
				std::cout<<"CVsHttpClient::Send::Content-Length : "<<nContentLength<<std::endl;
			}
			else
			{
				bOK = RecvContentByLength(ps, pRecvBuffer, (int)recvbuf.size(), data_offset, data_size, nContentLength);
			}
		}
		else
		{
			std::string strTransferEncoding = response.GetHeader("Transfer-Encoding");

			if ("chunked" == strTransferEncoding)
			{
				bOK = RecvContentByChunked(ps, pRecvBuffer, (int)recvbuf.size(), data_offset, data_size);
			}
			else
			{
				bOK = false;
				std::cout<<"CVsHttpClient::Send::Transfer-Encoding : "<<strTransferEncoding.c_str()<<" unimplemented"<<std::endl;
			}
		}

		ps->Seek(0);
		closesocket(m_fd);
		return bOK;
	}

protected:
	bool ReadLine(char* buffer, int size, std::string& strLine, const std::string strDelimiter = "\r\n")
	{
		std::string str;
		static const std::string strCRFL = "\r\n";

		for (int i = 0; i < size; i++)
		{
			if (i + strDelimiter.size() <= size)
			{
				if (0 == memcmp(&buffer[i], strDelimiter.c_str(), strDelimiter.size()))
				{
					strLine = str;
					return true;
				}
			}

			str += buffer[i];
		}

		return false;
	}

	bool RecvContentByChunked(IVsHttpStream* ps, char* buffer, int buffer_size, int data_offset, int data_size)
	{
		std::string strTemp;
		std::string strLine;

		int size = data_size;
		int offset = data_offset;
		int recv_size = 0;
		int frame_size = 4096;
		int bytes_recv = 0;
		static const std::string strCRFL = "\r\n";

		while (ReadLine(&buffer[offset], size, strLine))
		{
			if (strLine.empty())
			{
				return true;
			}

			size -= strLine.size() + strCRFL.size();
			offset += strLine.size() + strCRFL.size();

			size_t pos = strLine.find(";");
			strTemp = strLine.substr(0, pos);
			long chunk_size = strtol(strTemp.c_str(), NULL, 16);

			if (0 == chunk_size)
			{
				return true;
			}

			if (!RecvContentByLength(ps, buffer, buffer_size, offset, size, chunk_size))
			{
				return false;
			}

			offset = 0;
			recv_size = min(frame_size, buffer_size - offset);
			bytes_recv = recv(m_fd, &buffer[offset], recv_size, 0);

			if (bytes_recv == 0 || bytes_recv == SOCKET_ERROR) 
			{
				int err = WSAGetLastError();

				if (bytes_recv == 0)
				{
					std::cout<<"RecvContentByChunked: Connection Closed"<<std::endl;
				}
				else if (err == WSAECONNRESET)
				{
					std::cout<<"RecvContentByChunked: Connection Reset"<<std::endl;
				}
				else
				{
					std::cout<<"RecvContentByChunked: Connection Error("<<err<<")"<<std::endl;
				}

				return false;
			}
			else
			{
				size = bytes_recv;
			}
		}

		return false;
	}

	bool RecvContentByLength(IVsHttpStream* ps, char* buffer, int buffer_size, int data_offset, int data_size, int content_length)
	{
		int recv_size = 0;
		int frame_size = min(4096, buffer_size);
		int bytes_recv = 0;
		int bytes_start = data_size;
		int bytes_offset = data_size;
		int buffer_start = data_offset;
		int buffer_offset = data_offset + data_size;

		DWORD dwStartTime = GetTickCount();

		if (buffer_size <= data_size)
		{
			return false;
		}

		if (bytes_offset >= content_length)
		{
			return ps->Write(&buffer[data_offset], data_size, content_length);
		}

		std::cout<<"RecvContentByLength("<<content_length<<") starting..."<<std::endl;

		while (bytes_offset < content_length) 
		{
			recv_size = (int)min(min(frame_size, content_length - bytes_offset), buffer_size - buffer_offset);
			bytes_recv = recv(m_fd, &buffer[buffer_offset], recv_size, 0);

			if (bytes_recv == 0 || bytes_recv == SOCKET_ERROR) 
			{
				int err = WSAGetLastError();

				if (bytes_recv == 0)
				{
					std::cout<<"RecvContentByLength: Connection Closed"<<std::endl;
				}
				else if (err == WSAECONNRESET)
				{
					std::cout<<"RecvContentByLength: Connection Reset"<<std::endl;
				}
				else
				{
					std::cout<<"RecvContentByLength: Connection Error("<<err<<")"<<std::endl;
				}

				return false;
			}

			bytes_offset += bytes_recv;
			buffer_offset += bytes_recv;

			if (buffer_offset - buffer_start + frame_size >= buffer_size || bytes_offset >= content_length)
			{
				ps->Write(&buffer[buffer_start], buffer_offset - buffer_start, content_length);
				buffer_start = 0;
				buffer_offset = 0;
			}
		}

		return bytes_offset == content_length;
	}

	virtual bool SendRequest(const CVsHttpRequest& request)
	{
		int nPort = 0;
		std::string strIPAddress;
		std::string strHostName;

		if (!CVsHttpRequest::GetHostNameByUrl(request.GetUrl(), strHostName))
		{
			std::cout<<"CVsHttpClient::SendRequest: CVsHttpRequest::GetHostNameByUrl("<<request.GetUrl()<<") failed"<<std::endl;
			return false;
		}

		if (!CVsHttpRequest::GetIPAndPortByHostName(strHostName, strIPAddress, nPort))
		{
			std::cout<<"CVsHttpClient::SendRequest: CVsHttpRequest::GetIPAndPortByHostName("<<strHostName<<") failed"<<std::endl;
			return false;
		}

		m_fd = socket(AF_INET, SOCK_STREAM, 0);

		if (INVALID_SOCKET == m_fd)
		{
			std::cout<<"CVsHttpClient::SendRequest: create socket failed"<<std::endl;
			return false;
		}

		sockaddr_in addr = {0};
		addr.sin_addr.S_un.S_addr = inet_addr(strIPAddress.c_str());
		addr.sin_port = htons(nPort);
		addr.sin_family = AF_INET;

		if (SOCKET_ERROR == connect(m_fd, (sockaddr*)&addr, sizeof(addr)))
		{
			std::cout<<"CVsHttpClient::SendRequest: connect failed"<<std::endl;
			closesocket(m_fd);
			return false;
		}

		int nTimeout = 10000;
		setsockopt(m_fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&nTimeout, sizeof(nTimeout));

		CVsHttpByteStream is;
		request.toStream(is);
		std::vector<byte_t> vBuf(is.GetSize() + 1);
		is.Read(&vBuf[0], vBuf.size());

		int bytesSent = send(m_fd, (char*)&vBuf[0], vBuf.size(), 0);

		std::cout<<"CVsHttpClient::SendRequest[size = "<<bytesSent<<"] : "<<std::endl<<(char*)&vBuf[0]<<std::endl;
		return bytesSent == vBuf.size();
	}

private:
	SOCKET m_fd;
};