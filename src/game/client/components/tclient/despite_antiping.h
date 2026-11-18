#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_ANTIPING_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_ANTIPING_H

#include <base/vmath.h>

#include <engine/shared/config.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

inline int DespitePresetStrength(int Preset)
{
	constexpr std::array<int, 9> aStrengths = {35, 42, 50, 58, 66, 74, 82, 90, 100};
	return aStrengths[std::clamp(Preset, 1, 9) - 1];
}

inline int DespitePresetProfile(int Preset)
{
	return Preset <= 3 ? 0 : (Preset == 9 ? 2 : 1);
}

// Uses server-reported local-player latency, independently of prediction margin.
// Runtime values never overwrite the user's saved manual profile/strength.
class CDespiteAutoPing
{
	int m_LastTick = -1;
	int m_Preset = 0;
	int m_PendingPreset = 0;
	float m_PendingSeconds = 0.0f;
	float m_PingMs = 0.0f;
	float m_LastPingMs = 0.0f;
	float m_VariationMs = 0.0f;
	float m_Strength = 0.0f;

public:
	void Reset() { *this = CDespiteAutoPing(); }
	bool Ready() const { return m_Preset != 0; }
	int Preset() const { return m_Preset; }
	int Profile() const { return DespitePresetProfile(m_Preset); }
	int Smoothness() const { return std::clamp(static_cast<int>(std::round(m_Strength)), 0, 100); }
	float PingMs() const { return m_PingMs; }

	void Observe(int Tick, int TickSpeed, int PingMs, float PredictionJitterMs)
	{
		if(Tick < 0 || TickSpeed <= 0 || PingMs < 0)
		{
			Reset();
			return;
		}
		if(Tick == m_LastTick)
			return;
		const float Sample = static_cast<float>(std::clamp(PingMs, 0, 800));
		const int64_t Ticks = static_cast<int64_t>(Tick) - m_LastTick;
		if(!Ready() || Ticks < 0 || Ticks > static_cast<int64_t>(TickSpeed) * 2)
		{
			Reset();
			m_LastTick = Tick;
			m_PingMs = m_LastPingMs = Sample;
			m_Preset = std::clamp(static_cast<int>((Sample - 25.0f) / 10.0f) + 1, 1, 9);
			m_Strength = static_cast<float>(DespitePresetStrength(m_Preset));
			return;
		}
		const float Seconds = static_cast<float>(Ticks) / TickSpeed;
		m_LastTick = Tick;
		m_PingMs += (Sample - m_PingMs) * (1.0f - std::exp(-Seconds / 0.75f));
		m_VariationMs += (std::abs(Sample - m_LastPingMs) - m_VariationMs) * (1.0f - std::exp(-Seconds / 0.8f));
		m_LastPingMs = Sample;
		int Target = std::clamp(static_cast<int>((m_PingMs - 25.0f) / 10.0f) + 1, 1, 9);
		const float Center = 20.0f + 10.0f * m_Preset;
		if((Target > m_Preset && m_PingMs <= Center + 8.0f) || (Target < m_Preset && m_PingMs >= Center - 8.0f))
			Target = m_Preset;
		if(Target == m_Preset || Target != m_PendingPreset)
		{
			m_PendingPreset = Target;
			m_PendingSeconds = 0.0f;
		}
		else
		{
			m_PendingSeconds += Seconds;
			if(m_PendingSeconds >= (Target > m_Preset ? 1.0f : 2.5f))
			{
				m_Preset = Target;
				m_PendingSeconds = 0.0f;
			}
		}
		const float PredictionVariation = std::isfinite(PredictionJitterMs) ? std::clamp(PredictionJitterMs, 0.0f, 40.0f) : 0.0f;
		const float TargetStrength = std::min(100.0f, DespitePresetStrength(m_Preset) + std::clamp((m_VariationMs + PredictionVariation) * 0.25f, 0.0f, 10.0f));
		m_Strength += (TargetStrength - m_Strength) * (1.0f - std::exp(-Seconds / 0.6f));
	}
};

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
			const float Strength = std::clamp(Smoothness, 0, 100) / 100.0f;
			const float DurationMs = std::min(85.0f, (std::clamp(std::clamp(PredictionMs, 0, 800) * 0.35f, 15.0f, 60.0f) +
									 0.4f * std::clamp(JitterMs, 0.0f, 40.0f)) *
									 (0.75f + 0.6f * Strength));
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
	Config.m_TcDespiteAutoPing = 0;
	Config.m_TcDespitePingPreset = 0;
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

