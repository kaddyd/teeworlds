/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <math.h>

#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include "menus.h"
#include "skins.h"

#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/serverbrowser.h>
#include <engine/keys.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include <game/version.h>
#include <game/generated/protocol.h>

#include <game/generated/client_data.h>
#include <game/client/gameclient.h>
#include <game/client/lineinput.h>
#include <game/localization.h>
#include <mastersrv/mastersrv.h>

#include <game/client/animstate.h>

vec4 CMenus::ms_GuiColor;
vec4 CMenus::ms_ColorTabbarInactiveOutgame;
vec4 CMenus::ms_ColorTabbarActiveOutgame;
vec4 CMenus::ms_ColorTabbarInactive;
vec4 CMenus::ms_ColorTabbarActive;
vec4 CMenus::ms_ColorTabbarInactiveIngame;
vec4 CMenus::ms_ColorTabbarActiveIngame;


float CMenus::ms_ButtonHeight = 25.0f;
float CMenus::ms_ListheaderHeight = 17.0f;
float CMenus::ms_FontmodHeight = 0.8f;

IInput::CEvent CMenus::m_aInputEvents[MAX_INPUTEVENTS];
int CMenus::m_NumInputEvents;


CMenus::CMenus()
{
	m_Popup = POPUP_NONE;
	m_ActivePage = PAGE_INTERNET;
	m_GamePage = PAGE_GAME;
	
	m_NeedRestartGraphics = false;
	m_NeedRestartSound = false;
	m_NeedSendinfo = false;
	m_MenuActive = true;
	m_UseMouseButtons = true;
	m_DemolistDelEntry = false;
	
	m_EscapePressed = false;
	m_EnterPressed = false;
	m_DeletePressed = false;
	m_NumInputEvents = 0;
	
	m_LastInput = time_get();
	
	str_copy(m_aCurrentDemoFolder, "demos", sizeof(m_aCurrentDemoFolder));
	m_aCallvoteReason[0] = 0;
}

vec4 CMenus::ButtonColorMul(const void *pID)
{
	if(UI()->ActiveItem() == pID)
		return vec4(1,1,1,0.5f);
	else if(UI()->HotItem() == pID)
		return vec4(1,1,1,1.5f);
	return vec4(1,1,1,1);
}

int CMenus::DoButton_Icon(int ImageId, int SpriteId, const CUIRect *pRect)
{
	Graphics()->TextureSet(g_pData->m_aImages[ImageId].m_Id);
	
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SpriteId);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
	
	return 0;
}


int CMenus::DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	RenderTools()->DrawUIRect(pRect, vec4(1,1,1,0.5f)*ButtonColorMul(pID), CUI::CORNER_ALL, 5.0f);
	UI()->DoLabel(pRect, pText, pRect->h*ms_FontmodHeight, 0);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

void CMenus::DoButton_KeySelect(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	RenderTools()->DrawUIRect(pRect, vec4(1,1,1,0.5f)*ButtonColorMul(pID), CUI::CORNER_ALL, 5.0f);
	UI()->DoLabel(pRect, pText, pRect->h*ms_FontmodHeight, 0);
}

int CMenus::DoButton_MenuTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners)
{
	if(Checked)
		RenderTools()->DrawUIRect(pRect, ms_ColorTabbarActive, Corners, 10.0f);
	else
		RenderTools()->DrawUIRect(pRect, ms_ColorTabbarInactive, Corners, 10.0f);
	UI()->DoLabel(pRect, pText, pRect->h*ms_FontmodHeight, 0);
	
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int CMenus::DoButton_GridHeader(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
//void CMenus::ui_draw_grid_header(const void *id, const char *text, int checked, const CUIRect *r, const void *extra)
{
	if(Checked)
		RenderTools()->DrawUIRect(pRect, vec4(1,1,1,0.5f), CUI::CORNER_T, 5.0f);
	CUIRect t;
	pRect->VSplitLeft(5.0f, 0, &t);
	UI()->DoLabel(&t, pText, pRect->h*ms_FontmodHeight, -1);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int CMenus::DoButton_CheckBox_Common(const void *pID, const char *pText, const char *pBoxText, const CUIRect *pRect)
//void CMenus::ui_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const CUIRect *r, const void *extra)
{
	CUIRect c = *pRect;
	CUIRect t = *pRect;
	c.w = c.h;
	t.x += c.w;
	t.w -= c.w;
	t.VSplitLeft(5.0f, 0, &t);
	
	c.Margin(2.0f, &c);
	RenderTools()->DrawUIRect(&c, vec4(1,1,1,0.25f)*ButtonColorMul(pID), CUI::CORNER_ALL, 3.0f);
	c.y += 2;
	UI()->DoLabel(&c, pBoxText, pRect->h*ms_FontmodHeight*0.6f, 0);
	UI()->DoLabel(&t, pText, pRect->h*ms_FontmodHeight*0.8f, -1);
	return UI()->DoButtonLogic(pID, pText, 0, pRect);
}

int CMenus::DoButton_CheckBox(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	return DoButton_CheckBox_Common(pID, pText, Checked?"X":"", pRect);
}


int CMenus::DoButton_CheckBox_Number(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	char aBuf[16];
	str_format(aBuf, sizeof(aBuf), "%d", Checked);
	return DoButton_CheckBox_Common(pID, pText, aBuf, pRect);
}

int CMenus::DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *Offset, bool Hidden, int Corners)
{
    int Inside = UI()->MouseInside(pRect);
	bool ReturnValue = false;
	bool UpdateOffset = false;
	static int s_AtIndex = 0;
	static bool s_DoScroll = false;
	static float s_ScrollStart = 0.0f;

	if(UI()->LastActiveItem() == pID)
	{
		int Len = str_length(pStr);
		if(Len == 0)
			s_AtIndex = 0;
			
		if(Inside && UI()->MouseButton(0))
		{
			s_DoScroll = true;
			s_ScrollStart = UI()->MouseX();
			int MxRel = (int)(UI()->MouseX() - pRect->x);

			for(int i = 1; i <= Len; i++)
			{
				if(TextRender()->TextWidth(0, FontSize, pStr, i) - *Offset + 10 > MxRel)
				{
					s_AtIndex = i - 1;
					break;
				}

				if(i == Len)
					s_AtIndex = Len;
			}
		}
		else if(!UI()->MouseButton(0))
			s_DoScroll = false;
		else if(s_DoScroll)
		{
			// do scrolling
			if(UI()->MouseX() < pRect->x && s_ScrollStart-UI()->MouseX() > 10.0f)
			{
				s_AtIndex = max(0, s_AtIndex-1);
				s_ScrollStart = UI()->MouseX();
				UpdateOffset = true;
			}
			else if(UI()->MouseX() > pRect->x+pRect->w && UI()->MouseX()-s_ScrollStart > 10.0f)
			{
				s_AtIndex = min(Len, s_AtIndex+1);
				s_ScrollStart = UI()->MouseX();
				UpdateOffset = true;
			}
		}

		for(int i = 0; i < m_NumInputEvents; i++)
		{
			Len = str_length(pStr);
			ReturnValue |= CLineInput::Manipulate(m_aInputEvents[i], pStr, StrSize, &Len, &s_AtIndex);
		}
	}

	bool JustGotActive = false;
	
	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0))
		{
			s_DoScroll = false;
			UI()->SetActiveItem(0);
		}
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			if (UI()->LastActiveItem() != pID)
				JustGotActive = true;
			UI()->SetActiveItem(pID);
		}
	}
	
	if(Inside)
		UI()->SetHotItem(pID);

	CUIRect Textbox = *pRect;
	RenderTools()->DrawUIRect(&Textbox, vec4(1, 1, 1, 0.5f), Corners, 3.0f);
	Textbox.VMargin(2.0f, &Textbox);
	Textbox.HMargin(2.0f, &Textbox);
	
	const char *pDisplayStr = pStr;
	char aStars[128];
	
	if(Hidden)
	{
		unsigned s = str_length(pStr);
		if(s >= sizeof(aStars))
			s = sizeof(aStars)-1;
		for(unsigned int i = 0; i < s; ++i)
			aStars[i] = '*';
		aStars[s] = 0;
		pDisplayStr = aStars;
	}

	// check if the text has to be moved
	if(UI()->LastActiveItem() == pID && !JustGotActive && (UpdateOffset || m_NumInputEvents))
	{
		float w = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex)*UI()->Scale();
		if(w-*Offset > Textbox.w)
		{
			// move to the left
			float wt = TextRender()->TextWidth(0, FontSize, pDisplayStr, -1)*UI()->Scale();
			do
			{
				*Offset += min(wt-*Offset-Textbox.w, Textbox.w/3);
			}
			while(w-*Offset > Textbox.w);
		}
		else if(w-*Offset < 0.0f)
		{
			// move to the right
			do
			{
				*Offset = max(0.0f, *Offset-Textbox.w/3);
			}
			while(w-*Offset < 0.0f);
		}
	}
	UI()->ClipEnable(pRect);
	Textbox.x -= *Offset*UI()->Scale();

	UI()->DoLabel(&Textbox, pDisplayStr, FontSize, -1);
	
	// render the cursor
	if(UI()->LastActiveItem() == pID && !JustGotActive)
	{
		float w = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex);
		Textbox = *pRect;
		Textbox.VSplitLeft(2.0f, 0, &Textbox);
		Textbox.x += (w-*Offset-TextRender()->TextWidth(0, FontSize, "|", -1)/2)*UI()->Scale();

		if((2*time_get()/time_freq()) % 2)	// make it blink
			UI()->DoLabel(&Textbox, "|", FontSize, -1);
	}
	UI()->ClipDisable();

	return ReturnValue;
}

