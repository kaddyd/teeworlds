#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/client/gameclient.h>

#include "scoreboard.h"

#include "infopanel.h"

CInfoPanel::CInfoPanel()
{
	OnReset();
}

void CInfoPanel::OnReset()
{
	for (int i = 0; i < MAX_LINES; i++)
	{
		m_aLines[i].m_Time = 0;
		m_aLines[i].m_aText[0] = 0;
	}
}

void CInfoPanel::OnStateChange(int NewState, int OldState)
{
	if(OldState <= IClient::STATE_CONNECTING)
	{
		for(int i = 0; i < MAX_LINES; i++)
			m_aLines[i].m_Time = 0;
		m_CurrentLine = 0;
	}
}

void CInfoPanel::ConInfoMsg(IConsole::IResult *pResult, void *pUserData)
{
        ((CInfoPanel*)pUserData)->AddLine(pResult->GetString(0));
}

void CInfoPanel::OnConsoleInit()
{
	Console()->Register("infomsg", "r", CFGFLAG_CLIENT, ConInfoMsg, this, "Show message on info panel");
}

void CInfoPanel::AddLine(const char *pLine)
{
	char *p = const_cast<char*>(pLine);
	while(*p)
	{
		pLine = p;

		// find line seperator and strip multiline
		while(*p)
		{
			if(*p++ == '\n')
			{
				*(p-1) = 0;
				break;
			}
		}

		m_CurrentLine = (m_CurrentLine + 1) % MAX_LINES;
		m_aLines[m_CurrentLine].m_Time = time_get();

		str_copy(m_aLines[m_CurrentLine].m_aText, pLine, sizeof(m_aLines[m_CurrentLine].m_aText));

		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info", pLine);
	}
											
}

void CInfoPanel::OnRender()
{
	if (m_pClient->m_pScoreboard->Active())
		return;

	Graphics()->MapScreen(0,0,300*Graphics()->ScreenAspect(),300);
	float x = 10.0f;
	float y = 80.0f;

	int64 Now = time_get();
	float LineWidth = m_pClient->m_pScoreboard->Active() ? 95.0f : 200.0f;

	CTextCursor Cursor;

	TextRender()->TextColor(1.0f, 0.75f, 0.5f, 1.0f);

	for (int i = 0; i < MAX_LINES; i++)
	{
		int r = ((m_CurrentLine -  i) + MAX_LINES) % MAX_LINES;
		if(Now > m_aLines[r].m_Time + 15 * time_freq())
			break;

		if (y < 50.0f)
			break;

		TextRender()->SetCursor(&Cursor, x, y, 7.0f, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = LineWidth;

		TextRender()->TextEx(&Cursor, m_aLines[r].m_aText, -1);

		y += Cursor.m_Y - y - 7.0f;
	}

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
}
