/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include "packer.h"
#include "compression.h"
#include "engine.h"
#include "config.h"

void ZPack2ConvertCyrillic(char * str, bool decode)
{
        unsigned int len = str_length(str);
        if (len == 0) return;
        if (!decode)
        {
                unsigned char * pt = (unsigned char *)str, * t = (unsigned char *)str;
                int ch;
                while (( ch = str_utf8_decode((const char **)(&t)) ) != 0)
                {
                        if (ch >= 1039 && ch <= 1039 + 64)
                        {
                                pt[0] = 175;
                                pt[1] = ch - 1039 + 255 - 64;
                        }
                        pt = t;
                }
        } else {
                unsigned char * t = (unsigned char *)str;
                while (t[0])
                {
                        if (t[0] == 175)
                        {
                                str_utf8_encode((char *)t, (int)t[1] + 1039 - 255 + 64);
                                t++;
                                if (!t[0]) break;
                        }
                        t++;
                }
        }
}

void CPacker::Reset()
{
	m_Error = 0;
	m_pCurrent = m_aBuffer;
	m_pEnd = m_pCurrent + PACKER_BUFFER_SIZE;
}

void CPacker::AddInt(int i)
{
	if(m_Error)
		return;
		
	// make sure that we have space enough
	if(m_pEnd - m_pCurrent < 6)
	{
		dbg_break();
		m_Error = 1;
	}
	else
		m_pCurrent = CVariableInt::Pack(m_pCurrent, i);
}

void CPacker::AddString(const char *pStr, int Limit)
{
	if(m_Error)
		return;

	char * Ptr = (char *)m_pCurrent;
	
	//
	if(Limit > 0)
	{
		while(*pStr && Limit != 0)
		{
			*m_pCurrent++ = *pStr++;
			Limit--;
			
			if(m_pCurrent >= m_pEnd)
			{
				m_Error = 1;
				break;
			}
		}
		*m_pCurrent++ = 0;
	}
	else
	{
		while(*pStr)
		{
			*m_pCurrent++ = *pStr++;

			if(m_pCurrent >= m_pEnd)
			{
				m_Error = 1;
				break;
			}
		}
		*m_pCurrent++ = 0;
	}

	if (g_Config.m_NetZPack2Cyrillic)
		ZPack2ConvertCyrillic(Ptr, true);
}

void CPacker::AddRaw(const void *pData, int Size)
{
	if(m_Error)
		return;
		
	if(m_pCurrent+Size >= m_pEnd)
	{
		m_Error = 1;
		return;
	}
	
	const unsigned char *pSrc = (const unsigned char *)pData;
	while(Size)
	{
		*m_pCurrent++ = *pSrc++;
		Size--;
	}
}


void CUnpacker::Reset(const void *pData, int Size)
{
	m_Error = 0;
	m_pStart = (const unsigned char *)pData;
	m_pEnd = m_pStart + Size;
	m_pCurrent = m_pStart;
}

int CUnpacker::GetInt()
{
	if(m_Error)
		return 0;
		
	if(m_pCurrent >= m_pEnd)
	{
		m_Error = 1;
		return 0;
	}
	
	int i;
	m_pCurrent = CVariableInt::Unpack(m_pCurrent, &i);
	if(m_pCurrent > m_pEnd)
	{
		m_Error = 1;
		return 0;
	}
	return i;
}

const char *CUnpacker::GetString(int SanitizeType)
{
	if(m_Error || m_pCurrent >= m_pEnd)
		return "";
		
	char *pPtr = (char *)m_pCurrent;
	while(*m_pCurrent) // skip the string
	{
		m_pCurrent++;
		if(m_pCurrent == m_pEnd)
		{
			m_Error = 1;;
			return "";
		}
	}
	m_pCurrent++;

	if (g_Config.m_NetZPack2Cyrillic)
		ZPack2ConvertCyrillic(pPtr, true);
	
	// sanitize all strings
	if(SanitizeType&SANITIZE)
		str_sanitize(pPtr);
	else if(SanitizeType&SANITIZE_CC)
		str_sanitize_cc(pPtr);
	return SanitizeType&SKIP_START_WHITESPACES ? str_skip_whitespaces(pPtr) : pPtr;
}

const unsigned char *CUnpacker::GetRaw(int Size)
{
	const unsigned char *pPtr = m_pCurrent;
	if(m_Error)
		return 0;
	
	// check for nasty sizes
	if(Size < 0 || m_pCurrent+Size > m_pEnd)
	{
		m_Error = 1;
		return 0;
	}

	// "unpack" the data
	m_pCurrent += Size;
	return pPtr;
}