float CMenus::DoScrollbarV(const void *pID, const CUIRect *pRect, float Current)
{
	CUIRect Handle;
	static float OffsetY;
	pRect->HSplitTop(33, &Handle, 0);

	Handle.y += (pRect->h-Handle.h)*Current;

	// logic
    float ReturnValue = Current;
    int Inside = UI()->MouseInside(&Handle);

	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);
		
		float Min = pRect->y;
		float Max = pRect->h-Handle.h;
		float Cur = UI()->MouseY()-OffsetY;
		ReturnValue = (Cur-Min)/Max;
		if(ReturnValue < 0.0f) ReturnValue = 0.0f;
		if(ReturnValue > 1.0f) ReturnValue = 1.0f;
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(pID);
			OffsetY = UI()->MouseY()-Handle.y;
		}
	}
	
	if(Inside)
		UI()->SetHotItem(pID);

	// render
	CUIRect Rail;
	pRect->VMargin(5.0f, &Rail);
	RenderTools()->DrawUIRect(&Rail, vec4(1,1,1,0.25f), 0, 0.0f);

	CUIRect Slider = Handle;
	Slider.w = Rail.x-Slider.x;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_L, 2.5f);
	Slider.x = Rail.x+Rail.w;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_R, 2.5f);

	Slider = Handle;
	Slider.Margin(5.0f, &Slider);
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f)*ButtonColorMul(pID), CUI::CORNER_ALL, 2.5f);
	
    return ReturnValue;
}



float CMenus::DoScrollbarH(const void *pID, const CUIRect *pRect, float Current)
{
	CUIRect Handle;
	static float OffsetX;
	pRect->VSplitLeft(33, &Handle, 0);

	Handle.x += (pRect->w-Handle.w)*Current;

	// logic
    float ReturnValue = Current;
    int Inside = UI()->MouseInside(&Handle);

	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);
		
		float Min = pRect->x;
		float Max = pRect->w-Handle.w;
		float Cur = UI()->MouseX()-OffsetX;
		ReturnValue = (Cur-Min)/Max;
		if(ReturnValue < 0.0f) ReturnValue = 0.0f;
		if(ReturnValue > 1.0f) ReturnValue = 1.0f;
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(pID);
			OffsetX = UI()->MouseX()-Handle.x;
		}
	}
	
	if(Inside)
		UI()->SetHotItem(pID);

	// render
	CUIRect Rail;
	pRect->HMargin(5.0f, &Rail);
	RenderTools()->DrawUIRect(&Rail, vec4(1,1,1,0.25f), 0, 0.0f);

	CUIRect Slider = Handle;
	Slider.h = Rail.y-Slider.y;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_T, 2.5f);
	Slider.y = Rail.y+Rail.h;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_B, 2.5f);

	Slider = Handle;
	Slider.Margin(5.0f, &Slider);
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f)*ButtonColorMul(pID), CUI::CORNER_ALL, 2.5f);
	
    return ReturnValue;
}

int CMenus::DoKeyReader(void *pID, const CUIRect *pRect, int Key)
{
	// process
	static void *pGrabbedID = 0;
	static bool MouseReleased = true;
	static int ButtonUsed = 0;
	int Inside = UI()->MouseInside(pRect);
	int NewKey = Key;
	
	if(!UI()->MouseButton(0) && !UI()->MouseButton(1) && pGrabbedID == pID)
		MouseReleased = true;

	if(UI()->ActiveItem() == pID)
	{
		if(m_Binder.m_GotKey)
		{
			// abort with escape key
			if(m_Binder.m_Key.m_Key != KEY_ESCAPE)
				NewKey = m_Binder.m_Key.m_Key;
			m_Binder.m_GotKey = false;
			UI()->SetActiveItem(0);
			MouseReleased = false;
			pGrabbedID = pID;
		}

		if(ButtonUsed == 1 && !UI()->MouseButton(1))
		{
			if(Inside)
				NewKey = 0;
			UI()->SetActiveItem(0);
		}
	}
	else if(UI()->HotItem() == pID)
	{
		if(MouseReleased)
		{
			if(UI()->MouseButton(0))
			{
				m_Binder.m_TakeKey = true;
				m_Binder.m_GotKey = false;
				UI()->SetActiveItem(pID);
				ButtonUsed = 0;
			}

			if(UI()->MouseButton(1))
			{
				UI()->SetActiveItem(pID);
				ButtonUsed = 1;
			}
		}
	}
	
	if(Inside)
		UI()->SetHotItem(pID);

	// draw
	if (UI()->ActiveItem() == pID && ButtonUsed == 0)
		DoButton_KeySelect(pID, "???", 0, pRect);
	else
	{
		if(Key == 0)
			DoButton_KeySelect(pID, "", 0, pRect);
		else
			DoButton_KeySelect(pID, Input()->KeyName(Key), 0, pRect);
	}
	return NewKey;
}