inline void ApplyDespitePingPreset(CConfig &Config, int Preset)
{
	Preset = std::clamp(Preset, 1, 9);
	ApplyDespiteAntipingPreset(Config, DespitePresetProfile(Preset));
	Config.m_TcDespitePingPreset = Preset;
	Config.m_TcDespiteSmoothness = DespitePresetStrength(Preset);
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

inline void ApplyDespiteHighPingPreset(CConfig &Config)
{
	ApplyDespitePingPreset(Config, 8);
}

inline void ApplyDespiteAutoPing(CConfig &Config)
{
	const int Profile = Config.m_TcDespiteProfile;
	const int Smoothness = Config.m_TcDespiteSmoothness;
	const int Preset = Config.m_TcDespitePingPreset;
	ApplyDespitePingPreset(Config, 8);
	Config.m_TcDespiteProfile = Profile;
	Config.m_TcDespiteSmoothness = Smoothness;
	Config.m_TcDespitePingPreset = Preset;
	Config.m_TcDespiteAutoPing = 1;
}

class CDespiteHammerAnimation
{
	int m_AttackTick = -1;
	float m_Age = 0.0f;
	bool m_Initialized = false;

public:
	void Reset() { *this = CDespiteHammerAnimation(); }

	// Pose clock only: never changes attack ticks, hit detection or effects.
	float Update(int AttackTick, float TargetAge, float DeltaSeconds, bool Paused, bool ReconcileAdjacent = true)
	{
		if(!std::isfinite(TargetAge))
			return m_Age;
		TargetAge = std::clamp(TargetAge, 0.0f, 0.2f);
		const int64_t TickDelta = static_cast<int64_t>(AttackTick) - m_AttackTick;
		const bool NewSwing = !m_Initialized || (ReconcileAdjacent ? std::abs(TickDelta) > 1 : TickDelta != 0);
		if(Paused && m_Initialized)
			return m_Age;
		if(NewSwing || DeltaSeconds > 0.25f || TargetAge >= 0.2f)
		{
			m_Initialized = true;
			m_AttackTick = AttackTick;
			m_Age = TargetAge;
		}
		else if(std::isfinite(DeltaSeconds) && DeltaSeconds > 0.0f)
		{
			// Adjacent reconciled attack ticks belong to the same visible swing.
			m_AttackTick = AttackTick;
			const float Advance = DeltaSeconds + std::clamp(TargetAge - (m_Age + DeltaSeconds), -DeltaSeconds * 0.3f, DeltaSeconds * 0.3f);
			m_Age = std::min(0.2f, m_Age + Advance);
		}
		return m_Age;
	}
};

inline bool DespiteAntipingEnabled(const CConfig &Config)
{
	return Config.m_TcDespiteAntiping && Config.m_ClPredict && Config.m_ClAntiPing &&
	       Config.m_ClAntiPingPlayers && Config.m_TcAntiPingImproved && !Config.m_ClAntiPingSmooth;
}

inline int DespiteRuntimeProfile(const CConfig &Config, const CDespiteAutoPing &Auto)
{
	return Config.m_TcDespiteAutoPing && DespiteAntipingEnabled(Config) && Auto.Ready() ? Auto.Profile() : Config.m_TcDespiteProfile;
}

inline int DespiteRuntimeSmoothness(const CConfig &Config, const CDespiteAutoPing &Auto)
{
	return Config.m_TcDespiteAutoPing && DespiteAntipingEnabled(Config) && Auto.Ready() ? Auto.Smoothness() : Config.m_TcDespiteSmoothness;
}

inline void SetDespiteManualSmoothness(CConfig &Config, const CDespiteAutoPing &Auto, int Smoothness)
{
	if(Config.m_TcDespiteAutoPing && DespiteAntipingEnabled(Config) && Auto.Ready())
	{
		Config.m_TcDespiteProfile = Auto.Profile();
		Config.m_TcAntiPingNegativeBuffer = Auto.Profile() == 2 ? 1 : 0;
	}
	Config.m_TcDespiteAutoPing = 0;
	Config.m_TcDespitePingPreset = 0;
	Config.m_TcDespiteSmoothness = std::clamp(Smoothness, 0, 100);
}

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_DESPITE_ANTIPING_H
