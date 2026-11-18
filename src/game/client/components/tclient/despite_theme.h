#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_THEME_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_THEME_H

#include "despite_button_motion.h"

#include <base/time.h>

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/textrender.h>

#include <game/client/ui.h>

namespace DespiteTheme
{
	class CButton : public CButtonContainer
	{
	public:
		CDespiteButtonMotion m_Motion;
		int64_t m_LastDraw = 0;
	};

	struct SButtonVisual
	{
		CUIRect m_Rect;
		float m_Scale;
		float m_Hover;
		float m_Press;
		bool m_Clicked;
	};

	inline SButtonVisual Animate(CUi *pUi, CButton *pId, const CUIRect &Rect, float Growth)
	{
		// Hit-testing stays on the layout rectangle: enlarged faces never move
		// neighbouring controls or switch hover on/off at the animated edge.
		const bool Clicked = pUi->DoButtonLogic(pId, 0, &Rect, BUTTONFLAG_LEFT);
		const int64_t Now = time_get();
		if(pId->m_LastDraw && Now - pId->m_LastDraw > time_freq() / 3)
			pId->m_Motion = CDespiteButtonMotion();
		pId->m_LastDraw = Now;
		pId->m_Motion.Update(pUi->Enabled() && pUi->MouseHovered(&Rect),
			pUi->CheckActiveItem(pId) && pUi->MouseButton(0), pUi->Client()->RenderFrameTime());
		const float Scale = pId->m_Motion.Scale(Growth);
		CUIRect Visual = {Rect.x - Rect.w * (Scale - 1) / 2, Rect.y - Rect.h * (Scale - 1) / 2,
			Rect.w * Scale, Rect.h * Scale};
		Visual.y += -0.7f * pId->m_Motion.Hover() + 0.8f * pId->m_Motion.Press();
		return {Visual, Scale, pId->m_Motion.Hover(), pId->m_Motion.Press(), Clicked};
	}

	inline ColorRGBA Text() { return ColorRGBA(0.94f, 0.94f, 0.96f, 1.0f); }
	inline ColorRGBA Muted() { return ColorRGBA(0.70f, 0.70f, 0.72f, 1.0f); }
	inline ColorRGBA HoverText(float Amount)
	{
		const auto From = Muted();
		const auto To = Text();
		return ColorRGBA(mix(From.r, To.r, Amount), mix(From.g, To.g, Amount), mix(From.b, To.b, Amount), 1.0f);
	}

	inline void Background(const CUIRect &Rect)
	{
		Rect.Draw4(ColorRGBA(0.075f, 0.075f, 0.075f, 1), ColorRGBA(0.18f, 0.18f, 0.18f, 1),
			ColorRGBA(0.045f, 0.045f, 0.045f, 1), ColorRGBA(0.09f, 0.09f, 0.09f, 1), IGraphics::CORNER_NONE, 0);
	}

	inline void Surface(const CUIRect &Rect, float Radius, float Highlight = 0.0f, float Press = 0.0f)
	{
		Highlight = std::clamp(Highlight, 0.0f, 1.0f);
		CUIRect Shadow = Rect;
		Shadow.y += 2.0f + Highlight - Press;
		Shadow.Draw(ColorRGBA(0, 0, 0, 0.18f + 0.06f * Highlight), IGraphics::CORNER_ALL, Radius);
		const float Edge = 0.26f + 0.22f * Highlight;
		Rect.Draw4(ColorRGBA(1, 1, 1, Edge), ColorRGBA(1, 1, 1, Edge * 0.55f),
			ColorRGBA(1, 1, 1, Edge * 0.30f), ColorRGBA(1, 1, 1, Edge * 0.40f), IGraphics::CORNER_ALL, Radius);
		CUIRect Inner;
		Rect.Margin(0.65f, &Inner);
		const float Top = 0.225f + 0.075f * Highlight - 0.035f * Press;
		const float Bottom = 0.13f + 0.035f * Highlight - 0.02f * Press;
		Inner.Draw4(ColorRGBA(Top, Top, Top, 1), ColorRGBA(Top - 0.025f, Top - 0.025f, Top - 0.025f, 1),
			ColorRGBA(Bottom, Bottom, Bottom, 1), ColorRGBA(Bottom + 0.02f, Bottom + 0.02f, Bottom + 0.02f, 1), IGraphics::CORNER_ALL, Radius - 0.65f);
	}

	inline void Icon(CUi *pUi, ITextRender *pTextRender, const CUIRect &Rect, const char *pIcon, float Size)
	{
		pTextRender->SetFontPreset(EFontPreset::ICON_FONT);
		pTextRender->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
		pUi->DoLabel(&Rect, pIcon, Size, TEXTALIGN_MC);
		pTextRender->SetRenderFlags(0);
		pTextRender->SetFontPreset(EFontPreset::DEFAULT_FONT);
	}