int CMenus::RenderMenubar(CUIRect r)
{
	CUIRect Box = r;
	CUIRect Button;
	
	m_ActivePage = g_Config.m_UiPage;
	int NewPage = -1;
	
	if(Client()->State() != IClient::STATE_OFFLINE)
		m_ActivePage = m_GamePage;
	
	if(Client()->State() == IClient::STATE_OFFLINE)
	{
		// offline menus
		if(0) // this is not done yet
		{
			Box.VSplitLeft(90.0f, &Button, &Box);
			static int s_NewsButton=0;
			if (DoButton_MenuTab(&s_NewsButton, Localize("News"), m_ActivePage==PAGE_NEWS, &Button, 0))
				NewPage = PAGE_NEWS;
			Box.VSplitLeft(30.0f, 0, &Box); 
		}

		Box.VSplitLeft(100.0f, &Button, &Box);
		static int s_InternetButton=0;
		if(DoButton_MenuTab(&s_InternetButton, Localize("Internet"), m_ActivePage==PAGE_INTERNET, &Button, CUI::CORNER_TL))
		{
			ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
			NewPage = PAGE_INTERNET;
		}

		//Box.VSplitLeft(4.0f, 0, &Box);
		Box.VSplitLeft(80.0f, &Button, &Box);
		static int s_LanButton=0;
		if(DoButton_MenuTab(&s_LanButton, Localize("LAN"), m_ActivePage==PAGE_LAN, &Button, 0))
		{
			ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
			NewPage = PAGE_LAN;
		}

		//box.VSplitLeft(4.0f, 0, &box);
		Box.VSplitLeft(110.0f, &Button, &Box);
		static int s_FavoritesButton=0;
		if(DoButton_MenuTab(&s_FavoritesButton, Localize("Favorites"), m_ActivePage==PAGE_FAVORITES, &Button, CUI::CORNER_TR))
		{
			ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
			NewPage  = PAGE_FAVORITES;
		}
		
		Box.VSplitLeft(4.0f*5, 0, &Box);
		Box.VSplitLeft(100.0f, &Button, &Box);
		static int s_DemosButton=0;
		if(DoButton_MenuTab(&s_DemosButton, Localize("Demos"), m_ActivePage==PAGE_DEMOS, &Button, CUI::CORNER_T))
		{
			DemolistPopulate();
			NewPage  = PAGE_DEMOS;
		}		
	}
	else
	{
		// online menus
		Box.VSplitLeft(90.0f, &Button, &Box);
		static int s_GameButton=0;
		if(DoButton_MenuTab(&s_GameButton, Localize("Game"), m_ActivePage==PAGE_GAME, &Button, CUI::CORNER_T))
			NewPage = PAGE_GAME;

		Box.VSplitLeft(4.0f, 0, &Box);
		Box.VSplitLeft(140.0f, &Button, &Box);
		static int s_ServerInfoButton=0;
		if(DoButton_MenuTab(&s_ServerInfoButton, Localize("Server info"), m_ActivePage==PAGE_SERVER_INFO, &Button, CUI::CORNER_T))
			NewPage = PAGE_SERVER_INFO;

		Box.VSplitLeft(4.0f, 0, &Box);
		Box.VSplitLeft(140.0f, &Button, &Box);
		static int s_CallVoteButton=0;
		if(DoButton_MenuTab(&s_CallVoteButton, Localize("Call vote"), m_ActivePage==PAGE_CALLVOTE, &Button, CUI::CORNER_T))
			NewPage = PAGE_CALLVOTE;
			
		Box.VSplitLeft(30.0f, 0, &Box);
	}
		
	/*
	box.VSplitRight(110.0f, &box, &button);
	static int system_button=0;
	if (UI()->DoButton(&system_button, "System", g_Config.m_UiPage==PAGE_SYSTEM, &button))
		g_Config.m_UiPage = PAGE_SYSTEM;
		
	box.VSplitRight(30.0f, &box, 0);
	*/
	
	Box.VSplitRight(90.0f, &Box, &Button);
	static int s_QuitButton=0;
	if(DoButton_MenuTab(&s_QuitButton, Localize("Quit"), 0, &Button, CUI::CORNER_T))
		m_Popup = POPUP_QUIT;

	Box.VSplitRight(10.0f, &Box, &Button);
	Box.VSplitRight(130.0f, &Box, &Button);
	static int s_SettingsButton=0;
	if(DoButton_MenuTab(&s_SettingsButton, Localize("Settings"), m_ActivePage==PAGE_SETTINGS, &Button, CUI::CORNER_T))
		NewPage = PAGE_SETTINGS;
	
	if(NewPage != -1)
	{
		if(Client()->State() == IClient::STATE_OFFLINE)
			g_Config.m_UiPage = NewPage;
		else
			m_GamePage = NewPage;
	}
		
	return 0;
}

void CMenus::RenderLoading(float Percent)
{
	static int64 LastLoadRender = 0;

	// make sure that we don't render for each little thing we load
	// because that will slow down loading if we have vsync
	if(time_get()-LastLoadRender < time_freq()/60)
		return;
		
	LastLoadRender = time_get();
	
	// need up date this here to get correct
	vec3 Rgb = HslToRgb(vec3(g_Config.m_UiColorHue/255.0f, g_Config.m_UiColorSat/255.0f, g_Config.m_UiColorLht/255.0f));
	ms_GuiColor = vec4(Rgb.r, Rgb.g, Rgb.b, g_Config.m_UiColorAlpha/255.0f);
	
    CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
	
	RenderBackground();

	float tw;

	float w = 700;
	float h = 200;
	float x = Screen.w/2-w/2;
	float y = Screen.h/2-h/2;

	Graphics()->BlendNormal();

	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.50f);
	RenderTools()->DrawRoundRect(x, y, w, h, 40.0f);
	Graphics()->QuadsEnd();


	const char *pCaption = Localize("Loading");

	tw = TextRender()->TextWidth(0, 48.0f, pCaption, -1);
	CUIRect r;
	r.x = x;
	r.y = y+20;
	r.w = w;
	r.h = h;
	UI()->DoLabel(&r, pCaption, 48.0f, 0, -1);

	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,0.75f);
	RenderTools()->DrawRoundRect(x+40, y+h-75, (w-80)*Percent, 25, 5.0f);
	Graphics()->QuadsEnd();

	Graphics()->Swap();
}

void CMenus::RenderNews(CUIRect MainView)
{
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
}

void CMenus::OnInit()
{

	/*
	array<string> my_strings;
	array<string>::range r2;
	my_strings.add("4");
	my_strings.add("6");
	my_strings.add("1");
	my_strings.add("3");
	my_strings.add("7");
	my_strings.add("5");
	my_strings.add("2");

	for(array<string>::range r = my_strings.all(); !r.empty(); r.pop_front())
		dbg_msg("", "%s", r.front().cstr());
		
	sort(my_strings.all());
	
	dbg_msg("", "after:");
	for(array<string>::range r = my_strings.all(); !r.empty(); r.pop_front())
		dbg_msg("", "%s", r.front().cstr());
		
	
	array<int> myarray;
	myarray.add(4);
	myarray.add(6);
	myarray.add(1);
	myarray.add(3);
	myarray.add(7);
	myarray.add(5);
	myarray.add(2);

	for(array<int>::range r = myarray.all(); !r.empty(); r.pop_front())
		dbg_msg("", "%d", r.front());
		
	sort(myarray.all());
	sort_verify(myarray.all());
	
	dbg_msg("", "after:");
	for(array<int>::range r = myarray.all(); !r.empty(); r.pop_front())
		dbg_msg("", "%d", r.front());
	
	exit(-1);
	// */
	
	if(g_Config.m_ClShowWelcome)
		m_Popup = POPUP_LANGUAGE;
	g_Config.m_ClShowWelcome = 0;

	Console()->Chain("add_favorite", ConchainServerbrowserUpdate, this);
}

void CMenus::PopupMessage(const char *pTopic, const char *pBody, const char *pButton)
{
	// reset active item
	UI()->SetActiveItem(0);

	str_copy(m_aMessageTopic, pTopic, sizeof(m_aMessageTopic));
	str_copy(m_aMessageBody, pBody, sizeof(m_aMessageBody));
	str_copy(m_aMessageButton, pButton, sizeof(m_aMessageButton));
	m_Popup = POPUP_MESSAGE;
}


