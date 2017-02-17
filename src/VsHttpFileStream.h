/** 
Project: VsHttpClient
Author : vison0300
Email  : vison0300@163.com
*/
#pragma once
#include "VsHttpStream.h"
#include <fstream>
using namespace std;

class CVsHttpFileStream : public CVsHttpStream
{
public:
	CVsHttpFileStream(const std::string& strFilePath)
		: m_fs(strFilePath.c_str(),  ios::in | ios::out | ios::ate | ios::binary)
	{
		if (!m_fs.is_open())
		{
			m_fs.clear();
			m_fs.open(strFilePath.c_str(), ios::in | ios::out | ios::trunc | ios::binary);
		}
	}

	virtual ~CVsHttpFileStream()
	{
		if (m_fs.is_open())
		{
			m_fs.close();
		}
	}

	virtual void Clear()
	{
		if (m_fs.is_open())
		{
			m_fs.clear();
		}
	}

	virtual void Seek(long offset, SeekDir_t dir = SeekDir_Begin)
	{
		if (m_fs.is_open())
		{
			m_fs.seekg(offset, ios::seek_dir(dir));
			m_fs.seekp(offset, ios::seek_dir(dir));
		}
	}

	virtual size_t Tell() const
	{
		return const_cast<std::fstream&>(m_fs).tellp();
	}

	virtual size_t GetSize() const
	{
		std::fstream& fs = const_cast<std::fstream&>(m_fs);
		size_t offset = fs.tellg();
		fs.seekg(0, std::ios::end);
		size_t sz = fs.tellg();
		fs.seekg(offset, ios::beg);

		if (0xffffffff == sz)
		{
			return 0;
		}

		return sz;
	}

	virtual bool Write(const void* data, size_t size, size_t total_size = -1)
	{
		size_t offset = m_fs.tellp();
		m_fs.write((char*)data, size);
		m_fs.seekp(m_fs.tellp());

		if (((size_t)m_fs.tellp() - offset) == size)
		{
			OnWrite(data, size, total_size);
			return true;
		}

		return false;
	}

	virtual size_t Read(void* data, size_t size, const char* szDelimter = NULL) 
	{
		std::istream::pos_type offset = m_fs.tellg();

		if (szDelimter)
		{
			Read(m_fs, data, size, szDelimter);
		}
		else
		{
			m_fs.read((char*)data, size);
		}

		m_fs.seekp(m_fs.tellg());
		size_t sz = m_fs.tellg() - offset;
		OnRead(data, sz, szDelimter);
		return sz;
	}

	static std::istream& Read(std::istream& is, void* buffer, size_t length, const std::string& strDelimter)
	{
		std::istream::sentry se(is, true);
		std::streambuf* sb = is.rdbuf();
		size_t index = 0;
		char* pBuf = (char*)buffer;

		while (index < length)
		{
			if (EOF == sb->sgetc())
			{
				return is;
			}

			size_t i = 0;

			for (;i < strDelimter.size(); i++)
			{
				if (sb->sgetc() != strDelimter[i])
				{
					break;
				}

				sb->stossc();
			}

			if (i >= strDelimter.size())
			{
				return is;
			}
			else
			{
				while (i > 0)
				{
					i--;
					sb->sungetc();
				}
			}

			pBuf[index++] = (char)sb->sbumpc();
		}
	}
private:
	std::fstream m_fs;
};