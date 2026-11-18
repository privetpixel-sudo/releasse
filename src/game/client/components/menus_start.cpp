/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* Modified for releasse: graphite theme inspired by the supplied launcher reference. */
#include "menus_start.h"

#include <engine/font_icons.h>
#include <engine/keys.h>
#include <engine/shared/config.h>

#include <game/client/components/tclient/despite_theme.h>
#include <game/client/gameclient.h>
#include <game/localization.h>
#include <game/version.h>

#include <algorithm>

void CMenusStart::RenderStartMenu(CUIRect MainView)
{
	DespiteTheme::Background(*Ui()->Screen());
	TextRender()->TextOutlineColor(0, 0, 0, 0);
	CUIRect Header, Brand, Mark, Quit;
	MainView.Margin(8.0f, &MainView);
	MainView.HSplitTop(30.0f, &Header, nullptr);
	Header.VSplitRight(70.0f, &Brand, &Quit);
	Brand.VSplitLeft(28.0f, &Mark, &Brand);
	DespiteTheme::Surface(Mark, 9.0f);
	Ui()->DoLabel(&Mark, "r", 18.0f, TEXTALIGN_MC);
	Brand.VSplitLeft(9.0f, nullptr, &Brand);
	TextRender()->TextColor(DespiteTheme::Text());
	Ui()->DoLabel(&Brand, "releasse", 14.0f, TEXTALIGN_ML);
	static DespiteTheme::CButton s_Quit;
	const bool Escape = Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE);
	if(DespiteTheme::Button(Ui(), TextRender(), &s_Quit, Quit, Localize("Quit"), false, FontIcon::XMARK) || Escape || CheckHotKey(KEY_Q))
	{
		if(Escape || GameClient()->Editor()->HasUnsavedData())
			GameClient()->m_Menus.ShowQuitPopup();
		else
			Client()->Quit();
	}

	const float CenterX = MainView.x + MainView.w / 2.0f;
	const float PanelWidth = std::min(430.0f, MainView.w - 30.0f);
	CUIRect Title = {MainView.x, MainView.y + MainView.h * 0.19f, MainView.w, 48.0f};
	TextRender()->TextColor(DespiteTheme::Text());
	Ui()->DoLabel(&Title, CLIENT_NAME, 34.0f, TEXTALIGN_MC);

	CUIRect Panel = {CenterX - PanelWidth / 2, MainView.y + MainView.h * 0.38f, PanelWidth, 150.0f};
	DespiteTheme::Surface(Panel, 24.0f);
	CUIRect Grid, Top, Bottom, Settings, Demos, Editor, Server;
	Panel.Margin(9.0f, &Grid);
	Grid.HSplitMid(&Top, &Bottom, 8.0f);
	Top.VSplitMid(&Settings, &Demos, 8.0f);
	Bottom.VSplitMid(&Editor, &Server, 8.0f);
	static DespiteTheme::CButton s_Settings, s_Demos, s_Editor, s_Server;
	int NewPage = -1;
	if(DespiteTheme::Navigation(Ui(), TextRender(), &s_Settings, Settings, FontIcon::GEAR, Localize("Settings"), TCLocalize("Make it yours")) || CheckHotKey(KEY_S))
		NewPage = CMenus::PAGE_SETTINGS;
	if(DespiteTheme::Navigation(Ui(), TextRender(), &s_Demos, Demos, FontIcon::FILM, Localize("Demos"), TCLocalize("Your saved runs")) || CheckHotKey(KEY_D))
		NewPage = CMenus::PAGE_DEMOS;
	if(DespiteTheme::Navigation(Ui(), TextRender(), &s_Editor, Editor, FontIcon::PEN_TO_SQUARE, Localize("Editor"), TCLocalize("Create a map")) || CheckHotKey(KEY_E))
	{
		g_Config.m_ClEditor = 1;
		Input()->MouseModeRelative();
	}
	const bool Running = GameClient()->m_LocalServer.IsServerRunning();
	if(DespiteTheme::Navigation(Ui(), TextRender(), &s_Server, Server, FontIcon::CIRCLE_PLAY,
		   Running ? Localize("Stop server") : Localize("Run server"), TCLocalize("Local practice")) ||
		CheckHotKey(KEY_R))
	{
		if(Running)
			GameClient()->m_LocalServer.KillServer();
		else
			GameClient()->m_LocalServer.RunServer({});
	}

	CUIRect Play = {CenterX - 77.0f, Panel.y + Panel.h + 25.0f, 154.0f, 40.0f};
	static DespiteTheme::CButton s_Play;
	if(DespiteTheme::Button(Ui(), TextRender(), &s_Play, Play, Localize("Play", "Start menu"), true, FontIcon::PLAY) || Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER) || CheckHotKey(KEY_P))
		NewPage = g_Config.m_UiPage >= CMenus::PAGE_INTERNET && g_Config.m_UiPage <= CMenus::PAGE_FAVORITE_COMMUNITY_5 ? g_Config.m_UiPage : CMenus::PAGE_INTERNET;

	CUIRect Footer = {CenterX - 160.0f, MainView.y + MainView.h - 32.0f, 320.0f, 22.0f};
	CUIRect Link;
	static DespiteTheme::CButton s_Tutorial, s_News, s_Console, s_Source;
	Footer.VSplitLeft(80.0f, &Link, &Footer);
	if(DespiteTheme::Link(Ui(), TextRender(), &s_Tutorial, Link, Localize("Tutorial")))
		GameClient()->m_Menus.JoinTutorial();
	Footer.VSplitLeft(80.0f, &Link, &Footer);
	if(DespiteTheme::Link(Ui(), TextRender(), &s_News, Link, Localize("News")) || CheckHotKey(KEY_N))
		NewPage = CMenus::PAGE_NEWS;
	Footer.VSplitLeft(80.0f, &Link, &Footer);
	if(DespiteTheme::Link(Ui(), TextRender(), &s_Console, Link, TCLocalize("Console")))
		GameClient()->m_GameConsole.Toggle(CGameConsole::CONSOLETYPE_LOCAL);
	if(DespiteTheme::Link(Ui(), TextRender(), &s_Source, Footer, "GitHub"))
		Client()->ViewLink("https://github.com/privetpixel-sudo/releasse");
	CUIRect Version = {MainView.x, MainView.y + MainView.h - 18.0f, MainView.w, 14.0f};
	TextRender()->TextColor(DespiteTheme::Muted());
	Ui()->DoLabel(&Version, CLIENT_RELEASE_VERSION " / TClient " TCLIENT_VERSION, 9.0f, TEXTALIGN_MR);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
	TextRender()->TextOutlineColor(TextRender()->DefaultTextOutlineColor());
	if(NewPage != -1)
	{
		GameClient()->m_Menus.SetShowStart(false);
		GameClient()->m_Menus.SetMenuPage(NewPage);
	}
}

bool CMenusStart::CheckHotKey(int Key) const
{
	return !Input()->ShiftIsPressed() && !Input()->ModifierIsPressed() && !Input()->AltIsPressed() &&
	       Input()->KeyPress(Key) && !GameClient()->m_GameConsole.IsActive();
}