int CMenus::Render()
{
    CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	static bool s_First = true;
	if(s_First)
	{
		if(g_Config.m_UiPage == PAGE_INTERNET)
			ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
		else if(g_Config.m_UiPage == PAGE_LAN)
			ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
		else if(g_Config.m_UiPage == PAGE_FAVORITES)
			ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
		s_First = false;
	}
	
	if(Client()->State() == IClient::STATE_ONLINE)
	{
		ms_ColorTabbarInactive = ms_ColorTabbarInactiveIngame;
		ms_ColorTabbarActive = ms_ColorTabbarActiveIngame;
	}
	else
	{
		RenderBackground();
		ms_ColorTabbarInactive = ms_ColorTabbarInactiveOutgame;
		ms_ColorTabbarActive = ms_ColorTabbarActiveOutgame;
	}

	if (m_Popup != POPUP_DISCONNECTED)
	{
		m_ReconnectTime = 0;
	}
	
	CUIRect TabBar;
	CUIRect MainView;

	// some margin around the screen
	Screen.Margin(10.0f, &Screen);
	
	if(m_Popup == POPUP_NONE)
	{
		// do tab bar
		Screen.HSplitTop(24.0f, &TabBar, &MainView);
		TabBar.VMargin(20.0f, &TabBar);
		RenderMenubar(TabBar);
		
		// news is not implemented yet
		if(g_Config.m_UiPage <= PAGE_NEWS || g_Config.m_UiPage > PAGE_SETTINGS || (Client()->State() == IClient::STATE_OFFLINE && g_Config.m_UiPage >= PAGE_GAME && g_Config.m_UiPage <= PAGE_CALLVOTE))
		{
			ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
			g_Config.m_UiPage = PAGE_INTERNET;
		}
		
		// render current page
		if(Client()->State() != IClient::STATE_OFFLINE)
		{
			if(m_GamePage == PAGE_GAME)
				RenderGame(MainView);
			else if(m_GamePage == PAGE_SERVER_INFO)
				RenderServerInfo(MainView);
			else if(m_GamePage == PAGE_CALLVOTE)
				RenderServerControl(MainView);
			else if(m_GamePage == PAGE_SETTINGS)
				RenderSettings(MainView);
		}
		else if(g_Config.m_UiPage == PAGE_NEWS)
			RenderNews(MainView);
		else if(g_Config.m_UiPage == PAGE_INTERNET)
			RenderServerbrowser(MainView);
		else if(g_Config.m_UiPage == PAGE_LAN)
			RenderServerbrowser(MainView);
		else if(g_Config.m_UiPage == PAGE_DEMOS)
			RenderDemoList(MainView);
		else if(g_Config.m_UiPage == PAGE_FAVORITES)
			RenderServerbrowser(MainView);
		else if(g_Config.m_UiPage == PAGE_SETTINGS)
			RenderSettings(MainView);
	}
	else
	{
		// make sure that other windows doesn't do anything funnay!
		//UI()->SetHotItem(0);
		//UI()->SetActiveItem(0);
		char aBuf[128];
		const char *pTitle = "";
		const char *pExtraText = "";
		const char *pButtonText = "";
		int ExtraAlign = 0;
		
		if(m_Popup == POPUP_MESSAGE)
		{
			pTitle = m_aMessageTopic;
			pExtraText = m_aMessageBody;
			pButtonText = m_aMessageButton;
		}
		else if(m_Popup == POPUP_CONNECTING)
		{
			pTitle = Localize("Connecting to");
			pExtraText = g_Config.m_UiServerAddress;  // TODO: query the client about the address
			pButtonText = Localize("Abort");
			if(Client()->MapDownloadTotalsize() > 0)
			{
				pTitle = Localize("Downloading map");
				pExtraText = "";
			}
		}
		else if(m_Popup == POPUP_DISCONNECTED)
		{
			pTitle = Localize("Disconnected");
			pExtraText = Client()->ErrorString();
			if (g_Config.m_ClAutoReconnect)
			{
				int64 currTime = time_get();
				if (!m_ReconnectTime)
				{
					m_ReconnectTime = currTime + RECONNECTION_TIME * time_freq();
				}
				else if (currTime > m_ReconnectTime)
				{
					Client()->Connect(g_Config.m_UiServerAddress);
					m_ReconnectTime = 0;
				} else {
					str_format(aBuf, sizeof(aBuf), Localize("Ok (%d)"), clamp<int64>((m_ReconnectTime - currTime) / time_freq(), 0, RECONNECTION_TIME) + 1);
					pButtonText = aBuf;
				}
			} else {
				pButtonText = Localize("Ok");
			}
			ExtraAlign = -1;
		}
		else if(m_Popup == POPUP_PURE)
		{
			pTitle = Localize("Disconnected");
			pExtraText = Localize("The server is running a non-standard tuning on a pure game type.");
			pButtonText = Localize("Ok");
			ExtraAlign = -1;
		}
		else if(m_Popup == POPUP_DELETE_DEMO)
		{
			pTitle = Localize("Delete demo");
			pExtraText = Localize("Are you sure that you want to delete the demo?");
			ExtraAlign = -1;
		}
		else if(m_Popup == POPUP_PASSWORD)
		{
			pTitle = Localize("Password incorrect");
			pExtraText = "";
			pButtonText = Localize("Try again");
		}
		else if(m_Popup == POPUP_QUIT)
		{
			pTitle = Localize("Quit");
			pExtraText = Localize("Are you sure that you want to quit?");
		}
		else if(m_Popup == POPUP_FIRST_LAUNCH)
		{
			pTitle = Localize("Welcome to Teeworlds");
			pExtraText = Localize("As this is the first time you launch the game, please enter your nick name below. It's recommended that you check the settings to adjust them to your liking before joining a server.");
			pButtonText = Localize("Ok");
			ExtraAlign = -1;
		}
		
		CUIRect Box, Part;
		Box = Screen;
		Box.VMargin(150.0f, &Box);
		Box.HMargin(150.0f, &Box);
		
		// render the box
		RenderTools()->DrawUIRect(&Box, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 15.0f);
		 
		Box.HSplitTop(20.f, &Part, &Box);
		Box.HSplitTop(24.f, &Part, &Box);
		UI()->DoLabel(&Part, pTitle, 24.f, 0);
		Box.HSplitTop(20.f, &Part, &Box);
		Box.HSplitTop(24.f, &Part, &Box);
		Part.VMargin(20.f, &Part);
		
		if(ExtraAlign == -1)
			UI()->DoLabel(&Part, pExtraText, 20.f, -1, (int)Part.w);
		else
			UI()->DoLabel(&Part, pExtraText, 20.f, 0, -1);

		if(m_Popup == POPUP_QUIT)
		{
			CUIRect Yes, No;
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(80.0f, &Part);
			
			Part.VSplitMid(&No, &Yes);
			
			Yes.VMargin(20.0f, &Yes);
			No.VMargin(20.0f, &No);

			static int s_ButtonAbort = 0;
			if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static int s_ButtonTryAgain = 0;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), 0, &Yes) || m_EnterPressed)
				Client()->Quit();
		}
		else if(m_Popup == POPUP_PASSWORD)
		{
			CUIRect Label, TextBox, TryAgain, Abort;
			
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(80.0f, &Part);
			
			Part.VSplitMid(&Abort, &TryAgain);
			
			TryAgain.VMargin(20.0f, &TryAgain);
			Abort.VMargin(20.0f, &Abort);
			
			static int s_ButtonAbort = 0;
			if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &Abort) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static int s_ButtonTryAgain = 0;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Try again"), 0, &TryAgain) || m_EnterPressed)
			{
				Client()->Connect(g_Config.m_UiServerAddress);
			}
			
			Box.HSplitBottom(60.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			
			Part.VSplitLeft(60.0f, 0, &Label);
			Label.VSplitLeft(100.0f, 0, &TextBox);
			TextBox.VSplitLeft(20.0f, 0, &TextBox);
			TextBox.VSplitRight(60.0f, &TextBox, 0);
			UI()->DoLabel(&Label, Localize("Password"), 18.0f, -1);
			static float Offset = 0.0f;
			DoEditBox(&g_Config.m_Password, &TextBox, g_Config.m_Password, sizeof(g_Config.m_Password), 12.0f, &Offset, true);
		}
		else if(m_Popup == POPUP_CONNECTING)
		{
			Box = Screen;
			Box.VMargin(150.0f, &Box);
			Box.HMargin(150.0f, &Box);
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(120.0f, &Part);

			static int s_Button = 0;
			if(DoButton_Menu(&s_Button, pButtonText, 0, &Part) || m_EscapePressed || m_EnterPressed)
			{
				Client()->Disconnect();
				m_Popup = POPUP_NONE;
			}

			if(Client()->MapDownloadTotalsize() > 0)
			{
				int64 Now = time_get();
				if(Now-m_DownloadLastCheckTime >= time_freq())
				{
					if(m_DownloadLastCheckSize > Client()->MapDownloadAmount())
					{
						// map downloaded restarted
						m_DownloadLastCheckSize = 0;
					}

					// update download speed
					float Diff = (Client()->MapDownloadAmount()-m_DownloadLastCheckSize)/1024.0f;
					m_DownloadSpeed = (m_DownloadSpeed*(1.0f-(1.0f/m_DownloadSpeed))) + (Diff*(1.0f/m_DownloadSpeed));
					m_DownloadLastCheckTime = Now;
					m_DownloadLastCheckSize = Client()->MapDownloadAmount();
				}

				Box.HSplitTop(64.f, 0, &Box);
				Box.HSplitTop(24.f, &Part, &Box);
				str_format(aBuf, sizeof(aBuf), "%d/%d KiB (%.1f KiB/s)", Client()->MapDownloadAmount()/1024, Client()->MapDownloadTotalsize()/1024,	m_DownloadSpeed);
				UI()->DoLabel(&Part, aBuf, 20.f, 0, -1);
				
				// time left
				const char *pTimeLeftString;
				int TimeLeft = (Client()->MapDownloadTotalsize()-Client()->MapDownloadAmount())/(m_DownloadSpeed*1024)+1;
				if(TimeLeft >= 60)
				{
					TimeLeft /= 60;
					pTimeLeftString = TimeLeft == 1 ? Localize("minute") : Localize("minutes");
				}
				else
					pTimeLeftString = TimeLeft == 1 ? Localize("second") : Localize("seconds");
				Box.HSplitTop(20.f, 0, &Box);
				Box.HSplitTop(24.f, &Part, &Box);
				str_format(aBuf, sizeof(aBuf), "%i %s %s", TimeLeft, pTimeLeftString, Localize("left"));
				UI()->DoLabel(&Part, aBuf, 20.f, 0, -1);

				// progress bar
				Box.HSplitTop(20.f, 0, &Box);
				Box.HSplitTop(24.f, &Part, &Box);
				Part.VMargin(40.0f, &Part);
				RenderTools()->DrawUIRect(&Part, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
				Part.w = max(10.0f, (Part.w*Client()->MapDownloadAmount())/Client()->MapDownloadTotalsize());
				RenderTools()->DrawUIRect(&Part, vec4(1.0f, 1.0f, 1.0f, 0.5f), CUI::CORNER_ALL, 5.0f);
			}
		}
		else if(m_Popup == POPUP_LANGUAGE)
		{
			Box = Screen;
			Box.VMargin(150.0f, &Box);
			Box.HMargin(150.0f, &Box);
			Box.HSplitTop(20.f, &Part, &Box);
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Box.HSplitBottom(20.f, &Box, 0);
			Box.VMargin(20.0f, &Box);
			RenderLanguageSelection(Box);
			Part.VMargin(120.0f, &Part);

			static int s_Button = 0;
			if(DoButton_Menu(&s_Button, Localize("Ok"), 0, &Part) || m_EscapePressed || m_EnterPressed)
				m_Popup = POPUP_FIRST_LAUNCH;
		}
		else if(m_Popup == POPUP_DELETE_DEMO)
		{
			CUIRect Yes, No;
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(80.0f, &Part);
			
			Part.VSplitMid(&No, &Yes);
			
			Yes.VMargin(20.0f, &Yes);
			No.VMargin(20.0f, &No);

			static int s_ButtonAbort = 0;
			if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static int s_ButtonTryAgain = 0;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), 0, &Yes) || m_EnterPressed)
			{
				m_Popup = POPUP_NONE;
				m_DemolistDelEntry = true;
			}
		}
		else if(m_Popup == POPUP_FIRST_LAUNCH)
		{
			CUIRect Label, TextBox;
			
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(80.0f, &Part);
			
			static int s_EnterButton = 0;
			if(DoButton_Menu(&s_EnterButton, Localize("Enter"), 0, &Part) || m_EnterPressed)
				m_Popup = POPUP_NONE;
			
			Box.HSplitBottom(40.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			
			Part.VSplitLeft(60.0f, 0, &Label);
			Label.VSplitLeft(100.0f, 0, &TextBox);
			TextBox.VSplitLeft(20.0f, 0, &TextBox);
			TextBox.VSplitRight(60.0f, &TextBox, 0);
			UI()->DoLabel(&Label, Localize("Nickname"), 18.0f, -1);
			static float Offset = 0.0f;
			DoEditBox(&g_Config.m_PlayerName, &TextBox, g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName), 12.0f, &Offset);
		}
		else
		{
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(120.0f, &Part);

			static int s_Button = 0;
			if(DoButton_Menu(&s_Button, pButtonText, 0, &Part) || m_EscapePressed || m_EnterPressed)
				m_Popup = POPUP_NONE;
		}
	}
	
	return 0;
}


