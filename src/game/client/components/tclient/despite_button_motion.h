#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_BUTTON_MOTION_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_BUTTON_MOTION_H

#include <algorithm>
#include <cmath>

class CDespiteButtonMotion
{
	float m_Hover = 0.0f;
	float m_Press = 0.0f;

public:
	void Update(bool Hovered, bool Pressed, float DeltaSeconds)
	{
		if(!std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0f)
			return;
		const float Delta = std::min(DeltaSeconds, 0.1f);
		const float HoverAlpha = -std::expm1(-Delta / (Hovered ? 0.065f : 0.09f));
		const float PressAlpha = -std::expm1(-Delta / (Pressed ? 0.035f : 0.07f));
		m_Hover += ((Hovered ? 1.0f : 0.0f) - m_Hover) * HoverAlpha;
		m_Press += ((Pressed ? 1.0f : 0.0f) - m_Press) * PressAlpha;
	}
	float Hover() const { return m_Hover; }
	float Press() const { return m_Press; }
	float Scale(float Growth) const { return 1.0f + std::clamp(Growth, 0.0f, 0.06f) * m_Hover - 0.025f * m_Press; }
};

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_BUTTON_MOTION_H
