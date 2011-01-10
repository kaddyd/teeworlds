#ifndef ENGINE_SHARED_TRANSLATE_H
#define ENGINE_SHARED_TRANSLATE_H

// warning: all these functions needs to free results through mem_free
class CTranslate
{
private:
	// unescape functions. shall not be used
	static char * UnescapeStep1(char * From);
	static char * UnescapeStep2(char * From);

	// unescape function. use it ;)
	static char * UnescapeStr(char * From);

	// escape string to \u0000-like
	static char * EscapeStrByLong(const char * From);
	
	// escape function
	static char * EscapeStr(const char * From);
public:
	static char * Translate(const char * String, const char * TargetLanguageCode = "en");
};

#endif