void CMenus::SetActive(bool Active)
{
	m_MenuActive = Active;
	if(!m_MenuActive)
	{
		if(m_NeedSendinfo)
		{
			m_pClient->SendInfo(false);
			m_NeedSendinfo = false;
		}

		if(Client()->State() == IClient::STATE_ONLINE)
		{
			m_pClient->OnRelease();
		}
	}
}

void CMenus::OnReset()
{
}

bool CMenus::OnMouseMove(float x, float y)
{
	m_LastInput = time_get();
	
	if(!m_MenuActive)
		return false;
		
	m_MousePos.x += x;
	m_MousePos.y += y;
	if(m_MousePos.x < 0) m_MousePos.x = 0;
	if(m_MousePos.y < 0) m_MousePos.y = 0;
	if(m_MousePos.x > Graphics()->ScreenWidth()) m_MousePos.x = Graphics()->ScreenWidth();
	if(m_MousePos.y > Graphics()->ScreenHeight()) m_MousePos.y = Graphics()->ScreenHeight();
	
	return true;
}

bool CMenus::OnInput(IInput::CEvent e)
{
	m_LastInput = time_get();
	
	// special handle esc and enter for popup purposes
	if(e.m_Flags&IInput::FLAG_PRESS)
	{
		if(e.m_Key == KEY_ESCAPE)
		{
			m_EscapePressed = true;
			SetActive(!IsActive());
			return true;
		}
	}
		
	if(IsActive())
	{
		if(e.m_Flags&IInput::FLAG_PRESS)
		{
			// special for popups
			if(e.m_Key == KEY_RETURN || e.m_Key == KEY_KP_ENTER)
				m_EnterPressed = true;
			else if(e.m_Key == KEY_DELETE)
				m_DeletePressed = true;
		}
		
		if(m_NumInputEvents < MAX_INPUTEVENTS)
			m_aInputEvents[m_NumInputEvents++] = e;
		return true;
	}
	return false;
}

