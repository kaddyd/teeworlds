/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/render.h>
#include <game/client/components/motd.h>
#include <game/localization.h>
#include "scoreboard.h"
#include "hud.h"


CScoreboard::CScoreboard()
{
	OnReset();
}

void CScoreboard::ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	if (pResult->GetInteger(0) == 0)
	{
		((CScoreboard *)pUserData)->m_Active = 0;
	} else {
		if (g_Config.m_ClDetailedScoreboard)
		{
			((CScoreboard *)pUserData)->m_Active = 2;
		} else {
			((CScoreboard *)pUserData)->m_Active = 1;
		}
	}
}

void CScoreboard::ConKeyBasicScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	((CScoreboard *)pUserData)->m_Active = pResult->GetInteger(0) != 0 ? 1 : 0;
}


void CScoreboard::ConKeyDetailedScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	((CScoreboard *)pUserData)->m_Active = pResult->GetInteger(0) != 0 ? 2 : 0;
}

void CScoreboard::OnReset()
{
	m_Active = 0;
	
	m_DetailedScoreboardWidth = 900.0f;
}

void CScoreboard::OnRelease()
{
	m_Active = 0;
}

void CScoreboard::OnConsoleInit()
{
	Console()->Register("+scoreboard", "", CFGFLAG_CLIENT, ConKeyScoreboard, this, "Show scoreboard");
	Console()->Register("+basic_scoreboard", "", CFGFLAG_CLIENT, ConKeyBasicScoreboard, this, "Show basic scoreboard");
	Console()->Register("+detailed_scoreboard", "", CFGFLAG_CLIENT, ConKeyDetailedScoreboard, this, "Show detailed scoreboard");
}