	inline bool Button(CUi *pUi, ITextRender *pTextRender, CButton *pId, const CUIRect &Rect, const char *pLabel, bool Primary = false, const char *pIcon = nullptr)
	{
		const auto Visual = Animate(pUi, pId, Rect, Primary ? 0.045f : 0.025f);
		const CUIRect &Face = Visual.m_Rect;
		if(Primary)
		{
			CUIRect Shadow = Face;
			Shadow.y += 3.0f - Visual.m_Press;
			Shadow.Draw(ColorRGBA(0, 0, 0, 0.24f), IGraphics::CORNER_ALL, 13.0f);
			Face.Draw(ColorRGBA(1, 1, 1, 0.58f + 0.22f * Visual.m_Hover), IGraphics::CORNER_ALL, 13.0f * Visual.m_Scale);
			CUIRect Inner;
			Face.Margin(0.75f, &Inner);
			const float Top = 0.94f + 0.05f * Visual.m_Hover - 0.08f * Visual.m_Press;
			const float Bottom = 0.69f + 0.08f * Visual.m_Hover - 0.07f * Visual.m_Press;
			Inner.Draw4(ColorRGBA(Top, Top, Top, 1), ColorRGBA(Top - 0.02f, Top - 0.02f, Top - 0.02f, 1), ColorRGBA(Bottom, Bottom, Bottom, 1), ColorRGBA(Bottom, Bottom, Bottom, 1), IGraphics::CORNER_ALL, 12.3f * Visual.m_Scale);
		}
		else
			Surface(Face, 10.0f * Visual.m_Scale, Visual.m_Hover, Visual.m_Press);
		pTextRender->TextColor(Primary ? ColorRGBA(0.10f, 0.10f, 0.10f, 1) : Text());
		pTextRender->TextOutlineColor(0, 0, 0, 0);
		CUIRect Label = Face;
		if(pIcon)
		{
			const float TextWidth = pTextRender->TextWidth(14.0f * Visual.m_Scale, pLabel);
			CUIRect Symbol = {Face.x + (Face.w - TextWidth - 24.0f * Visual.m_Scale) / 2, Face.y, 16.0f * Visual.m_Scale, Face.h};
			Icon(pUi, pTextRender, Symbol, pIcon, 12.0f * Visual.m_Scale);
			Label.x = Symbol.x + 24.0f * Visual.m_Scale;
			Label.w = TextWidth;
		}
		pUi->DoLabel(&Label, pLabel, 14.0f * Visual.m_Scale, TEXTALIGN_MC);
		pTextRender->TextColor(pTextRender->DefaultTextColor());
		pTextRender->TextOutlineColor(pTextRender->DefaultTextOutlineColor());
		return Visual.m_Clicked;
	}

	inline bool Navigation(CUi *pUi, ITextRender *pTextRender, CButton *pId, const CUIRect &Rect, const char *pIcon, const char *pLabel, const char *pDescription)
	{
		const auto Visual = Animate(pUi, pId, Rect, 0.03f);
		Surface(Visual.m_Rect, 15.0f * Visual.m_Scale, Visual.m_Hover * 0.9f, Visual.m_Press);
		CUIRect Body, Symbol, Label;
		Visual.m_Rect.Margin(10.0f * Visual.m_Scale, &Body);
		Body.VSplitLeft(33.0f * Visual.m_Scale, &Symbol, &Body);
		Symbol.HMargin((Symbol.h - 33.0f * Visual.m_Scale) / 2, &Symbol);
		Surface(Symbol, 10.0f * Visual.m_Scale, Visual.m_Hover, Visual.m_Press);
		pTextRender->TextColor(Text());
		pTextRender->TextOutlineColor(0, 0, 0, 0);
		Icon(pUi, pTextRender, Symbol, pIcon, 14.0f * Visual.m_Scale);
		Body.VSplitLeft(10.0f * Visual.m_Scale, nullptr, &Body);
		Body.HSplitMid(&Label, &Body);
		pUi->DoLabel(&Label, pLabel, 14.0f * Visual.m_Scale, TEXTALIGN_ML);
		pTextRender->TextColor(Muted());
		pUi->DoLabel(&Body, pDescription, 10.0f * Visual.m_Scale, TEXTALIGN_ML);
		pTextRender->TextColor(pTextRender->DefaultTextColor());
		pTextRender->TextOutlineColor(pTextRender->DefaultTextOutlineColor());
		return Visual.m_Clicked;
	}

	inline bool Choice(CUi *pUi, ITextRender *pTextRender, CButton *pId, const CUIRect &Rect, const char *pLabel, bool Selected)
	{
		const auto Visual = Animate(pUi, pId, Rect, 0.012f);
		Surface(Visual.m_Rect, 9.0f * Visual.m_Scale, (Selected ? 0.42f : 0.0f) + 0.5f * Visual.m_Hover, Visual.m_Press);
		pTextRender->TextColor(Selected ? Text() : HoverText(Visual.m_Hover));
		pUi->DoLabel(&Visual.m_Rect, pLabel, 13.0f * Visual.m_Scale, TEXTALIGN_MC);
		pTextRender->TextColor(pTextRender->DefaultTextColor());
		return Visual.m_Clicked;
	}

	inline bool Link(CUi *pUi, ITextRender *pTextRender, CButton *pId, const CUIRect &Rect, const char *pLabel)
	{
		const auto Visual = Animate(pUi, pId, Rect, 0.03f);
		pTextRender->TextColor(HoverText(Visual.m_Hover));
		pUi->DoLabel(&Visual.m_Rect, pLabel, 11.0f * Visual.m_Scale, TEXTALIGN_MC);
		pTextRender->TextColor(pTextRender->DefaultTextColor());
		return Visual.m_Clicked;
	}
}

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_THEME_H
