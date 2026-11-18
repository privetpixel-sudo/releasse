#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_ANTIPING_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_ANTIPING_H

#include <base/vmath.h>

#include <engine/shared/config.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Samples prediction horizon, not network RTT. One sample per server tick keeps
// adaptation independent of rendering FPS. No simulation or packet data is changed.
class CDespiteAntiping
{
	int m_LastTick = -1;
	float m_LastPredictionMs = 0.0f;
	float m_PredictionMs = 0.0f;
	float m_JitterMs = 0.0f;

public:
	void Reset() { *this = CDespiteAntiping(); }

	void Observe(int GameTick, int PredictionMs)
	{
		if(GameTick < 0 || GameTick == m_LastTick)
			return;
		const float Sample = static_cast<float>(std::clamp(PredictionMs, 0, 800));
		if(m_LastTick < 0 || GameTick < m_LastTick || static_cast<int64_t>(GameTick) - m_LastTick > 50)
		{
			m_PredictionMs = Sample;
			m_JitterMs = 0.0f;
		}
		else
		{
			const float Delta = std::abs(Sample - m_LastPredictionMs);
			// React promptly to instability, release conservatively after it passes.
			m_JitterMs += (Delta - m_JitterMs) * (Delta > m_JitterMs ? 0.25f : 0.0625f);
			m_PredictionMs += (Sample - m_PredictionMs) * 0.125f;
		}
		m_LastPredictionMs = Sample;
		m_LastTick = GameTick;
	}

	float JitterMs() const { return m_JitterMs; }

	float RecoveryScale(int Profile) const
	{
		constexpr std::array<float, 3> aBaseScale = {0.85f, 1.25f, 1.8f};
		const float JitterPenalty = std::clamp(2.0f * m_JitterMs / std::max(m_PredictionMs, 20.0f), 0.0f, 2.0f);
		return aBaseScale[std::clamp(Profile, 0, 2)] + JitterPenalty;
	}
};

class CDespitePredictionBlend
{
	vec2 m_Weights = vec2(1, 1);
	bool m_Initialized = false;

public:
	void Reset() { *this = CDespitePredictionBlend(); }

	// Smooth confidence changes, not player motion. Normal prediction remains
	// immediate, and the caller still clamps positions to the predicted path.
	vec2 Update(vec2 Target, float DeltaSeconds, int Profile, float JitterMs, int Smoothness)
	{
		Target.x = std::clamp(Target.x, 0.0f, 1.0f);
		Target.y = std::clamp(Target.y, 0.0f, 1.0f);
		if(!m_Initialized || Smoothness <= 0 || DeltaSeconds > 0.25f)
		{
			m_Initialized = true;
			m_Weights = Target;
			return m_Weights;
		}
		constexpr std::array<float, 3> aBaseMs = {12.0f, 22.0f, 35.0f};
		const float DurationMs = aBaseMs[std::clamp(Profile, 0, 2)] +
					 0.25f * std::clamp(Smoothness, 0, 100) + 0.5f * std::clamp(JitterMs, 0.0f, 40.0f);
		const float Alpha = 1.0f - std::exp(-std::max(DeltaSeconds, 0.0f) * 1000.0f / DurationMs);
		m_Weights = mix(m_Weights, Target, Alpha);
		return m_Weights;
	}
};

inline void ApplyDespiteAntipingPreset(CConfig &Config, int Profile)
{
	Config.m_TcDespiteAntiping = 1;
	Config.m_TcDespiteProfile = std::clamp(Profile, 0, 2);
	Config.m_ClPredict = 1;
	Config.m_ClAntiPing = 1;
	Config.m_ClAntiPingPlayers = 1;
	Config.m_ClAntiPingGrenade = 1;
	Config.m_ClAntiPingWeapons = 1;
	Config.m_ClAntiPingGunfire = 1;
	Config.m_ClAntiPingPreInput = 1;
	Config.m_ClAntiPingLimit = 0;
	Config.m_ClAntiPingPercent = 100;
	Config.m_ClAntiPingSmooth = 0;
	Config.m_TcAntiPingImproved = 1;
	Config.m_TcAntiPingStableDirection = 1;
	Config.m_TcAntiPingNegativeBuffer = Profile == 2 ? 1 : 0;
	Config.m_TcFastInput = 1;
	Config.m_TcFastInputAmount = 20;
}

inline bool DespiteAntipingEnabled(const CConfig &Config)
{
	return Config.m_TcDespiteAntiping && Config.m_ClPredict && Config.m_ClAntiPing &&
	       Config.m_ClAntiPingPlayers && Config.m_TcAntiPingImproved && !Config.m_ClAntiPingSmooth;
}

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_ANTIPING_H