void CScoreboard::RenderDetailedScoreboard()
{
	if (!m_pClient->m_Snap.m_pGameobj) return;

        float width = 400 * 3.0f * Graphics()->ScreenAspect();
        float height = 400*3.0f;

        float w = m_DetailedScoreboardWidth;
	float need_w = 1400.0f;
        float h = 900.0f;
	
	int numActiveWeapons = 0;
	bool activeWeapons[NUM_WEAPONS];
	
	for (int j = 0; j < NUM_WEAPONS; j++) activeWeapons[j] = false;
	
	if (g_Config.m_ClDetailedScoreboardFull)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (!m_pClient->m_Snap.m_paPlayerInfos[i]) continue;
			for (int j = 0; j < NUM_WEAPONS; j++)
			{
				if (m_pClient->m_aClients[i].m_Stats.m_aKills[j] != 0 || m_pClient->m_aClients[i].m_Stats.m_aKilled[j] != 0)
				{
					activeWeapons[j] = true;
				}
			}
		}
		for (int j = 0; j < NUM_WEAPONS; j++)
			if (activeWeapons[j])
				numActiveWeapons++;
			
		need_w -= ((need_w - 30.0f - 50.0f - 350.0f - 125.0f - 100.0f) / (NUM_WEAPONS + 2)) * (NUM_WEAPONS - numActiveWeapons);
	} else {
		need_w -= ((need_w - 30.0f - 50.0f - 350.0f - 125.0f - 100.0f) / (NUM_WEAPONS + 2)) * (NUM_WEAPONS);
	}
	
	
	if (fabs(w - need_w) < 7.0f) w = need_w;
	else w = w * 0.9f + need_w * 0.1f;
	
	m_DetailedScoreboardWidth = w;

        float x = (width - w) / 2;
        float y = (height - h) / 2;

        Graphics()->MapScreen(0, 0, width, height);

        CUIRect mainView;
        mainView.x = x;
        mainView.y = y;
        mainView.w = w;
        mainView.h = h;

        RenderTools()->DrawUIRect(&mainView, vec4(0.0f, 0.0f, 0.0f, 0.5f), CUI::CORNER_ALL, 10.0f);

        CUIRect header = mainView, footer = mainView, baseRect = mainView;

        header.HSplitTop(40.0f, &header, &mainView);
        RenderTools()->DrawUIRect(&header, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_T, 10.0f);

        footer.HSplitBottom(35.0f, &mainView, &footer);
        RenderTools()->DrawUIRect(&footer, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_B, 10.0f);

        header.VSplitLeft(15.0f, 0, &header);
        header.VSplitRight(15.0f, &header, 0);

        mainView.Margin(5.0f, &mainView);

        header.VSplitLeft(15.0f, 0, &mainView);
        header.VSplitRight(15.0f, &mainView, 0);

        footer.VSplitLeft(25.0f, 0, &footer);
        footer.VSplitRight(25.0f, &footer, 0);

        //mainView.w += 15.0f;
	
	{
		char aBuf[256];
		
		if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_ScoreLimit)
		{
			str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_Snap.m_pGameobj->m_ScoreLimit);
			UI()->DoLabel(&footer, aBuf, footer.h * 0.8f, -1);
		}

		footer.VSplitLeft(400.0f, 0, &footer);

		if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_TimeLimit)
		{
			str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), m_pClient->m_Snap.m_pGameobj->m_TimeLimit);
			UI()->DoLabel(&footer, aBuf, footer.h * 0.8f, -1);
		}

		footer.VSplitLeft(800.0f, 0, &footer);

		if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_RoundNum && m_pClient->m_Snap.m_pGameobj->m_RoundCurrent)
		{
			str_format(aBuf, sizeof(aBuf), Localize("Round: %d/%d"), m_pClient->m_Snap.m_pGameobj->m_RoundCurrent, m_pClient->m_Snap.m_pGameobj->m_RoundNum);
			UI()->DoLabel(&footer, aBuf, footer.h * 0.8f, -1);
		}
	}

        float headerWidth = header.w;

        header.VSplitLeft(50.0f, 0, &header);
        UI()->DoLabel(&header, Localize("Name"), header.h * 0.8f, -1);


        header.VSplitLeft(350.0f, 0, &header);

        {
                CUIRect line_t = header;
                line_t.x += abs(125.0f - TextRender()->TextWidth(0, header.h * 0.8f, Localize("Score"), -1)) / 2.0f;
                UI()->DoLabel(&line_t, Localize("Score"), header.h * 0.8f, -1);
        }

        header.VSplitLeft(125.0f, 0, &header);

        {
                CUIRect line_t = header;
                line_t.x += abs(100.0f - TextRender()->TextWidth(0, header.h * 0.8f, Localize("Ping"), -1)) / 2.0f;
                UI()->DoLabel(&line_t, Localize("Ping"), header.h * 0.8f, -1);
        }

        header.VSplitLeft(115.0f, 0, &header);

        float spriteSize = header.h * 0.8f;
        float spacing;
	if (g_Config.m_ClDetailedScoreboardFull)
	{
		spacing = (headerWidth - 50.0f - 350.0f - 125.0f - 100.0f) / (numActiveWeapons + 2);
	} else {
		spacing = (headerWidth - 50.0f - 350.0f - 125.0f - 100.0f) / 2;
	}

        Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
        Graphics()->QuadsBegin();

        Graphics()->QuadsSetRotation(0);

        {
                RenderTools()->SelectSprite(&g_pData->m_aSprites[SPRITE_STAR1]);
		IGraphics::CQuadItem QuadItem(header.x + (spacing - spriteSize) * 0.5f, header.y + spriteSize * 0.5f + header.h * 0.1f, spriteSize, spriteSize);
                Graphics()->QuadsDraw(&QuadItem, 1);
                header.VSplitLeft(spacing, 0, &header);
        }
        {
                RenderTools()->SelectSprite(&g_pData->m_aSprites[SPRITE_RED_MINUS]);
		IGraphics::CQuadItem QuadItem(header.x + (spacing - spriteSize) * 0.5f, header.y + spriteSize * 0.5f + header.h * 0.1f, spriteSize, spriteSize);
		Graphics()->QuadsDraw(&QuadItem, 1);
                header.VSplitLeft(spacing, 0, &header);
        }

	if (g_Config.m_ClDetailedScoreboardFull)
	{
		for (int i = 0; i < NUM_WEAPONS; i++)
		{
			if (!activeWeapons[i]) continue;
			
			RenderTools()->SelectSprite((i == WEAPON_HAMMER || i == WEAPON_NINJA) ? g_pData->m_Weapons.m_aId[i].m_pSpriteBody : g_pData->m_Weapons.m_aId[i].m_pSpriteProj);

			float sw = i != WEAPON_NINJA ? spriteSize : spriteSize * 2.0f;

			IGraphics::CQuadItem QuadItem(header.x + (spacing - sw) * 0.5f, header.y + spriteSize / 2.0f + header.h * 0.1f, sw, spriteSize);
			Graphics()->QuadsDraw(&QuadItem, 1);

			header.VSplitLeft(spacing, 0, &header);
		}
	}

        Graphics()->QuadsEnd();

        mainView.y = y + 35.0f;
        mainView.h = h - 70.0f - 70.0f * 3 - 15.0f;

        float lineHeight = mainView.h / (float)MAX_CLIENTS;

        for (int team = 0; team <= 2; team++)
        {
                if (!(m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_TEAMS) && team == 1)
			continue;

                const CNetObj_PlayerInfo *players[MAX_CLIENTS] = {0};
                int numPlayers = 0;
                for(int i = 0; i < Client()->SnapNumItems(IClient::SNAP_CURRENT); i++)
                {
                        IClient::CSnapItem Item;
                        const void * pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

                        if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
                        {
                                const CNetObj_PlayerInfo * pInfo = (const CNetObj_PlayerInfo *)pData;
                                if(pInfo->m_Team == (team == 2 ? -1 : team))
                                {
                                        players[numPlayers] = pInfo;
                                        numPlayers++;
                                }
                        }
                }

                if (team == 2 && numPlayers == 0) continue;

                // sort players
                for(int k = 0; k < numPlayers; k++) // ffs, bubblesort
                {
                        for(int i = 0; i < numPlayers-k-1; i++)
                        {
                                if(players[i]->m_Score < players[i+1]->m_Score)
                                {
                                        const CNetObj_PlayerInfo *tmp = players[i];
                                        players[i] = players[i+1];
                                        players[i+1] = tmp;
                                }
                        }
                }

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
                if (m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_TEAMS)
                {
                        if (team == 0 || team == 1)
                        {
				if (g_Config.m_ClColorNicks && m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_TEAMS && team >= 0)
				{
					if (team)
					{
						TextRender()->TextColor(0.7f, 0.7f, 1.0f, 1.0f);
					} else {
						TextRender()->TextColor(1.0f, 0.5f, 0.5f, 1.0f);
					}
				}
				
                                CUIRect line = mainView;

                                line.VSplitLeft((headerWidth / 2) / 5, 0, &line);

                                if (team == 0)
                                        UI()->DoLabel(&line, Localize("Red team"), lineHeight * 0.8f * 2.0f, -1);
                                else if (team == 1)
                                        UI()->DoLabel(&line, Localize("Blue team"), lineHeight * 0.8f * 2.0f, -1);

                                CUIRect line2 = line;

                                char aBuf[64];
                                int score = team ? m_pClient->m_Snap.m_pGameobj->m_TeamscoreBlue : m_pClient->m_Snap.m_pGameobj->m_TeamscoreRed;
                                str_format(aBuf, sizeof(aBuf), "%d", score);

                                line2.x += line.w - 125.0f - TextRender()->TextWidth(0, lineHeight * 0.8f * 2.0f, aBuf, -1) * 0.5f;
                                UI()->DoLabel(&line2, aBuf, lineHeight * 0.8f * 2.0f, -1);

                                line2 = line;
                                str_format(aBuf, sizeof(aBuf), "%d", numPlayers);
                                line2.x += line.w * 0.65f - TextRender()->TextWidth(0, lineHeight * 0.8f * 1.0f, aBuf, -1) * 0.5f;
                                line2.y += lineHeight * 0.5f;
                                line2.h = line.h - line2.y + line.y;
                                UI()->DoLabel(&line2, aBuf, lineHeight * 0.8f * 1.00f, -1);

                                mainView.HSplitTop(lineHeight * 2.0f, 0, &mainView);
                        }
                } else {
                        if (team == 0)
                        {
                                CUIRect line = mainView;
                                line.VSplitLeft((headerWidth / 2) / 5, 0, &line);
                                UI()->DoLabel(&line, Localize("Players"), lineHeight * 0.8f * 2.0f, -1);
                                CUIRect line2 = line;

                                line2 = line;
                                char aBuf[64];
                                str_format(aBuf, sizeof(aBuf), "%d", numPlayers);
                                line2.x += line.w * 0.65f - TextRender()->TextWidth(0, lineHeight * 0.8f * 1.0f, aBuf, -1) * 0.5f;
                                line2.y += lineHeight * 0.5f;
                                line2.h = line.h - line2.y + line.y;
                                UI()->DoLabel(&line2, aBuf, lineHeight * 0.8f * 1.00f, -1);

                                mainView.HSplitTop(lineHeight * 2.0f, 0, &mainView);
                        }
                }

                if (team == 2)
                {
                        mainView.y = y + h - 35.0f - 15.0f;
                        mainView.h = lineHeight * 2.0f + lineHeight * numPlayers;
                        mainView.y -= mainView.h;

                        CUIRect line = mainView;
                        line.VSplitLeft((headerWidth / 2) / 5, 0, &line);
                        UI()->DoLabel(&line, Localize("Spectators"), lineHeight * 0.8f * 2.0f, -1);

                        CUIRect line2 = line;

                        line2 = line;
                        char aBuf[64];
                        str_format(aBuf, sizeof(aBuf), "%d", numPlayers);
                        line2.x += abs(400.0f - TextRender()->TextWidth(0, lineHeight * 0.8f * 1.0f, aBuf, -1)) / 2.0f;
                        line2.w = line.w - line2.x + line.x;
                        line2.y += lineHeight * 0.5f;
                        line2.h = line.h - line2.y + line.y;
                        UI()->DoLabel(&line2, aBuf, lineHeight * 0.8f * 1.00f, -1);

                        mainView.HSplitTop(lineHeight * 2.0f, 0, &mainView);
                }

                for (int i = 0; i < numPlayers; i++)
                {
                        char aBuf[64];

                        const CNetObj_PlayerInfo * pInfo = players[i];
			
			if (g_Config.m_ClColorNicks)
			{
				vec3 color = m_pClient->m_pHud->GetNickColor(pInfo);
				TextRender()->TextColor(color.r, color.g, color.b, 1.0f);
			}

                        CUIRect line = baseRect;
			line.y = mainView.y;
                        line.h = lineHeight;
			
			line.VSplitLeft(15.0f, 0, &line);

                        if (pInfo->m_Local)
			{
				CUIRect line = mainView;
				line.x = baseRect.x;
				line.w = baseRect.w;
				line.h = lineHeight;
                                RenderTools()->DrawUIRect(&line, vec4(1.0f, 1.0f, 1.0f, 0.25f), 0, 0);
			} else if (i%2 == 0) {
				CUIRect line = mainView;
				line.x = baseRect.x;
				line.w = baseRect.w;
				line.h = lineHeight;
                                RenderTools()->DrawUIRect(&line, vec4(1.0f, 1.0f, 1.0f, 0.02f), 0, 0);
			}

			if (team != 2)
			{
				CTeeRenderInfo teeRenderInfo = m_pClient->m_aClients[pInfo->m_ClientId].m_RenderInfo;
				teeRenderInfo.m_Size *= lineHeight / 64.0f;

				RenderTools()->RenderTee(CAnimState::GetIdle(), &teeRenderInfo, EMOTE_NORMAL, vec2(1,0), vec2(line.x + abs(75.0f - teeRenderInfo.m_Size) / 2.0f, line.y + teeRenderInfo.m_Size / 2.0f));
			}

                        line.VSplitLeft(50.0f, 0, &line);

                        UI()->DoLabel(&line, m_pClient->m_aClients[pInfo->m_ClientId].m_aName, lineHeight * 0.8f, -1);

                        line.VSplitLeft(350.0f, 0, &line);

                        if (team != 2)
                        {
                                CUIRect line_t = line;
                                str_format(aBuf, sizeof(aBuf), "%d", pInfo->m_Score);
                                line_t.x += abs(125.0f - TextRender()->TextWidth(0, lineHeight * 0.8f, aBuf, -1)) * 0.5f;
                                UI()->DoLabel(&line_t, aBuf, lineHeight * 0.8f, -1);
                        }

                        line.VSplitLeft(125.0f, 0, &line);

                        {
                                CUIRect line_t = line;
                                str_format(aBuf, sizeof(aBuf), "%d", pInfo->m_Latency);
                                line_t.x += abs(100.0f - TextRender()->TextWidth(0, lineHeight * 0.8f, aBuf, -1)) * 0.5f;
                                UI()->DoLabel(&line_t, aBuf, lineHeight * 0.8f, -1);
                        }
                        
                        line.VSplitLeft(100.0f, 0, &line);

                        if (team != 2)
                        {
                                {
                                        CUIRect line_t = line;
					if (m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_TotalKills == 0 &&
					    m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_TotalKilled == 0)
						str_format(aBuf, sizeof(aBuf), "---");
					else
						str_format(aBuf, sizeof(aBuf), "%d/%.1f", m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_TotalKills - m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_TotalKilled, (float)m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_TotalKills / (float)(m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_TotalKilled == 0 ? 1 : m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_TotalKilled));
                                        line_t.x += (spacing - TextRender()->TextWidth(0, lineHeight * 0.8f, aBuf, -1)) * 0.5f;
                                        UI()->DoLabel(&line_t, aBuf, lineHeight * 0.8f, -1);
                                }

                                line.VSplitLeft(spacing, 0, &line);
				
                                {
                                        CUIRect line_t = line;
					if (m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_TotalKills == 0 &&
					    m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_TotalKilled == 0)
						str_format(aBuf, sizeof(aBuf), "---");
					else
						str_format(aBuf, sizeof(aBuf), "%d/%d", m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_TotalKills, m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_TotalKilled);
                                        line_t.x += (spacing - TextRender()->TextWidth(0, lineHeight * 0.8f, aBuf, -1)) * 0.5f;
                                        UI()->DoLabel(&line_t, aBuf, lineHeight * 0.8f, -1);
                                }
                                
                                line.VSplitLeft(spacing, 0, &line);

				if (g_Config.m_ClDetailedScoreboardFull)
				{
					for (int i = 0; i < NUM_WEAPONS; i++)
					{
						if (!activeWeapons[i]) continue;
						
						CUIRect line_t = line;
						str_format(aBuf, sizeof(aBuf), "%d/%d", m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_aKills[i], m_pClient->m_aClients[pInfo->m_ClientId].m_Stats.m_aKilled[i]);
						line_t.x += (spacing - TextRender()->TextWidth(0, lineHeight * 0.8f, aBuf, -1)) * 0.5f;
						UI()->DoLabel(&line_t, aBuf, lineHeight * 0.8f, -1);

						line.VSplitLeft(spacing, 0, &line);
					}
				}
                        }
                        mainView.HSplitTop(lineHeight, 0, &mainView);
                }
        }
        
        TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CScoreboard::RenderGoals(float x, float y, float w)
{
	float h = 50.0f;

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y-10.f, w, h, 10.0f);
	Graphics()->QuadsEnd();

	// render goals
	//y = ystart+h-54;
	float tw = 0.0f;
	if(m_pClient->m_Snap.m_pGameobj)
	{
		if(m_pClient->m_Snap.m_pGameobj->m_ScoreLimit)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_Snap.m_pGameobj->m_ScoreLimit);
			TextRender()->Text(0, x+20.0f, y, 22.0f, aBuf, -1);
			tw += TextRender()->TextWidth(0, 22.0f, aBuf, -1);
		}
		if(m_pClient->m_Snap.m_pGameobj->m_TimeLimit)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), m_pClient->m_Snap.m_pGameobj->m_TimeLimit);
			TextRender()->Text(0, x+220.0f, y, 22.0f, aBuf, -1);
			tw += TextRender()->TextWidth(0, 22.0f, aBuf, -1);
		}
		if(m_pClient->m_Snap.m_pGameobj->m_RoundNum && m_pClient->m_Snap.m_pGameobj->m_RoundCurrent)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s %d/%d", Localize("Round"), m_pClient->m_Snap.m_pGameobj->m_RoundCurrent, m_pClient->m_Snap.m_pGameobj->m_RoundNum);
			TextRender()->Text(0, x+450.0f, y, 22.0f, aBuf, -1);
		}
	}
}

