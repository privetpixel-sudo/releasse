#include "despite_antiping.h"
#include "despite_theme.h"

#include <engine/shared/config.h>

#include <game/client/components/menus.h>
#include <game/client/gameclient.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

void CMenus::RenderSettingsDespite(CUIRect MainView)
{
	MainView.Draw(ColorRGBA(0.075f, 0.075f, 0.075f, 0.96f), IGraphics::CORNER_ALL, 14.0f);
	MainView.Margin(18.0f, &MainView);
	static CScrollRegion s_Scroll;
	static bool s_Details = false;
	CScrollRegionParams Params;
	Params.m_ScrollUnit = 40.0f;
	Params.m_ScrollbarThickness = 10.0f;
	Params.m_SliderColor = DespiteTheme::Muted();
	s_Scroll.Begin(&MainView, &Params);
	CUIRect Content = MainView;
	Content.VMargin(4.0f, &Content);
	Content.y += 4.0f; // Leave room for scaled/lifted button faces inside the clip.
	Content.h = s_Details ? 380.0f : 300.0f;
	s_Scroll.AddRect(Content);
	const float Width = std::min(540.0f, Content.w);
	Content.x += (Content.w - Width) / 2.0f;
	Content.w = Width;
	CUIRect Row, Title, Toggle;
	Content.HSplitTop(32.0f, &Row, &Content);
	Row.VSplitRight(122.0f, &Title, &Toggle);
	TextRender()->TextColor(DespiteTheme::Text());
	Ui()->DoLabel(&Title, "Ðespiтe", 24.0f, TEXTALIGN_ML);
	const bool Enabled = DespiteAntipingEnabled(g_Config);
	static DespiteTheme::CButton s_Enable;
	if(DespiteTheme::Button(Ui(), TextRender(), &s_Enable, Toggle, Enabled ? TCLocalize("Turn off") : TCLocalize("Enable antiping"), !Enabled))
	{
		if(Enabled)
			g_Config.m_TcDespiteAntiping = 0;
		else
			ApplyDespiteAntipingPreset(g_Config, g_Config.m_TcDespiteProfile);
	}
	Content.HSplitTop(6.0f, nullptr, &Content);
	Content.HSplitTop(22.0f, &Row, &Content);
	TextRender()->TextColor(DespiteTheme::Muted());
	Ui()->DoLabel(&Row, TCLocalize("Adaptive antiping"), 13.0f, TEXTALIGN_ML);
	Content.HSplitTop(20.0f, nullptr, &Content);

	CUIRect Profiles, Button;
	Content.HSplitTop(37.0f, &Profiles, &Content);
	DespiteTheme::Surface(Profiles, 12.0f);
	Profiles.Margin(3.0f, &Profiles);
	const char *apNames[] = {TCLocalize("Responsive"), TCLocalize("Balanced"), TCLocalize("Smooth")};
	static DespiteTheme::CButton s_aProfiles[3];
	const float ProfileWidth = Profiles.w / 3.0f;
	for(int Profile = 0; Profile < 3; ++Profile)
	{
		Profiles.VSplitLeft(ProfileWidth, &Button, &Profiles);
		Button.VMargin(2.0f, &Button);
		if(DespiteTheme::Choice(Ui(), TextRender(), &s_aProfiles[Profile], Button, apNames[Profile], Profile == g_Config.m_TcDespiteProfile))
		{
			if(Enabled)
				ApplyDespiteAntipingPreset(g_Config, Profile);
			else
				g_Config.m_TcDespiteProfile = Profile;
		}
	}
	TextRender()->TextColor(TextRender()->DefaultTextColor());
	Content.HSplitTop(15.0f, nullptr, &Content);
	Content.HSplitTop(24.0f, &Row, &Content);
	Ui()->DoScrollbarOption(&g_Config.m_TcDespiteSmoothness, &g_Config.m_TcDespiteSmoothness, &Row, TCLocalize("Smoothness"), 0, 100, &CUi::ms_LinearScrollbarScale, 0, "%");
	Content.HSplitTop(9.0f, nullptr, &Content);
	Content.HSplitTop(22.0f, &Row, &Content);
	SLabelProperties Wrap;
	Wrap.m_MaxWidth = Row.w;
	TextRender()->TextColor(DespiteTheme::Muted());
	Ui()->DoLabel(&Row, TCLocalize("Softer player corrections. Your own input stays immediate."), 11.0f, TEXTALIGN_ML, Wrap);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
	Content.HSplitTop(10.0f, nullptr, &Content);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_TcDespiteHud, TCLocalize("Show antiping status in game"), &g_Config.m_TcDespiteHud, &Content, 22.0f);

	const bool Online = Client()->State() == IClient::STATE_ONLINE;
	const bool Active = Online && Enabled && GameClient()->Predict() && GameClient()->AntiPingPlayers();
	const int LocalId = GameClient()->m_Snap.m_LocalClientId;
	const auto *pInfo = Online && in_range(LocalId, 0, MAX_CLIENTS - 1) ? GameClient()->m_Snap.m_apPlayerInfos[LocalId] : nullptr;
	Content.HSplitTop(17.0f, nullptr, &Content);
	Content.HSplitTop(26.0f, &Row, &Content);
	CUIRect Details;
	Row.VSplitRight(100.0f, &Row, &Details);
	char aStatus[192];
	if(pInfo)
		str_format(aStatus, sizeof(aStatus), "%s / %d ms", Active ? TCLocalize("Active") : TCLocalize("Inactive"), pInfo->m_Latency);
	else
		str_copy(aStatus, TCLocalize("Connect to see live values"));
	TextRender()->TextColor(DespiteTheme::Muted());
	Ui()->DoLabel(&Row, aStatus, 12.0f, TEXTALIGN_ML);
	static DespiteTheme::CButton s_DetailsButton;
	if(DespiteTheme::Link(Ui(), TextRender(), &s_DetailsButton, Details, s_Details ? TCLocalize("Less") : TCLocalize("Details")))
		s_Details = !s_Details;
	if(s_Details)
	{
		Content.HSplitTop(12.0f, nullptr, &Content);
		Content.HSplitTop(23.0f, &Row, &Content);
		const auto &Filter = GameClient()->m_aDespiteAntiping[g_Config.m_ClDummy];
		if(Active)
			str_format(aStatus, sizeof(aStatus), "%s: %.1f ms / %s: %.2fx", TCLocalize("Prediction variation"), Filter.JitterMs(), TCLocalize("Recovery scale"), Filter.RecoveryScale(g_Config.m_TcDespiteProfile));
		else
			str_copy(aStatus, TCLocalize("Adaptive recovery inactive"));
		Ui()->DoLabel(&Row, aStatus, 11.0f, TEXTALIGN_ML);
		Content.HSplitTop(8.0f, nullptr, &Content);
		Content.HSplitTop(40.0f, &Row, &Content);
		Wrap.m_MaxWidth = Row.w;
		Ui()->DoLabel(&Row, TCLocalize("Prediction can make movement feel more immediate. Your real ping stays the same; packet loss and server corrections still matter."), 11.0f, TEXTALIGN_ML, Wrap);
	}
	TextRender()->TextColor(TextRender()->DefaultTextColor());
	s_Scroll.End();
}
