#include "translate.h"

#include <memory.h>

#include <base/system.h>
#include "network.h"

char * CTranslate::UnescapeStep1(char * From)
{
        unsigned long Len = str_length(From);
        char * Result = (char *)mem_alloc(Len * 4 + 1, 1);
        memset(Result, 0, Len * 4 + 1);
        
        unsigned long i = 0;
        unsigned long j = 0;
        while (i < Len)
        {
                if (From[i] == '\\')
                {
                        if (From[i + 1] == '\\')
                        {
                                Result[j++] = '\\';
                                i += 2;
                        } else {
                                if (From[i + 2] == 'u')
                                {
                                        unsigned long Code = 0;
                                        int q;
                                        for (q = 0; q < 4; q++)
                                        {
                                                if (From[i + 3 + q] >= '0' && From[i + 3 + q] <= '9')
                                                        q = (q * 16) + From[i + 3 + q] - '0';
                                                else if (From[i + 3 + q] >= 'a' && From[i + 3 + q] <= 'f')
                                                        q = (q * 16) + 10 + From[i + 3 + q] - 'a';
                                                else if (From[i + 3 + q] >= 'A' && From[i + 3 + q] <= 'F')
                                                        q = (q * 16) + 10 + From[i + 3 + q] - 'A';
                                                else break;
                                        }
                                        j += str_utf8_encode(Result, Code);
                                        i += 2 + q;
                                }
                        }
                } else {
                        Result[j++] = From[i++];
                }
        }
        
        return Result;
}

char * CTranslate::UnescapeStep2(char * From)
{
        unsigned long Len = str_length(From);
        char * Result = (char *)mem_alloc(Len * 4 + 1, 1);
        memset(Result, 0, Len * 4 + 1);
        
        unsigned long i = 0;
        unsigned long j = 0;
        while (i < Len)
        {
                if (From[i] == '%')
                {
                        if (From[i + 1] != '&')
                        {
                                Result[j++] = From[i++];
                        } else {
                                unsigned long Code = 0;
                                int q;
                                for (q = 0; q < 4; q++)
                                {
                                        if (From[i + 3 + q] >= '0' && From[i + 3 + q] <= '9')
                                                q = (q * 16) + From[i + 3 + q] - '0';
                                        else if (From[i + 3 + q] >= 'a' && From[i + 3 + q] <= 'f')
                                                q = (q * 16) + 10 + From[i + 3 + q] - 'a';
                                        else if (From[i + 3 + q] >= 'A' && From[i + 3 + q] <= 'F')
                                                q = (q * 16) + 10 + From[i + 3 + q] - 'A';
                                        else break;
                                }
                                j += str_utf8_encode(Result, Code);
                                i += 2 + q;
                                if (From[i] == ';') i++;
                        }
                } else {
                        Result[j++] = From[i++];
                }
        }
        
        return Result;
}

char * CTranslate::UnescapeStr(char * From)
{
        char * First = UnescapeStep1(From);
        char * Second = UnescapeStep2(First);
        mem_free((void *)First);
        return Second;
}

char * CTranslate::EscapeStrByLong(const char * From)
{
        unsigned long Len = str_length(From);
        unsigned long DestLen = Len * 6;
        char * Result = (char *)mem_alloc(DestLen + 1, 1);
        memset(Result, 0, DestLen + 1);
        
        unsigned long Char;
        const char * Text = From;
        char * ToText = Result;
        unsigned long i;
        while (Char = str_utf8_decode(&Text))
        {
                *(ToText++) = '\\';
                *(ToText++) = 'u';
                str_format(ToText, 5, "%04x", Char);
                ToText += 4;
        }
        
        return Result;
}

char * CTranslate::EscapeStr(const char * From)
{
        unsigned long Len = str_length(From);
        unsigned long DestLen = Len * 4;
        char * Result = (char *)mem_alloc(DestLen + 1, 1);
        memset(Result, 0, DestLen + 1);
        
        unsigned long Char;
        const char * Text = From;
        char * ToText = Result;
        unsigned long i;
        for (i = 0; i < Len; i++)
        {
                if ((From[i] >= 'a' && From[i] <= 'z') || (From[i] >= 'A' && From[i] <= 'Z') || (From[i] >= '0' && From[i] <= '9'))
                        *(ToText++) = From[i];
                else
                {
                        *(ToText++) = '%';
                        str_format(ToText, 5, "%02x", ((unsigned int)From[i])%0x100);
                        ToText += 2;
                }
        }
        
        return Result;
}

char * CTranslate::Translate(const char * String, const char * TargetLanguageCode)
{
	const int RequestSize = 4096;
	const int BufferSize = 8192;

	char * EscapedStr = EscapeStr(String);

	char * Request = (char *)mem_alloc(RequestSize + BufferSize, 1);
	char * Buffer = Request + RequestSize;
	str_format(Request, RequestSize, "/ajax/services/language/translate?v=1.0&q=%s&langpair=%%7C%s", EscapedStr, TargetLanguageCode);

	mem_free(EscapedStr);

	Buffer[0] = 0;
	HttpGet("ajax.googleapis.com", Request, Buffer, BufferSize);

	// check for source language
	// if source language equals target language, returns nothing
	str_format(Request, RequestSize, "\"detectedSourceLanguage\":\"%s\"", TargetLanguageCode);
	if (str_find_nocase(Buffer, Request))
	{
		mem_free(Request);
		return 0;
	}

	char * TranslatedText = (char *)str_find_nocase(Buffer, "\"translatedText\":\"");
	// nothing translated, nothing to return
	if (!TranslatedText)
	{
		mem_free(Request);
		return 0;
	}

	TranslatedText += 18; //str_length("\"translatedText\":\"");
	char * TranslationEnd = (char *)str_find_nocase(TranslatedText, "\"");
	// incomplete string
	if (!TranslationEnd)
	{
		mem_free(Request);
		return 0;
	}
	
	TranslationEnd[0] = 0;

	char * Result = UnescapeStr(TranslatedText);

	mem_free(Request);
	return Result;
}