void CScoreboard::RenderSpectators(float x, float y, float w)
{
	char aBuffer[1024*4];
	int Count = 0;
	float h = 120.0f;
	
	str_format(aBuffer, sizeof(aBuffer), "%s: ", Localize("Spectators"));

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y-10.f, w, h, 10.0f);
	Graphics()->QuadsEnd();
	
	for(int i = 0; i < Client()->SnapNumItems(IClient::SNAP_CURRENT); i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
		{
			const CNetObj_PlayerInfo *pInfo = (const CNetObj_PlayerInfo *)pData;
			if(pInfo->m_Team == -1)
			{
				if(Count)
					str_append(aBuffer, ", ", sizeof(aBuffer));
				str_append(aBuffer, m_pClient->m_aClients[pInfo->m_ClientId].m_aName, sizeof(aBuffer));
				Count++;
			}
		}
	}
	
	TextRender()->Text(0, x+10, y, 32, aBuffer, (int)w-20);
}

void CScoreboard::RenderScoreboard(float x, float y, float w, int Team, const char *pTitle)
{
	//float ystart = y;
	float h = 750.0f;

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y-10.f, w, h, 17.0f);
	Graphics()->QuadsEnd();

	// render title
	if(!pTitle)
	{
		if(m_pClient->m_Snap.m_pGameobj->m_GameOver)
			pTitle = Localize("Game over");
		else
			pTitle = Localize("Score board");
	} else {
		if (g_Config.m_ClColorNicks && m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_TEAMS && Team >= 0)
		{
			if (Team)
			{
				TextRender()->TextColor(0.7f, 0.7f, 1.0f, 1.0f);
			} else {
				TextRender()->TextColor(1.0f, 0.5f, 0.5f, 1.0f);
			}
		}
	}

	float tw = TextRender()->TextWidth(0, 48, pTitle, -1);

	if(Team == -1)
	{
		TextRender()->Text(0, x+w/2-tw/2, y, 48, pTitle, -1);
	}
	else
	{
		TextRender()->Text(0, x+10, y, 48, pTitle, -1);

		if(m_pClient->m_Snap.m_pGameobj)
		{
			char aBuf[128];
			int Score = Team ? m_pClient->m_Snap.m_pGameobj->m_TeamscoreBlue : m_pClient->m_Snap.m_pGameobj->m_TeamscoreRed;
			str_format(aBuf, sizeof(aBuf), "%d", Score);
			tw = TextRender()->TextWidth(0, 48, aBuf, -1);
			TextRender()->Text(0, x+w-tw-30, y, 48, aBuf, -1);
		}
	}

	y += 54.0f;

	// find players
	const CNetObj_PlayerInfo *paPlayers[MAX_CLIENTS] = {0};
	int NumPlayers = 0;
	for(int i = 0; i < Client()->SnapNumItems(IClient::SNAP_CURRENT); i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
		{
			const CNetObj_PlayerInfo *pInfo = (const CNetObj_PlayerInfo *)pData;
			if(pInfo->m_Team == Team)
			{
				paPlayers[NumPlayers] = pInfo;
				if(++NumPlayers == MAX_CLIENTS)
					break;
			}
		}
	}

	// sort players
	for(int k = 0; k < NumPlayers-1; k++) // ffs, bubblesort
	{
		for(int i = 0; i < NumPlayers-k-1; i++)
		{
			if(paPlayers[i]->m_Score < paPlayers[i+1]->m_Score)
			{
				const CNetObj_PlayerInfo *pTmp = paPlayers[i];
				paPlayers[i] = paPlayers[i+1];
				paPlayers[i+1] = pTmp;
			}
		}
	}

	// render headlines
	TextRender()->Text(0, x+10, y, 24.0f, Localize("Score"), -1);
	TextRender()->Text(0, x+125, y, 24.0f, Localize("Name"), -1);
	TextRender()->Text(0, x+w-70, y, 24.0f, Localize("Ping"), -1);
	y += 29.0f;

	float FontSize = 35.0f;
	float LineHeight = 50.0f;
	float TeeSizeMod = 1.0f;
	float TeeOffset = 0.0f;
	
	if(NumPlayers > 13)
	{
		FontSize = 30.0f;
		LineHeight = 40.0f;
		TeeSizeMod = 0.8f;
		TeeOffset = -5.0f;
	}
	
	// render player scores
	for(int i = 0; i < NumPlayers; i++)
	{
		const CNetObj_PlayerInfo *pInfo = paPlayers[i];

		// make sure that we render the correct team

		char aBuf[128];
		if(pInfo->m_Local)
		{
			// background so it's easy to find the local player
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1,1,1,0.25f);
			RenderTools()->DrawRoundRect(x, y, w-20, LineHeight*0.95f, 17.0f);
			Graphics()->QuadsEnd();
		}

		float FontSizeResize = FontSize;
		float Width;
		const float ScoreWidth = 60.0f;
		const float PingWidth = 60.0f;

		if (g_Config.m_ClColorNicks)
		{
			vec3 color = m_pClient->m_pHud->GetNickColor(pInfo);
			TextRender()->TextColor(color.r, color.g, color.b, 1.0f);
		}

		str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Score, -9999, 9999));
		while((Width = TextRender()->TextWidth(0, FontSizeResize, aBuf, -1)) > ScoreWidth)
			--FontSizeResize;
		TextRender()->Text(0, x+ScoreWidth-Width, y+(FontSize-FontSizeResize)/2, FontSizeResize, aBuf, -1);
		
		FontSizeResize = FontSize;
		while(TextRender()->TextWidth(0, FontSizeResize, m_pClient->m_aClients[pInfo->m_ClientId].m_aName, -1) > w-163.0f-PingWidth)
			--FontSizeResize;
		TextRender()->Text(0, x+128.0f, y+(FontSize-FontSizeResize)/2, FontSizeResize, m_pClient->m_aClients[pInfo->m_ClientId].m_aName, -1);

		FontSizeResize = FontSize;
		str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Latency, -9999, 9999));
		while((Width = TextRender()->TextWidth(0, FontSizeResize, aBuf, -1)) > PingWidth)
			--FontSizeResize;
		TextRender()->Text(0, x+w-35.0f-Width, y+(FontSize-FontSizeResize)/2, FontSizeResize, aBuf, -1);

		// render avatar
		if((m_pClient->m_Snap.m_paFlags[0] && m_pClient->m_Snap.m_paFlags[0]->m_CarriedBy == pInfo->m_ClientId) ||
			(m_pClient->m_Snap.m_paFlags[1] && m_pClient->m_Snap.m_paFlags[1]->m_CarriedBy == pInfo->m_ClientId))
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			if(pInfo->m_Team == 0) RenderTools()->SelectSprite(SPRITE_FLAG_BLUE, SPRITE_FLAG_FLIP_X);
			else RenderTools()->SelectSprite(SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);
			
			float size = 64.0f;
			IGraphics::CQuadItem QuadItem(x+55, y-15, size/2, size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}
		
		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientId].m_RenderInfo;
		TeeInfo.m_Size *= TeeSizeMod;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1,0), vec2(x+90, y+28+TeeOffset));

		
		y += LineHeight;
	}

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CScoreboard::RenderRecordingNotification(float x)
{
	if(!m_pClient->DemoRecorder()->IsRecording())
		return;

	//draw the box
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.4f);
	RenderTools()->DrawRoundRectExt(x, 0.0f, 120.0f, 50.0f, 15.0f, CUI::CORNER_B);
	Graphics()->QuadsEnd();

	//draw the red dot
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
	RenderTools()->DrawRoundRect(x+20, 15.0f, 20.0f, 20.0f, 10.0f);
	Graphics()->QuadsEnd();

	//draw the text
	TextRender()->Text(0, x+50.0f, 8.0f, 24.0f, Localize("REC"), -1);
}

