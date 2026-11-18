#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_THEME_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_THEME_H

#include <engine/graphics.h>
#include <engine/textrender.h>

#include <game/client/ui.h>

namespace DespiteTheme
{
	inline ColorRGBA Text() { return ColorRGBA(0.94f, 0.94f, 0.96f, 1.0f); }
	inline ColorRGBA Muted() { return ColorRGBA(0.70f, 0.70f, 0.72f, 1.0f); }

	inline void Background(const CUIRect &Rect)
	{
		Rect.Draw4(ColorRGBA(0.075f, 0.075f, 0.075f, 1), ColorRGBA(0.18f, 0.18f, 0.18f, 1),
			ColorRGBA(0.045f, 0.045f, 0.045f, 1), ColorRGBA(0.09f, 0.09f, 0.09f, 1), IGraphics::CORNER_NONE, 0);
	}

	inline void Surface(const CUIRect &Rect, float Radius, bool Highlight = false)
	{
		CUIRect Shadow = Rect;
		Shadow.y += 3.0f;
		Shadow.Draw(ColorRGBA(0, 0, 0, 0.16f), IGraphics::CORNER_ALL, Radius);
		const float Edge = Highlight ? 0.44f : 0.22f;
		Rect.Draw4(ColorRGBA(1, 1, 1, Edge), ColorRGBA(1, 1, 1, Edge * 0.55f),
			ColorRGBA(1, 1, 1, Edge * 0.30f), ColorRGBA(1, 1, 1, Edge * 0.40f), IGraphics::CORNER_ALL, Radius);
		CUIRect Inner;
		Rect.Margin(0.7f, &Inner);
		const float Top = Highlight ? 0.32f : 0.22f;
		Inner.Draw4(ColorRGBA(Top, Top, Top, 1), ColorRGBA(Top - 0.04f, Top - 0.04f, Top - 0.04f, 1),
			ColorRGBA(0.12f, 0.12f, 0.12f, 1), ColorRGBA(0.15f, 0.15f, 0.15f, 1), IGraphics::CORNER_ALL, Radius - 0.7f);
	}

	inline void Icon(CUi *pUi, ITextRender *pTextRender, const CUIRect &Rect, const char *pIcon, float Size)
	{
		pTextRender->SetFontPreset(EFontPreset::ICON_FONT);
		pTextRender->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);
		pUi->DoLabel(&Rect, pIcon, Size, TEXTALIGN_MC);
		pTextRender->SetRenderFlags(0);
		pTextRender->SetFontPreset(EFontPreset::DEFAULT_FONT);
	}

	inline bool Button(CUi *pUi, ITextRender *pTextRender, CButtonContainer *pId, const CUIRect &Rect, const char *pLabel, bool Primary = false)
	{
		const bool Hover = pUi->MouseHovered(&Rect);
		if(Primary)
		{
			Rect.Draw(ColorRGBA(1, 1, 1, 0.5f), IGraphics::CORNER_ALL, 13.0f);
			CUIRect Inner;
			Rect.Margin(0.7f, &Inner);
			const float Top = Hover ? 1.0f : 0.94f;
			const float Bottom = Hover ? 0.76f : 0.65f;
			Inner.Draw4(ColorRGBA(Top, Top, Top, 1), ColorRGBA(Top, Top, Top, 1), ColorRGBA(Bottom, Bottom, Bottom, 1), ColorRGBA(Bottom, Bottom, Bottom, 1), IGraphics::CORNER_ALL, 12.3f);
		}
		else
			Surface(Rect, 9.0f, Hover);
		pTextRender->TextColor(Primary ? ColorRGBA(0.10f, 0.10f, 0.10f, 1) : Text());
		pTextRender->TextOutlineColor(0, 0, 0, 0);
		pUi->DoLabel(&Rect, pLabel, 14.0f, TEXTALIGN_MC);
		pTextRender->TextColor(pTextRender->DefaultTextColor());
		pTextRender->TextOutlineColor(pTextRender->DefaultTextOutlineColor());
		return pUi->DoButtonLogic(pId, 0, &Rect, BUTTONFLAG_LEFT);
	}

	inline bool Navigation(CUi *pUi, ITextRender *pTextRender, CButtonContainer *pId, const CUIRect &Rect, const char *pIcon, const char *pLabel, const char *pDescription)
	{
		const bool Hover = pUi->MouseHovered(&Rect);
		if(Hover)
			Surface(Rect, 14.0f, true);
		CUIRect Body, Symbol, Label;
		Rect.Margin(10.0f, &Body);
		Body.VSplitLeft(33.0f, &Symbol, &Body);
		Symbol.HMargin((Symbol.h - 33.0f) / 2, &Symbol);
		Surface(Symbol, 10.0f, Hover);
		pTextRender->TextColor(Text());
		Icon(pUi, pTextRender, Symbol, pIcon, 14.0f);
		Body.VSplitLeft(10.0f, nullptr, &Body);
		Body.HSplitMid(&Label, &Body);
		pUi->DoLabel(&Label, pLabel, 14.0f, TEXTALIGN_ML);
		pTextRender->TextColor(Muted());
		pUi->DoLabel(&Body, pDescription, 10.0f, TEXTALIGN_ML);
		pTextRender->TextColor(pTextRender->DefaultTextColor());
		return pUi->DoButtonLogic(pId, 0, &Rect, BUTTONFLAG_LEFT);
	}

	inline bool Link(CUi *pUi, ITextRender *pTextRender, CButtonContainer *pId, const CUIRect &Rect, const char *pLabel)
	{
		pTextRender->TextColor(pUi->MouseHovered(&Rect) ? Text() : Muted());
		pUi->DoLabel(&Rect, pLabel, 11.0f, TEXTALIGN_MC);
		pTextRender->TextColor(pTextRender->DefaultTextColor());
		return pUi->DoButtonLogic(pId, 0, &Rect, BUTTONFLAG_LEFT);
	}
}

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_THEME_H