void CMenus::OnStateChange(int NewState, int OldState)
{
	// reset active item
	UI()->SetActiveItem(0);

	if(NewState == IClient::STATE_OFFLINE)
	{
		m_Popup = POPUP_NONE;
		if(Client()->ErrorString() && Client()->ErrorString()[0] != 0)
		{
			if(str_find(Client()->ErrorString(), "password"))
			{
				m_Popup = POPUP_PASSWORD;
				UI()->SetHotItem(&g_Config.m_Password);
				UI()->SetActiveItem(&g_Config.m_Password);
			}
			else
				m_Popup = POPUP_DISCONNECTED;
		}
	}
	else if(NewState == IClient::STATE_LOADING)
	{
		m_Popup = POPUP_CONNECTING;
		m_DownloadLastCheckTime = time_get();
		m_DownloadLastCheckSize = 0;
		m_DownloadSpeed = 1.0f;
		m_ReconnectTime = 0;
		//client_serverinfo_request();
	}
	else if(NewState == IClient::STATE_CONNECTING)
		m_Popup = POPUP_CONNECTING;
	else if (NewState == IClient::STATE_ONLINE || NewState == IClient::STATE_DEMOPLAYBACK)
	{
		m_Popup = POPUP_NONE;
		SetActive(false);
	}
}

extern "C" void font_debug_render();

void CMenus::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		SetActive(true);

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		CUIRect Screen = *UI()->Screen();
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
		RenderDemoPlayer(Screen);
	}
	
	if(Client()->State() == IClient::STATE_ONLINE && m_pClient->m_ServerMode == m_pClient->SERVERMODE_PUREMOD)
	{
		Client()->Disconnect();
		SetActive(true);
		m_Popup = POPUP_PURE;
	}
	
	if(!IsActive())
	{
		m_EscapePressed = false;
		m_EnterPressed = false;
		m_DeletePressed = false;
		m_NumInputEvents = 0;
		return;
	}
	
	// update colors
	vec3 Rgb = HslToRgb(vec3(g_Config.m_UiColorHue/255.0f, g_Config.m_UiColorSat/255.0f, g_Config.m_UiColorLht/255.0f));
	ms_GuiColor = vec4(Rgb.r, Rgb.g, Rgb.b, g_Config.m_UiColorAlpha/255.0f);

	ms_ColorTabbarInactiveOutgame = vec4(0,0,0,0.25f);
	ms_ColorTabbarActiveOutgame = vec4(0,0,0,0.5f);

	float ColorIngameScaleI = 0.5f;
	float ColorIngameAcaleA = 0.2f;
	ms_ColorTabbarInactiveIngame = vec4(
		ms_GuiColor.r*ColorIngameScaleI,
		ms_GuiColor.g*ColorIngameScaleI,
		ms_GuiColor.b*ColorIngameScaleI,
		ms_GuiColor.a*0.8f);
	
	ms_ColorTabbarActiveIngame = vec4(
		ms_GuiColor.r*ColorIngameAcaleA,
		ms_GuiColor.g*ColorIngameAcaleA,
		ms_GuiColor.b*ColorIngameAcaleA,
		ms_GuiColor.a);
    
	// update the ui
	CUIRect *pScreen = UI()->Screen();
	float mx = (m_MousePos.x/(float)Graphics()->ScreenWidth())*pScreen->w;
	float my = (m_MousePos.y/(float)Graphics()->ScreenHeight())*pScreen->h;
		
	int Buttons = 0;
	if(m_UseMouseButtons)
	{
		if(Input()->KeyPressed(KEY_MOUSE_1)) Buttons |= 1;
		if(Input()->KeyPressed(KEY_MOUSE_2)) Buttons |= 2;
		if(Input()->KeyPressed(KEY_MOUSE_3)) Buttons |= 4;
	}
		
	UI()->Update(mx,my,mx*3.0f,my*3.0f,Buttons);
    
	// render
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		Render();

	// render cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,1);
	IGraphics::CQuadItem QuadItem(mx, my, 24, 24);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// render debug information
	if(g_Config.m_Debug)
	{
		CUIRect Screen = *UI()->Screen();
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "%p %p %p", UI()->HotItem(), UI()->ActiveItem(), UI()->LastActiveItem());
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, 10, 10, 10, TEXTFLAG_RENDER);
		TextRender()->TextEx(&Cursor, aBuf, -1);
	}

	m_EscapePressed = false;
	m_EnterPressed = false;
	m_DeletePressed = false;
	m_NumInputEvents = 0;
}

void CMenus::RenderTiles(float x, float y, int tx, int ty, int tw, int th, float frac, float nudge)
{
	if (tw == 0 || th == 0) return;

	float texsize = 1024.0f;
	float scale = 32.0f;

	float px0 = tx*(1024/16);
	float py0 = ty*(1024/16);
	float px1 = (tx+tw)*(1024/16)-1;
	float py1 = (ty+th)*(1024/16)-1;

	float u0 = nudge + px0/texsize+frac;
	float v0 = nudge + py0/texsize+frac;
	float u1 = nudge + px1/texsize-frac;
	float v1 = nudge + py1/texsize-frac;

	Graphics()->QuadsSetSubset(u0,v0,u1,v1);

	IGraphics::CQuadItem QuadItem(x, y - (th - 1) * scale, scale * tw + 1, scale * th + 1);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
}

static int gs_TextureBlob = -1;

static int gs_TextureSun = -1;
static int gs_TextureTiles[4] = {-1};
static int gs_TextureDoodads[4] = {-1};
static int gs_TextureTee = -1;
static int gs_TextureTeeDefault = -1;
static int gs_TextureGame = -1;

static int gs_LastUsedTexture = -1;

