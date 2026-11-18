#include "despite_antiping.h"

#include <engine/graphics.h>
#include <engine/textrender.h>

#include <game/client/components/menus.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

void CMenus::RenderSettingsDespite(CUIRect MainView)
{
	const ColorRGBA Background(0.055f, 0.10f, 0.18f, 0.97f);
	const ColorRGBA Panel(0.10f, 0.18f, 0.28f, 1.0f);
	const ColorRGBA Accent(0.32f, 0.86f, 0.82f, 1.0f);
	const ColorRGBA Muted(0.73f, 0.82f, 0.90f, 1.0f);
	MainView.Draw(Background, IGraphics::CORNER_ALL, 12.0f);
	MainView.Margin(14.0f, &MainView);

	static CScrollRegion s_Scroll;
	CScrollRegionParams Params;
	Params.m_ScrollUnit = 40.0f;
	Params.m_ScrollbarThickness = 10.0f;
	Params.m_SliderColor = Accent;
	s_Scroll.Begin(&MainView, &Params);
	CUIRect Content = MainView;
	Content.h = 405.0f;
	s_Scroll.AddRect(Content);

	CUIRect Header, Title, Subtitle, Tee;
	Content.HSplitTop(82.0f, &Header, &Content);
	Header.VSplitRight(88.0f, &Header, &Tee);
	Header.HSplitTop(38.0f, &Title, &Header);
	TextRender()->TextColor(Accent);
	Ui()->DoLabel(&Title, "Ðespiтe Tater", 28.0f, TEXTALIGN_ML);
	TextRender()->TextColor(Muted);
	Header.HSplitTop(23.0f, &Subtitle, &Header);
	Ui()->DoLabel(&Subtitle, TCLocalize("Keep your flow."), 16.0f, TEXTALIGN_ML);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
	RenderDevSkin(Tee.Center(), 62.0f, "default", "default", true, 0, 0, 0, false, false,
		ColorRGBA(0.24f, 0.48f, 0.67f, 1.0f), Accent);

	CUIRect Left, Right, Row, Button;
	Content.HSplitTop(252.0f, &Content, nullptr);
	Content.VSplitLeft(Content.w * 0.57f, &Left, &Right);
	Left.VSplitRight(12.0f, &Left, nullptr);
	Right.Draw(Panel, IGraphics::CORNER_ALL, 9.0f);
	Right.Margin(12.0f, &Right);

	Left.HSplitTop(28.0f, &Row, &Left);
	Ui()->DoLabel(&Row, TCLocalize("Adaptive antiping"), 19.0f, TEXTALIGN_ML);
	Left.HSplitTop(8.0f, nullptr, &Left);
	Left.HSplitTop(28.0f, &Row, &Left);
	const bool Enabled = DespiteAntipingEnabled(g_Config);
	static CButtonContainer s_Enable;
	if(DoButton_Menu(&s_Enable, Enabled ? TCLocalize("Disable adaptive recovery") : TCLocalize("Enable recommended setup"), 0, &Row,
		   BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 6.0f, 0.0f, ColorRGBA(0.17f, 0.48f, 0.51f, 1.0f)))
	{
		if(Enabled)
			g_Config.m_TcDespiteAntiping = 0;
		else
			ApplyDespiteAntipingPreset(g_Config, g_Config.m_TcDespiteProfile);
	}

	Left.HSplitTop(12.0f, nullptr, &Left);
	Left.HSplitTop(20.0f, &Row, &Left);
	Ui()->DoLabel(&Row, TCLocalize("Choose your feel"), 14.0f, TEXTALIGN_ML);
	Left.HSplitTop(26.0f, &Row, &Left);
	const char *apNames[] = {TCLocalize("Responsive"), TCLocalize("Balanced"), TCLocalize("Smooth")};
	static CButtonContainer s_aProfiles[3];
	const float Width = Row.w / 3.0f;
	for(int Profile = 0; Profile < 3; ++Profile)
	{
		Row.VSplitLeft(Width, &Button, &Row);
		Button.VMargin(2.0f, &Button);
		if(DoButton_MenuTab(&s_aProfiles[Profile], apNames[Profile], g_Config.m_TcDespiteProfile == Profile, &Button,
			   IGraphics::CORNER_ALL, nullptr, nullptr, nullptr, nullptr, 5.0f))
		{
			// Selecting a profile preserves the disabled state.
			if(Enabled)
				ApplyDespiteAntipingPreset(g_Config, Profile);
			else
				g_Config.m_TcDespiteProfile = Profile;
		}
	}
	Left.HSplitTop(7.0f, nullptr, &Left);
	Left.HSplitTop(42.0f, &Row, &Left);
	const char *apDescriptions[] = {
		TCLocalize("Faster recovery. Best for stable connections."),
		TCLocalize("A balance of quick response and smooth corrections."),
		TCLocalize("Gentler recovery when your connection fluctuates.")};
	TextRender()->TextColor(Muted);
	SLabelProperties Props;
	Props.m_MaxWidth = Row.w;
	Ui()->DoLabel(&Row, apDescriptions[std::clamp(g_Config.m_TcDespiteProfile, 0, 2)], 12.0f, TEXTALIGN_ML, Props);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_TcDespiteHud, TCLocalize("Show antiping status in game"), &g_Config.m_TcDespiteHud, &Left, 22.0f);

	Right.HSplitTop(24.0f, &Row, &Right);
	Ui()->DoLabel(&Row, TCLocalize("Connection"), 17.0f, TEXTALIGN_ML);
	Right.HSplitTop(26.0f, &Row, &Right);
	const bool Online = Client()->State() == IClient::STATE_ONLINE;
	const bool Active = Online && Enabled && GameClient()->Predict() && GameClient()->AntiPingPlayers();
	TextRender()->TextColor(Active ? Accent : Muted);
	Ui()->DoLabel(&Row, !Online ? TCLocalize("Connect to see live values") : Active ? TCLocalize("Adaptive recovery active") :
											  TCLocalize("Adaptive recovery inactive"),
		12.0f, TEXTALIGN_ML);
	TextRender()->TextColor(TextRender()->DefaultTextColor());

	const auto &Filter = GameClient()->m_aDespiteAntiping[g_Config.m_ClDummy];
	const int LocalId = GameClient()->m_Snap.m_LocalClientId;
	const CNetObj_PlayerInfo *pInfo = Online && LocalId >= 0 ? GameClient()->m_Snap.m_apPlayerInfos[LocalId] : nullptr;
	const char *apLabels[] = {TCLocalize("Server-reported ping"), TCLocalize("Prediction variation"), TCLocalize("Recovery scale")};
	for(int Metric = 0; Metric < 3; ++Metric)
	{
		Right.HSplitTop(43.0f, &Row, &Right);
		CUIRect Label, Value;
		Row.HSplitTop(17.0f, &Label, &Value);
		TextRender()->TextColor(Muted);
		Ui()->DoLabel(&Label, apLabels[Metric], 11.0f, TEXTALIGN_ML);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
		char aValue[64];
		str_copy(aValue, "--");
		if(Metric == 0 && pInfo)
			str_format(aValue, sizeof(aValue), "%d ms", pInfo->m_Latency);
		else if(Metric == 1 && Active)
			str_format(aValue, sizeof(aValue), "%.1f ms", Filter.JitterMs());
		else if(Metric == 2 && Active)
			str_format(aValue, sizeof(aValue), "%.2fx", Filter.RecoveryScale(g_Config.m_TcDespiteProfile));
		Ui()->DoLabel(&Value, aValue, 18.0f, TEXTALIGN_ML);
	}

	CUIRect Footer = MainView;
	Footer.y += 349.0f;
	Footer.h = 50.0f;
	Footer.Draw(Panel, IGraphics::CORNER_ALL, 6.0f);
	Footer.Margin(9.0f, &Footer);
	TextRender()->TextColor(Muted);
	Props.m_MaxWidth = Footer.w;
	Ui()->DoLabel(&Footer, TCLocalize("Prediction can make movement feel more immediate. Your real ping stays the same; packet loss and server corrections still matter."), 12.0f, TEXTALIGN_ML, Props);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
	s_Scroll.End();
}
