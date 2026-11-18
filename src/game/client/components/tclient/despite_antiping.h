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

inline float DespiteInteractionPriority(float Distance, bool Interacting)
{
	if(Interacting)
		return 1.0f;
	if(!std::isfinite(Distance))
		return 0.0f;
	// Full prediction around hammer range; ease into visual smoothing farther away.
	const float T = std::clamp((Distance - 96.0f) / 96.0f, 0.0f, 1.0f);
	return 1.0f - T * T * (3.0f - 2.0f * T);
}

class CDespiteCorrectionSmoother
{
	vec2 m_Offset = vec2(0, 0);

public:
	void Reset() { m_Offset = vec2(0, 0); }

	// Correction compares old/new predictions at the SAME simulation tick.
	// Ordinary velocity is never filtered, so steady motion acquires no extra lag.
	vec2 Update(vec2 Correction, float DeltaSeconds, int PredictionMs, float JitterMs, int Smoothness, float InteractionPriority, bool ComparablePrediction = true)
	{
		if(!ComparablePrediction || Smoothness <= 0 || InteractionPriority >= 1.0f || DeltaSeconds > 0.25f ||
			!std::isfinite(Correction.x) || !std::isfinite(Correction.y) || length(Correction) > 128.0f)
		{
			Reset();
			return m_Offset;
		}
		const float Limit = 12.0f * std::clamp(Smoothness, 0, 100) / 100.0f * (1.0f - std::clamp(InteractionPriority, 0.0f, 1.0f));
		if(std::isfinite(DeltaSeconds) && DeltaSeconds > 0.0f)
		{
			const float DurationMs = std::clamp(std::clamp(PredictionMs, 0, 800) * 0.35f, 15.0f, 60.0f) +
						 0.4f * std::clamp(JitterMs, 0.0f, 40.0f);
			m_Offset = m_Offset * std::exp(-DeltaSeconds * 1000.0f / DurationMs) - Correction;
		}
		const float Magnitude = length(m_Offset);
		if(Magnitude > Limit && Magnitude > 0.0f)
			m_Offset *= Limit / Magnitude;
		return m_Offset;
	}
};

inline float DespiteEffectiveConfidence(float Confidence, float InteractionPriority)
{
	return mix(std::clamp(Confidence, 0.0f, 1.0f), 1.0f, std::clamp(InteractionPriority, 0.0f, 1.0f));
}

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

inline void ApplyDespiteHighPingPreset(CConfig &Config)
{
	ApplyDespiteAntipingPreset(Config, 1);
	Config.m_TcDespiteSmoothness = 80;
	Config.m_ClPredictDummy = 1;
	Config.m_ClPredictEvents = 1;
	Config.m_ClSubTickAiming = 1;
	Config.m_TcFastInputRepredictHook = 0;
	Config.m_TcFastInputOthers = 0;
	Config.m_TcRemoveAnti = 0;
	Config.m_TcUnpredOthersInFreeze = 0;
	Config.m_TcShowOthersGhosts = 0;
	Config.m_TcSwapGhosts = 0;
}

inline bool DespiteAntipingEnabled(const CConfig &Config)
{
	return Config.m_TcDespiteAntiping && Config.m_ClPredict && Config.m_ClAntiPing &&
	       Config.m_ClAntiPingPlayers && Config.m_TcAntiPingImproved && !Config.m_ClAntiPingSmooth;
}

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_ANTIPING_H