void CMenus::RenderBackground()
{
	// Guys, i don't know how it works
	// I wrote that a long time ago, for Z-Pack 1
	// And now i can change only some details... like skin loading and hammer swing
	// So... let it works as it works
	if (g_Config.m_UiNewBackground > 0)
	{
		if(gs_TextureSun == -1)
		{
			gs_TextureSun = Graphics()->LoadTexture("mapres/sun.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		}

		const char * m_TileTextureFilenames[4] = {
			"mapres/grass_main.png", 
			"mapres/jungle_main.png", 
			"mapres/desert_main.png", 
			"mapres/winter_main.png"
		};
		const char * m_DoodadTextureFilenames[4] = {
			"mapres/grass_doodads.png", 
			"mapres/jungle_doodads.png", 
			"mapres/desert_main.png", 
			"mapres/winter_doodads.png"
		};

		const vec2 m_SolidTiles[4] = {
			vec2(1, 0),
			vec2(1, 0),
			vec2(1, 0),
			vec2(1, 0)
		};

		const vec2 m_LandTiles[4][2] = {
			{vec2(0, 1), vec2(1, 1)},
			{vec2(0, 1), vec2(1, 1)},
			{vec2(3, 2), vec2(1, 1)},
			{vec2(1, 1), vec2(3, 1)}
		};

		const vec2 m_GrassTiles[4][2] = {
			{vec2(10, 2), vec2(1, 1)},
			{vec2(10, 2), vec2(1, 1)}, 
			{vec2(12, 4), vec2(1, 1)},
			{vec2(0, 0), vec2(0, 0)}
		};


		const vec2 m_DoodadTiles[4][11][2] =
		{
			{
				{vec2(5, 1),	vec2(3, 1)},
				{vec2(7, 2),	vec2(2, 1)},
				{vec2(10, 0),	vec2(3, 2)},
				{vec2(8, 3),	vec2(3, 2)},
				{vec2(11, 3),	vec2(3, 2)},
				{vec2(0, 3),	vec2(8, 3)},
				{vec2(1, 0),	vec2(3, 3)},
				{vec2(0, 6),	vec2(5, 5)},
				{vec2(5, 6),	vec2(5, 5)},
				{vec2(9, 5),	vec2(7, 6)},
				{vec2(0, 11),	vec2(5, 5)}
			},
			{
				{vec2(4, 0),	vec2(4, 2)},
				{vec2(7, 2),	vec2(2, 1)},
				{vec2(10, 0),	vec2(2, 2)},
				{vec2(8, 3),	vec2(3, 1)},
				{vec2(11, 3),	vec2(3, 2)},
				{vec2(0, 3),	vec2(8, 3)},
				{vec2(0, 0),	vec2(4, 3)},
				{vec2(0, 6),	vec2(5, 3)},
				{vec2(0, 9),	vec2(5, 2)},
				{vec2(10, 11),	vec2(2, 5)},
				{vec2(0, 11),	vec2(5, 5)}
			},
			{
				{vec2(13, 0),	vec2(3, 1)},
				{vec2(14, 2),	vec2(1, 3)},
				{vec2(13, 5),	vec2(3, 3)},
				{vec2(11, 6),	vec2(2, 2)},
				{vec2(11, 5),	vec2(2, 1)},
				{vec2(12, 4),	vec2(1, 1)},
				{vec2(11, 8),	vec2(2, 4)},
				{vec2(13, 8),	vec2(3, 4)},
				{vec2(10, 10),	vec2(1, 2)},
				{vec2(9, 11),	vec2(1, 1)},
				{vec2(8, 6),	vec2(3, 1)},
			},
			{
				{vec2(6, 6),	vec2(1, 1)},
				{vec2(0, 1),	vec2(3, 5)},
				{vec2(6, 0),	vec2(3, 5)},
				{vec2(9, 0),	vec2(3, 4)},
				{vec2(12, 0),	vec2(4, 1)},
				{vec2(12, 1),	vec2(2, 3)},
				{vec2(14, 1),	vec2(1, 2)},
				{vec2(15, 1),	vec2(1, 2)},
				{vec2(14, 3),	vec2(2, 6)},
				{vec2(8, 4),	vec2(3, 6)},
				{vec2(11, 4),	vec2(3, 6)}
			}
		};

		int m_UsedTexture = g_Config.m_UiNewBackground - 1;

		/*if ((m_UsedTexture < 0 || m_UsedTexture > 3) && time(0) - gs_LastBackgroundChange > Z_BackgroundChangeTime)
		{
			srand(time(0));
			m_UsedTexture = (int)(rand()%4);
			
			gs_LastBackgroundChange = time(0);
		}*/

		if (gs_LastUsedTexture != m_UsedTexture || gs_TextureTiles[m_UsedTexture] < 1 || gs_TextureDoodads[m_UsedTexture] < 1)
		{
			if (gs_TextureTiles[m_UsedTexture] < 1)
				gs_TextureTiles[m_UsedTexture] = Graphics()->LoadTexture(m_TileTextureFilenames[m_UsedTexture], IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);

			if (gs_TextureDoodads[m_UsedTexture] < 1)
				gs_TextureDoodads[m_UsedTexture] = Graphics()->LoadTexture(m_DoodadTextureFilenames[m_UsedTexture], IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		}

		gs_LastUsedTexture = m_UsedTexture;

		if(gs_TextureTee == -1)
		{
			if (m_pClient->m_pSkins && m_pClient->m_pSkins->Num() > 0)
				gs_TextureTee = m_pClient->m_pSkins->Get(rand()%m_pClient->m_pSkins->Num())->m_OrgTexture;
			else
			{
				if (gs_TextureTeeDefault < 1)
					gs_TextureTeeDefault = Graphics()->LoadTexture("skins/default.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);

				gs_TextureTee = gs_TextureTeeDefault;
			}
		}

		if(gs_TextureGame == -1)
		{
			gs_TextureGame = Graphics()->LoadTexture("game.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		}

		float sw = 300 * Graphics()->ScreenAspect();
		float sh = 300;

		float max_size = max(sw, sh);
		float sun_size = max_size / 3.0f;
		float sunray_size = sun_size;

		static float last_tick = 0;
		if (last_tick == 0)
			last_tick = Client()->LocalTime();
		float curr_tick = Client()->LocalTime();
		static float sun_rotation = 0.0f;
		sun_rotation += 360.0f * (curr_tick - last_tick) / 100.0f;
		if (sun_rotation > 90.0f)
			sun_rotation -= 90.0f;
		float tick_diff = curr_tick - last_tick;
		last_tick = curr_tick;

		Graphics()->MapScreen(0, 0, sw, sh);

		CUIRect s = *(UI()->Screen());

		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
			Graphics()->SetColor(0.25f, 0.5f, 1.0f, 1.0f);

			IGraphics::CQuadItem QuadItem(0, 0, sw, sh);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 0.5f, 0.25f);

			vec2 raystart;
			raystart.x = sun_size * 0.25f;
			raystart.y = sun_size * 0.25f;

			vec2 ray_e[4];
			ray_e[0].x = raystart.x - sunray_size / 10.0f;
			ray_e[0].y = raystart.y;
			ray_e[1].x = raystart.x + sunray_size / 10.0f;
			ray_e[1].y = raystart.y;
			ray_e[2].x = raystart.x - sunray_size;
			ray_e[2].y = raystart.y + max_size * 2.0f;
			ray_e[3].x = raystart.x + sunray_size;
			ray_e[3].y = raystart.y + max_size * 2.0f;

			vec2 rays[4 * 8];

			for (int i = 0; i < 4 * 8; i++)
			{
				float angle = (-45.0f * (i / 4) + sun_rotation) * pi / 180.0f;

				rays[i].x = (ray_e[i%4].x - raystart.x) * cosf(angle) - (ray_e[i%4].y - raystart.y) * sinf(angle) + raystart.x;
				rays[i].y = (ray_e[i%4].y - raystart.y) * cosf(angle) + (ray_e[i%4].x - raystart.x) * sinf(angle) + raystart.y;
			}

			for (int i = 0; i < 4 * 8; i += 4)
			{
				IGraphics::CFreeformItem Freeform(
					rays[i].x, rays[i].y,
					rays[i + 1].x, rays[i + 1].y,
					rays[i + 2].x, rays[i + 2].y,
					rays[i + 3].x, rays[i + 3].y
				);
				Graphics()->QuadsDrawFreeform(&Freeform, 1);
			}
		Graphics()->QuadsEnd();

		Graphics()->TextureSet(gs_TextureSun);
		Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			Graphics()->QuadsSetRotation(sinf(curr_tick * 0.5f) * 0.5f - 0.25f);

			QuadItem = IGraphics::CQuadItem(-sun_size * 0.25, -sun_size * 0.25, sun_size * 1.25, sun_size * 1.25);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();


		{
			CUIRect screen = *(UI()->Screen());
			Graphics()->MapScreen(screen.x, screen.y, screen.w, screen.h);
		}

		Graphics()->TextureSet(gs_TextureTiles[m_UsedTexture]);

		float scale = 32.0f;

		float screen_x0, screen_y0, screen_x1, screen_y1;
		Graphics()->GetScreen(&screen_x0, &screen_y0, &screen_x1, &screen_y1);

		sw = screen_x1 - screen_x0;
		sh = screen_y1 - screen_y0;

		float tile_pixelsize = 1024/32.0f;
		float final_tilesize = scale/sw * Graphics()->ScreenWidth();
		float final_tilesize_scale = final_tilesize/tile_pixelsize;

		float texsize = 1024.0f;
		float frac = (1.25f/texsize) * (1/final_tilesize_scale);
		float nudge = (0.5f/texsize) * (1/final_tilesize_scale);

		int tx = 0;
		int ty = 1;

		int px0 = tx*(1024/16);
		int py0 = ty*(1024/16);
		int px1 = (tx+1)*(1024/16)-1;
		int py1 = (ty+1)*(1024/16)-1;

		float u0 = nudge + px0/texsize+frac;
		float v0 = nudge + py0/texsize+frac;
		float u1 = nudge + px1/texsize-frac;
		float v1 = nudge + py1/texsize-frac;

		float tcx = -((int)(curr_tick * scale))%((int)(scale));
		float gcx = m_GrassTiles[m_UsedTexture][1].x > 0 ? -((int)(curr_tick * scale))%((int)(scale * m_GrassTiles[m_UsedTexture][1].x)) : 0;
		float c_x = -((int)(curr_tick * scale))%((int)(scale * m_LandTiles[m_UsedTexture][1].x));
		float t_y = sh * 0.75f;

		Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

			Graphics()->QuadsSetSubset(u0,v0,u1,v1);

			for (int i = 0; i < sw / scale + 1; i++)
			{
				RenderTiles(c_x + i * scale * m_LandTiles[m_UsedTexture][1].x, t_y, m_LandTiles[m_UsedTexture][0].x, m_LandTiles[m_UsedTexture][0].y, m_LandTiles[m_UsedTexture][1].x, m_LandTiles[m_UsedTexture][1].y, nudge, frac);
			}

			t_y += m_LandTiles[m_UsedTexture][1].y * scale;

			tx = m_SolidTiles[m_UsedTexture].x;
			ty = m_SolidTiles[m_UsedTexture].y;
			px0 = tx*(1024/16);
			py0 = ty*(1024/16);
			px1 = (tx+1)*(1024/16)-1;
			py1 = (ty+1)*(1024/16)-1;

			u0 = nudge + px0/texsize+frac;
			v0 = nudge + py0/texsize+frac;
			u1 = nudge + px1/texsize-frac;
			v1 = nudge + py1/texsize-frac;

			Graphics()->QuadsSetSubset(u0,v0,u1,v1);

			for (int i = 0; i < sw / scale + 1 + m_LandTiles[m_UsedTexture][1].x; i++)
			{
				for (int j = 0; j < (sh - t_y) / scale; j++)
				{
					IGraphics::CQuadItem QuadItem(c_x + i * scale, t_y + j * scale, scale, scale);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
				}
			}
		Graphics()->QuadsEnd();

		Graphics()->TextureSet(gs_TextureDoodads[m_UsedTexture]);

		t_y = sh * 0.75f - scale;

		static int doodads[11] = {-100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100};

		static float old_c_x = 0.0f;
		bool moved = (old_c_x < tcx);
		old_c_x = tcx;

		for (int i = 0; i < 11; i++)
		{
			if (doodads[i] <= -10)
			{
				if (doodads[i] > -100)
					doodads[i] = (int)(sw / scale + (frandom() * sw * 2.0f / scale));
				else
					doodads[i] = (int)((frandom() * sw * 2.0f / scale));
			}

			if (moved) doodads[i]--;
		}

		Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

			if (m_GrassTiles[m_UsedTexture][1].x > 0)
				for (int i = 0; i < sw / scale + 1; i++)
				{
					RenderTiles(gcx + i * scale * m_GrassTiles[m_UsedTexture][1].x, t_y, m_GrassTiles[m_UsedTexture][0].x, m_GrassTiles[m_UsedTexture][0].y, m_GrassTiles[m_UsedTexture][1].x, m_GrassTiles[m_UsedTexture][1].y, nudge, frac);
				}

			for (int i = 0; i < 11; i++)
			{
				RenderTiles(tcx + doodads[i] * scale, t_y, m_DoodadTiles[m_UsedTexture][i][0].x, m_DoodadTiles[m_UsedTexture][i][0].y, m_DoodadTiles[m_UsedTexture][i][1].x, m_DoodadTiles[m_UsedTexture][i][1].y, nudge, frac);
			}
		Graphics()->QuadsEnd();

		CTeeRenderInfo render_info;

		render_info.m_Texture = gs_TextureTee;
		render_info.m_ColorBody = vec4(1,1,1,1);
		render_info.m_ColorFeet = vec4(1,1,1,1);
		render_info.m_GotAirJump = 0;
		render_info.m_Size = 64.0f;


		static float tee_x = 0.0f;
		tee_x += tick_diff * 64.0f;
		if (tee_x - 128.0f > sw)
		{
			tee_x = -256.0f;
			gs_TextureTee = -1;
		}
		if (tee_x < -256.0f) tee_x = -256.0f;

		vec2 tee_pos = vec2(tee_x, sh * 0.75f - 16.0f);

		float walk_time = fmod(tee_x * 4.0f, 100.0f)/100.0f;
		CAnimState state;
		state.Set(&g_pData->m_aAnimations[ANIM_BASE], 0);
		state.Add(&g_pData->m_aAnimations[ANIM_WALK], walk_time, 1.0f);

		float ct = fabs(sin(curr_tick) * 0.25f);
		state.Add(&g_pData->m_aAnimations[ANIM_HAMMER_SWING], clamp(ct*5.0f,0.0f,1.0f), 1.0f);

		Graphics()->TextureSet(gs_TextureGame);
		Graphics()->QuadsBegin();

		RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[WEAPON_HAMMER].m_pSpriteBody, 0);

		vec2 p = tee_pos + vec2(state.GetAttach()->m_X, state.GetAttach()->m_Y);
		p.y += g_pData->m_Weapons.m_aId[WEAPON_HAMMER].m_Offsety;

		Graphics()->QuadsSetRotation(-pi/2+state.GetAttach()->m_Angle*pi*2);

		RenderTools()->DrawSprite(p.x, p.y, g_pData->m_Weapons.m_aId[WEAPON_HAMMER].m_VisualSize);
		Graphics()->QuadsEnd();

		RenderTools()->RenderTee(&state, &render_info, 0, vec2(1.0f, 0.0f), tee_pos);

		return;
	}
	
	if(gs_TextureBlob == -1)
		gs_TextureBlob = Graphics()->LoadTexture("blob.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);


	float sw = 300*Graphics()->ScreenAspect();
	float sh = 300;
	Graphics()->MapScreen(0, 0, sw, sh);

	// render background color
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
		//vec4 bottom(gui_color.r*0.3f, gui_color.g*0.3f, gui_color.b*0.3f, 1.0f);
		//vec4 bottom(0, 0, 0, 1.0f);
		vec4 Bottom(ms_GuiColor.r, ms_GuiColor.g, ms_GuiColor.b, 1.0f);
		vec4 Top(ms_GuiColor.r, ms_GuiColor.g, ms_GuiColor.b, 1.0f);
		IGraphics::CColorVertex Array[4] = {
			IGraphics::CColorVertex(0, Top.r, Top.g, Top.b, Top.a),
			IGraphics::CColorVertex(1, Top.r, Top.g, Top.b, Top.a),
			IGraphics::CColorVertex(2, Bottom.r, Bottom.g, Bottom.b, Bottom.a),
			IGraphics::CColorVertex(3, Bottom.r, Bottom.g, Bottom.b, Bottom.a)};
		Graphics()->SetColorVertex(Array, 4);
		IGraphics::CQuadItem QuadItem(0, 0, sw, sh);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
	
	// render the tiles
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
		float Size = 15.0f;
		float OffsetTime = fmod(Client()->LocalTime()*0.15f, 2.0f);
		for(int y = -2; y < (int)(sw/Size); y++)
			for(int x = -2; x < (int)(sh/Size); x++)
			{
				Graphics()->SetColor(0,0,0,0.045f);
				IGraphics::CQuadItem QuadItem((x-OffsetTime)*Size*2+(y&1)*Size, (y+OffsetTime)*Size, Size, Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}
	Graphics()->QuadsEnd();

	// render border fade
	Graphics()->TextureSet(gs_TextureBlob);
	Graphics()->QuadsBegin();
		Graphics()->SetColor(0,0,0,0.5f);
		QuadItem = IGraphics::CQuadItem(-100, -100, sw+200, sh+200);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// restore screen	
    {CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);}	
}
