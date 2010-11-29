#ifndef GAME_CLIENT_COMPONENTS_INFOPANEL_H
#define GAME_CLIENT_COMPONENTS_INFOPANEL_H

#include <game/client/component.h>

class CInfoPanel : public CComponent
{
	enum
	{
		MAX_LINES = 25,
	};

	struct CInfoLine
	{
		int64 m_Time;
		char m_aText[512];
	};

	CInfoLine m_aLines[MAX_LINES];
	int m_CurrentLine;

	static void ConInfoMsg(IConsole::IResult *pResult, void *pUserData);
public:
	CInfoPanel();

	void AddLine(const char *pLine);

	virtual void OnReset();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnConsoleInit();
	virtual void OnRender();
};


#endif