void CScoreboard::OnRender()
{
	bool DoScoreBoard = false;

	// if we activly wanna look on the scoreboard	
	if(m_Active)
		DoScoreBoard = true;
		
	if(m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pLocalInfo->m_Team != -1)
	{
		// we are not a spectator, check if we are ead
		if(!m_pClient->m_Snap.m_pLocalCharacter || m_pClient->m_Snap.m_pLocalCharacter->m_Health < 0)
			DoScoreBoard = true;
	}

	// if we the game is over
	if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_GameOver)
		DoScoreBoard = true;
		
	if(!DoScoreBoard)
		return;
		
	// if the score board is active, then we should clear the motd message aswell
	if(m_pClient->m_pMotd->IsActive())
		m_pClient->m_pMotd->Clear();

	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;
	
	Graphics()->MapScreen(0, 0, Width, Height);

	float w = 650.0f;
	
	if (m_Active == 2 || (g_Config.m_ClDetailedScoreboard && DoScoreBoard))
	{
		RenderDetailedScoreboard();
		RenderRecordingNotification((Width / 7) * 4);
	} else {
		if(m_pClient->m_Snap.m_pGameobj && !(m_pClient->m_Snap.m_pGameobj->m_Flags&GAMEFLAG_TEAMS))
		{
			RenderScoreboard(Width/2-w/2, 150.0f, w, 0, 0);
			//render_scoreboard(gameobj, 0, 0, -1, 0);
		}
		else
		{
				
			if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_GameOver)
			{
				const char *pText = Localize("Draw!");
				if(m_pClient->m_Snap.m_pGameobj->m_TeamscoreRed > m_pClient->m_Snap.m_pGameobj->m_TeamscoreBlue)
					pText = Localize("Red team wins!");
				else if(m_pClient->m_Snap.m_pGameobj->m_TeamscoreBlue > m_pClient->m_Snap.m_pGameobj->m_TeamscoreRed)
					pText = Localize("Blue team wins!");
					
				float w = TextRender()->TextWidth(0, 92.0f, pText, -1);
				TextRender()->Text(0, Width/2-w/2, 45, 92.0f, pText, -1);
			}
			
			RenderScoreboard(Width/2-w-20, 150.0f, w, 0, Localize("Red team"));
			RenderScoreboard(Width/2 + 20, 150.0f, w, 1, Localize("Blue team"));
		}

		RenderGoals(Width/2-w/2, 150+750+25, w);
		RenderSpectators(Width/2-w/2, 150+750+25+50+25, w);
		RenderRecordingNotification((Width/7)*4);
	}
}

bool CScoreboard::Active()
{
	return (m_Active != 0) | (m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_GameOver);
}
