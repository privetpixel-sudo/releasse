#include <game/client/components/tclient/despite_antiping.h>
#include <game/client/components/tclient/despite_button_motion.h>

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

TEST(DespiteAutoPing, StableServerLatencySelectsAllNinePresets)
{
	for(int Preset = 1; Preset <= 9; ++Preset)
	{
		CDespiteAutoPing Auto;
		Auto.Observe(1, 50, Preset == 9 ? 150 : 20 + Preset * 10, 0);
		EXPECT_TRUE(Auto.Ready());
		EXPECT_EQ(Auto.Preset(), Preset);
		CConfig Expected{};
		ApplyDespitePingPreset(Expected, Preset);
		EXPECT_EQ(Auto.Profile(), Expected.m_TcDespiteProfile);
		EXPECT_EQ(Auto.Smoothness(), Expected.m_TcDespiteSmoothness);
	}
}

TEST(DespiteAutoPing, DuplicateTicksCannotAccelerateTuningAndBoundaryNoiseDoesNotSwitch)
{
	CDespiteAutoPing Auto;
	Auto.Observe(1, 50, 74, 0);
	for(int Frame = 0; Frame < 500; ++Frame)
		Auto.Observe(1, 50, 200, 30);
	EXPECT_FLOAT_EQ(Auto.PingMs(), 74.0f);
	for(int Tick = 2; Tick <= 1500; ++Tick)
		Auto.Observe(Tick, 50, Tick % 2 ? 74 : 76, 0);
	EXPECT_EQ(Auto.Preset(), 5);
}

TEST(DespiteAutoPing, BriefSpikeIsIgnoredButSustainedChangesRetuneWithSlowerRelease)
{
	CDespiteAutoPing Auto;
	Auto.Observe(0, 50, 40, 0);
	Auto.Observe(1, 50, 180, 0);
	for(int Tick = 2; Tick <= 100; ++Tick)
		Auto.Observe(Tick, 50, 40, 0);
	EXPECT_EQ(Auto.Preset(), 2);
	for(int Tick = 101; Tick <= 350; ++Tick)
		Auto.Observe(Tick, 50, 130, 0);
	EXPECT_EQ(Auto.Preset(), 9);
	for(int Tick = 351; Tick <= 400; ++Tick)
		Auto.Observe(Tick, 50, 40, 0);
	EXPECT_EQ(Auto.Preset(), 9);
	for(int Tick = 401; Tick <= 800; ++Tick)
		Auto.Observe(Tick, 50, 40, 0);
	EXPECT_EQ(Auto.Preset(), 2);
}

TEST(DespiteAutoPing, StrengthEasesAndPredictionJitterAddsBoundedProtection)
{
	CDespiteAutoPing Stable, Unstable;
	Stable.Observe(0, 50, 80, 0);
	Unstable.Observe(0, 50, 80, 0);
	for(int Tick = 1; Tick <= 250; ++Tick)
	{
		const int Previous = Unstable.Smoothness();
		Stable.Observe(Tick, 50, 80, 0);
		Unstable.Observe(Tick, 50, 80, 40);
		EXPECT_LE(std::abs(Unstable.Smoothness() - Previous), 3);
	}
	EXPECT_GT(Unstable.Smoothness(), Stable.Smoothness());
	EXPECT_LE(Unstable.Smoothness(), Stable.Smoothness() + 10);
	EXPECT_LE(Unstable.Smoothness(), 100);
}

TEST(DespiteAutoPing, AdaptationUsesElapsedSnapshotTime)
{
	CDespiteAutoPing Slow, Fast;
	Slow.Observe(0, 50, 40, 0);
	Fast.Observe(0, 50, 40, 0);
	for(int Tick = 1; Tick <= 250; ++Tick)
	{
		Fast.Observe(Tick, 50, 100, 8);
		if(Tick % 2 == 0)
			Slow.Observe(Tick, 50, 100, 8);
	}
	EXPECT_NEAR(Slow.PingMs(), Fast.PingMs(), 0.001f);
	EXPECT_EQ(Slow.Preset(), Fast.Preset());
	EXPECT_NEAR(Slow.Smoothness(), Fast.Smoothness(), 1);
}

TEST(DespiteAutoPing, ResetRewindLongGapAndMissingLatencyCannotReuseOldHistory)
{
	CDespiteAutoPing Auto;
	Auto.Observe(100, 50, 150, 20);
	Auto.Observe(1, 50, 30, 0);
	EXPECT_EQ(Auto.Preset(), 1);
	Auto.Observe(1000, 50, 100, 0);
	EXPECT_EQ(Auto.Preset(), 8);
	Auto.Reset();
	EXPECT_FALSE(Auto.Ready());
	Auto.Observe(1, 50, -1, 0);
	EXPECT_FALSE(Auto.Ready());
	Auto.Observe(2, 50, std::numeric_limits<int>::max(), std::numeric_limits<float>::quiet_NaN());
	EXPECT_TRUE(Auto.Ready());
	EXPECT_EQ(Auto.Preset(), 9);
	EXPECT_LE(Auto.Smoothness(), 100);
}

TEST(DespiteAutoPing, EnablingPreservesManualChoicesAndManualPresetsDisableAuto)
{
	CConfig Config{};
	Config.m_TcDespiteProfile = 0;
	Config.m_TcDespiteSmoothness = 47;
	Config.m_TcDespitePingPreset = 3;
	Config.m_ClPredictionMargin = 17;
	ApplyDespiteAutoPing(Config);
	EXPECT_TRUE(DespiteAntipingEnabled(Config));
	EXPECT_EQ(Config.m_TcDespiteAutoPing, 1);
	EXPECT_EQ(Config.m_TcDespiteProfile, 0);
	EXPECT_EQ(Config.m_TcDespiteSmoothness, 47);
	EXPECT_EQ(Config.m_TcDespitePingPreset, 3);
	EXPECT_EQ(Config.m_ClPredictionMargin, 17);
	ApplyDespitePingPreset(Config, 8);
	EXPECT_EQ(Config.m_TcDespiteAutoPing, 0);
	Config.m_TcDespiteAutoPing = 1;
	ApplyDespiteAntipingPreset(Config, 1);
	EXPECT_EQ(Config.m_TcDespiteAutoPing, 0);
}

TEST(DespiteAutoPing, RuntimeSettingsFollowMeasurementWhileManualFallbackRemainsIntact)
{
	CConfig Config{};
	Config.m_TcDespiteSmoothness = 47;
	Config.m_TcDespiteProfile = 2;
	ApplyDespiteAutoPing(Config);
	CDespiteAutoPing Auto;
	EXPECT_EQ(DespiteRuntimeSmoothness(Config, Auto), 47);
	Auto.Observe(1, 50, 40, 0);
	EXPECT_EQ(DespiteRuntimeSmoothness(Config, Auto), 42);
	EXPECT_EQ(DespiteRuntimeProfile(Config, Auto), 0);
	EXPECT_EQ(Config.m_TcDespiteSmoothness, 47);
	EXPECT_EQ(Config.m_TcDespiteProfile, 2);
	Config.m_TcDespiteAntiping = 0;
	EXPECT_EQ(DespiteRuntimeSmoothness(Config, Auto), 47);
	Config.m_TcDespiteAntiping = 1;
	Config.m_TcDespiteAutoPing = 0;
	EXPECT_EQ(DespiteRuntimeSmoothness(Config, Auto), 47);
}

TEST(DespiteAutoPing, ManualSliderPreservesAdvancedPreferencesAndOnlyAdoptsAnActiveAutoProfile)
{
	CDespiteAutoPing Auto;
	Auto.Observe(1, 50, 40, 0);
	for(bool Enabled : {false, true})
	{
		CConfig Config{};
		ApplyDespiteAntipingPreset(Config, 0);
		Config.m_TcDespiteAntiping = Enabled;
		Config.m_TcAntiPingNegativeBuffer = 1;
		SetDespiteManualSmoothness(Config, Auto, 53);
		EXPECT_EQ(Config.m_TcAntiPingNegativeBuffer, 1);
		EXPECT_EQ(Config.m_TcDespiteSmoothness, 53);
		EXPECT_EQ(Config.m_TcDespiteAntiping, Enabled);
	}
	CConfig Config{};
	ApplyDespiteAutoPing(Config);
	Config.m_TcDespiteProfile = 2;
	Config.m_TcAntiPingNegativeBuffer = 1;
	SetDespiteManualSmoothness(Config, Auto, 43);
	EXPECT_EQ(Config.m_TcDespiteAutoPing, 0);
	EXPECT_EQ(Config.m_TcDespiteProfile, 0);
	EXPECT_EQ(Config.m_TcAntiPingNegativeBuffer, 0);
	EXPECT_EQ(Config.m_TcDespiteSmoothness, 43);
}

TEST(DespitePresets, NinePresetsIncreaseSmoothnessAndPreserveNetworkMargin)
{
	int PreviousStrength = 0;
	for(int Preset = 1; Preset <= 9; ++Preset)
	{
		CConfig Config{};
		Config.m_ClPredictionMargin = 17;
		Config.m_TcRemoveAnti = Config.m_TcSwapGhosts = Config.m_TcFastInputRepredictHook = 1;
		ApplyDespitePingPreset(Config, Preset);
		EXPECT_TRUE(DespiteAntipingEnabled(Config));
		EXPECT_EQ(Config.m_TcDespitePingPreset, Preset);
		EXPECT_GT(Config.m_TcDespiteSmoothness, PreviousStrength);
		EXPECT_LE(Config.m_TcDespiteSmoothness, 100);
		EXPECT_EQ(Config.m_ClPredictionMargin, 17);
		EXPECT_EQ(Config.m_ClPredictEvents, 1);
		EXPECT_EQ(Config.m_ClSubTickAiming, 1);
		EXPECT_EQ(Config.m_ClPredictDummy, 1);
		EXPECT_EQ(Config.m_TcRemoveAnti + Config.m_TcSwapGhosts + Config.m_TcFastInputRepredictHook, 0);
		PreviousStrength = Config.m_TcDespiteSmoothness;
	}
}

TEST(DespitePresets, OutOfRangePresetsClampAndManualProfileClearsSelection)
{
	CConfig Config{};
	ApplyDespitePingPreset(Config, -10);
	EXPECT_EQ(Config.m_TcDespitePingPreset, 1);
	ApplyDespitePingPreset(Config, 100);
	EXPECT_EQ(Config.m_TcDespitePingPreset, 9);
	ApplyDespiteAntipingPreset(Config, 1);
	EXPECT_EQ(Config.m_TcDespitePingPreset, 0);
}

TEST(DespiteHammer, CorrectionsCannotRewindOrJumpAnActiveSwing)
{
	CDespiteHammerAnimation Hammer;
	EXPECT_FLOAT_EQ(Hammer.Update(100, 0.04f, 0.01f, false), 0.04f);
	const float Reconciled = Hammer.Update(101, 0.01f, 0.01f, false);
	EXPECT_GE(Reconciled, 0.047f);
	EXPECT_LE(Reconciled, 0.053f);
	const float Forward = Hammer.Update(101, 0.15f, 0.01f, false);
	EXPECT_GT(Forward, Reconciled);
	EXPECT_LE(Forward, Reconciled + 0.01301f);
	EXPECT_FLOAT_EQ(Hammer.Update(120, 0.005f, 0.01f, false), 0.005f);
}

TEST(DespiteHammer, ChainedCorrectionsStayContinuousAndRapidTunedAttacksRestart)
{
	CDespiteHammerAnimation Hammer;
	Hammer.Update(100, 0.04f, 0.01f, false);
	Hammer.Update(101, 0.01f, 0.01f, false);
	EXPECT_GT(Hammer.Update(102, 0.01f, 0.01f, false), 0.05f);
	EXPECT_FLOAT_EQ(Hammer.Update(103, 0.005f, 0.01f, false, false), 0.005f);
}

TEST(DespiteHammer, PausesGapsAndResetDoNotReplayOldSwings)
{
	CDespiteHammerAnimation Hammer;
	Hammer.Update(100, 0.08f, 0.01f, false);
	EXPECT_FLOAT_EQ(Hammer.Update(100, 0.10f, 0.01f, true), 0.08f);
	EXPECT_FLOAT_EQ(Hammer.Update(100, 0.10f, 0.0f, false), 0.08f);
	EXPECT_FLOAT_EQ(Hammer.Update(100, 0.12f, 0.5f, false), 0.12f);
	EXPECT_FLOAT_EQ(Hammer.Update(100, 5.0f, 0.01f, false), 0.2f);
	Hammer.Reset();
	EXPECT_FLOAT_EQ(Hammer.Update(100, 0.03f, 0.01f, false), 0.03f);
}

TEST(DespiteHammer, UncorrectedSwingProgressIsIndependentOfFrameRate)
{
	for(int Fps : {30, 60, 144, 240})
	{
		CDespiteHammerAnimation Hammer;
		Hammer.Update(100, 0.0f, 0.0f, false);
		for(int Frame = 1; Frame <= Fps / 5; ++Frame)
			EXPECT_NEAR(Hammer.Update(100, float(Frame) / Fps, 1.0f / Fps, false), float(Frame) / Fps, 0.00001f);
	}
}

TEST(DespiteCorrection, StrongerPresetSoftensCorrectionRecoveryFurther)
{
	CDespiteCorrectionSmoother Light, Strong;
	Light.Update(vec2(2, 0), 0.02f, 100, 0, 35, 0);
	Strong.Update(vec2(2, 0), 0.02f, 100, 0, 90, 0);
	EXPECT_GT(std::abs(Strong.Update(vec2(0, 0), 0.02f, 100, 0, 90, 0).x), std::abs(Light.Update(vec2(0, 0), 0.02f, 100, 0, 35, 0).x));
}

// These tests catch loss of adaptation, frame-dependent sampling, stale state,
// unsafe arithmetic and incomplete/conflicting presets at the actual config boundary.
TEST(DespiteAntiping, StableEightyMillisecondsUsesSelectedRecoveryPace)
{
	CDespiteAntiping Filter;
	for(int Tick = 1; Tick <= 100; ++Tick)
		Filter.Observe(Tick, 80);
	EXPECT_FLOAT_EQ(Filter.JitterMs(), 0.0f);
	EXPECT_FLOAT_EQ(Filter.RecoveryScale(0), 0.85f);
	EXPECT_FLOAT_EQ(Filter.RecoveryScale(1), 1.25f);
	EXPECT_FLOAT_EQ(Filter.RecoveryScale(2), 1.8f);
}

TEST(DespiteAntiping, JitterSlowsRecoveryAndStableSamplesReleaseIt)
{
	CDespiteAntiping Filter;
	for(int Tick = 1; Tick <= 80; ++Tick)
		Filter.Observe(Tick, Tick % 2 ? 60 : 100);
	EXPECT_GT(Filter.JitterMs(), 15.0f);
	EXPECT_GT(Filter.RecoveryScale(1), 1.5f);
	for(int Tick = 81; Tick <= 300; ++Tick)
		Filter.Observe(Tick, 80);
	EXPECT_LT(Filter.JitterMs(), 0.01f);
	EXPECT_NEAR(Filter.RecoveryScale(1), 1.25f, 0.001f);
}

TEST(DespiteAntiping, MultipleRendersOfOneTickDoNotChangeAdaptation)
{
	CDespiteAntiping Filter;
	Filter.Observe(1, 80);
	Filter.Observe(2, 120);
	const float Jitter = Filter.JitterMs();
	EXPECT_GT(Jitter, 0.0f);
	for(int Frame = 0; Frame < 1000; ++Frame)
		Filter.Observe(2, 300);
	EXPECT_FLOAT_EQ(Filter.JitterMs(), Jitter);
}

TEST(DespiteAntiping, TickRewindAndLongGapStartFreshHistory)
{
	CDespiteAntiping Filter;
	Filter.Observe(20, 80);
	Filter.Observe(21, 400);
	EXPECT_GT(Filter.JitterMs(), 0.0f);
	Filter.Observe(1, 40);
	EXPECT_FLOAT_EQ(Filter.JitterMs(), 0.0f);
	Filter.Observe(2, 80);
	EXPECT_GT(Filter.JitterMs(), 0.0f);
	Filter.Observe(500, 120);
	EXPECT_FLOAT_EQ(Filter.JitterMs(), 0.0f);
}

TEST(DespiteAntiping, ResetDropsPreviousConnectionHistory)
{
	CDespiteAntiping Filter;
	Filter.Observe(1, 80);
	Filter.Observe(2, 400);
	EXPECT_GT(Filter.JitterMs(), 0.0f);
	Filter.Reset();
	Filter.Observe(2, 40);
	EXPECT_FLOAT_EQ(Filter.JitterMs(), 0.0f);
	EXPECT_FLOAT_EQ(Filter.RecoveryScale(1), 1.25f);
}

TEST(DespiteAntiping, ZeroNegativeAndExtremeSamplesStayFiniteAndBounded)
{
	CDespiteAntiping Filter;
	const int aSamples[] = {0, -1, std::numeric_limits<int>::max(), std::numeric_limits<int>::min(), 80};
	int Tick = 1;
	for(int Sample : aSamples)
	{
		Filter.Observe(Tick++, Sample);
		for(int Profile : {-100, 0, 1, 2, 100})
		{
			EXPECT_TRUE(std::isfinite(Filter.RecoveryScale(Profile)));
			EXPECT_GE(Filter.RecoveryScale(Profile), 0.85f);
			EXPECT_LE(Filter.RecoveryScale(Profile), 4.0f);
		}
	}
}

TEST(DespiteAntiping, PresetsEnableCompatiblePredictionWithoutChangingNetworkMargin)
{
	for(int Profile = 0; Profile < 3; ++Profile)
	{
		CConfig Config{};
		Config.m_ClAntiPingSmooth = 1;
		Config.m_ClAntiPingLimit = 200;
		Config.m_ClPredictionMargin = 17;
		ApplyDespiteAntipingPreset(Config, Profile);
		EXPECT_TRUE(DespiteAntipingEnabled(Config));
		EXPECT_EQ(Config.m_ClAntiPingSmooth, 0);
		EXPECT_EQ(Config.m_ClAntiPingLimit, 0);
		EXPECT_EQ(Config.m_ClAntiPingPercent, 100);
		EXPECT_EQ(Config.m_ClAntiPingPreInput, 1);
		EXPECT_EQ(Config.m_TcFastInput, 1);
		EXPECT_EQ(Config.m_TcFastInputAmount, 20);
		EXPECT_EQ(Config.m_ClAntiPingWeapons, 1);
		EXPECT_EQ(Config.m_ClAntiPingGrenade, 1);
		EXPECT_EQ(Config.m_ClPredictionMargin, 17);
	}
}

TEST(DespiteAntiping, MissingDependenciesAndConflictingSmoothingReportInactive)
{
	CConfig Config{};
	ApplyDespiteAntipingPreset(Config, 1);
	EXPECT_TRUE(DespiteAntipingEnabled(Config));
	Config.m_ClPredict = 0;
	EXPECT_FALSE(DespiteAntipingEnabled(Config));
	ApplyDespiteAntipingPreset(Config, 1);
	Config.m_ClAntiPingPlayers = 0;
	EXPECT_FALSE(DespiteAntipingEnabled(Config));
	ApplyDespiteAntipingPreset(Config, 1);
	Config.m_TcAntiPingImproved = 0;
	EXPECT_FALSE(DespiteAntipingEnabled(Config));
	ApplyDespiteAntipingPreset(Config, 1);
	Config.m_ClAntiPingSmooth = 1;
	EXPECT_FALSE(DespiteAntipingEnabled(Config));
}

// Catch instantaneous confidence jumps, FPS-dependent easing, axis overshoot,
// stale state after teleports, and adding delay when smoothing is disabled.
TEST(DespiteBlend, CorrectionIsSoftButConverges)
{
	CDespitePredictionBlend Blend;
	Blend.Update(vec2(1, 1), 0.02f, 1, 0, 65);
	const vec2 First = Blend.Update(vec2(0, 0), 0.02f, 1, 0, 65);
	EXPECT_GT(First.x, 0.0f);
	EXPECT_LT(First.x, 1.0f);
	vec2 Result;
	for(int Tick = 0; Tick < 30; ++Tick)
		Result = Blend.Update(vec2(0, 0), 0.02f, 1, 0, 65);
	EXPECT_LT(Result.x, 0.001f);
}

TEST(DespiteBlend, EqualElapsedTimeHasEqualResultAtDifferentFrameRates)
{
	CDespitePredictionBlend Slow, Fast;
	Slow.Update(vec2(1, 0), 0.0f, 1, 8, 65);
	Fast.Update(vec2(1, 0), 0.0f, 1, 8, 65);
	vec2 A, B;
	for(int Frame = 0; Frame < 6; ++Frame)
		A = Slow.Update(vec2(0, 1), 1.0f / 60.0f, 1, 8, 65);
	for(int Frame = 0; Frame < 24; ++Frame)
		B = Fast.Update(vec2(0, 1), 1.0f / 240.0f, 1, 8, 65);
	EXPECT_NEAR(A.x, B.x, 0.00001f);
	EXPECT_NEAR(A.y, B.y, 0.00001f);
	EXPECT_GT(A.x, 0.0f);
}

TEST(DespiteBlend, MoreSmoothnessAndJitterSoftenTheSameCorrection)
{
	CDespitePredictionBlend Light, Smooth, Unstable;
	Light.Update(vec2(1, 1), 0.0f, 1, 0, 20);
	Smooth.Update(vec2(1, 1), 0.0f, 1, 0, 90);
	Unstable.Update(vec2(1, 1), 0.0f, 1, 30, 90);
	const float A = Light.Update(vec2(0, 0), 0.02f, 1, 0, 20).x;
	const float B = Smooth.Update(vec2(0, 0), 0.02f, 1, 0, 90).x;
	const float C = Unstable.Update(vec2(0, 0), 0.02f, 1, 30, 90).x;
	EXPECT_GT(B, A);
	EXPECT_GT(C, B);
}

TEST(DespiteBlend, ResetImmediatelyUsesNewTargetAfterTeleport)
{
	CDespitePredictionBlend Blend;
	Blend.Update(vec2(1, 1), 0.02f, 2, 20, 100);
	EXPECT_GT(Blend.Update(vec2(0, 0), 0.02f, 2, 20, 100).x, 0.0f);
	Blend.Reset();
	EXPECT_EQ(Blend.Update(vec2(0, 1), 0.02f, 2, 20, 100), vec2(0, 1));
}

TEST(DespiteBlend, DisabledSmoothingImmediatelyUsesTarget)
{
	CDespitePredictionBlend Blend;
	Blend.Update(vec2(1, 1), 0.02f, 1, 0, 65);
	EXPECT_EQ(Blend.Update(vec2(0, 1), 0.02f, 1, 0, 0), vec2(0, 1));
}

TEST(DespiteBlend, ZeroElapsedTimeHoldsBlendAndAxesNeverOvershoot)
{
	CDespitePredictionBlend Blend;
	Blend.Update(vec2(1, 0), 0.02f, 1, 0, 65);
	EXPECT_EQ(Blend.Update(vec2(0, 1), 0.0f, 1, 0, 65), vec2(1, 0));
	for(int Tick = 0; Tick < 200; ++Tick)
	{
		const vec2 Result = Blend.Update(Tick % 2 ? vec2(0, 1) : vec2(1, 0), 0.02f, 1, 20, 65);
		EXPECT_GE(Result.x, 0.0f);
		EXPECT_LE(Result.x, 1.0f);
		EXPECT_GE(Result.y, 0.0f);
		EXPECT_LE(Result.y, 1.0f);
	}
}

// Catch instant hover switches, snapping on reversal, FPS-dependent timing,
// missing press feedback and invalid elapsed time poisoning the visual state.
TEST(DespiteButton, HoverGrowsGraduallyAndExitSettlesBack)
{
	CDespiteButtonMotion Motion;
	Motion.Update(true, false, 1.0f / 60.0f);
	const float First = Motion.Scale(0.045f);
	EXPECT_GT(First, 1.0f);
	EXPECT_LT(First, 1.045f);
	Motion.Update(false, false, 1.0f / 60.0f);
	EXPECT_LT(Motion.Scale(0.045f), First);
	EXPECT_GT(Motion.Scale(0.045f), 1.0f);
	for(int Frame = 0; Frame < 120; ++Frame)
		Motion.Update(false, false, 1.0f / 60.0f);
	EXPECT_NEAR(Motion.Scale(0.045f), 1.0f, 0.00001f);
}

TEST(DespiteButton, ReenteringDuringExitReversesFromCurrentScale)
{
	CDespiteButtonMotion Motion;
	for(int Frame = 0; Frame < 10; ++Frame)
		Motion.Update(true, false, 1.0f / 60.0f);
	const float BeforeExit = Motion.Scale(0.045f);
	Motion.Update(false, false, 0.02f);
	const float Leaving = Motion.Scale(0.045f);
	EXPECT_LT(Leaving, BeforeExit);
	Motion.Update(true, false, 0.01f);
	EXPECT_GT(Motion.Scale(0.045f), Leaving);
	EXPECT_LT(Motion.Scale(0.045f), 1.045f);
}

TEST(DespiteButton, EqualElapsedTimeIsIndependentOfFrameRate)
{
	CDespiteButtonMotion Slow, Fast;
	for(int Frame = 0; Frame < 6; ++Frame)
		Slow.Update(true, false, 1.0f / 60.0f);
	for(int Frame = 0; Frame < 24; ++Frame)
		Fast.Update(true, false, 1.0f / 240.0f);
	EXPECT_NEAR(Slow.Scale(0.045f), Fast.Scale(0.045f), 0.00001f);
	EXPECT_GT(Slow.Scale(0.045f), 1.0f);
}

TEST(DespiteButton, PressShrinksAndReleaseReturnsSmoothly)
{
	CDespiteButtonMotion Motion;
	for(int Frame = 0; Frame < 60; ++Frame)
		Motion.Update(true, false, 1.0f / 60.0f);
	const float HoverScale = Motion.Scale(0.045f);
	Motion.Update(true, true, 0.02f);
	const float PressScale = Motion.Scale(0.045f);
	EXPECT_LT(PressScale, HoverScale);
	EXPECT_GT(Motion.Press(), 0.0f);
	Motion.Update(true, false, 0.02f);
	EXPECT_GT(Motion.Scale(0.045f), PressScale);
	EXPECT_LT(Motion.Scale(0.045f), HoverScale);
}

TEST(DespiteButton, InvalidTimeHoldsStateAndLongFrameStaysBounded)
{
	CDespiteButtonMotion Motion;
	Motion.Update(true, false, 0.02f);
	const float Before = Motion.Scale(0.045f);
	Motion.Update(false, true, -1.0f);
	EXPECT_FLOAT_EQ(Motion.Scale(0.045f), Before);
	Motion.Update(false, true, std::numeric_limits<float>::quiet_NaN());
	EXPECT_FLOAT_EQ(Motion.Scale(0.045f), Before);
	Motion.Update(true, false, 1000.0f);
	EXPECT_TRUE(std::isfinite(Motion.Scale(0.045f)));
	EXPECT_GE(Motion.Scale(0.045f), Before);
	EXPECT_LE(Motion.Scale(0.045f), 1.045f);
}

TEST(DespiteInteraction, ProtectsHammerRangeAndAttachedHooks)
{
	EXPECT_FLOAT_EQ(DespiteInteractionPriority(80.0f, false), 1.0f);
	EXPECT_FLOAT_EQ(DespiteInteractionPriority(400.0f, false), 0.0f);
	EXPECT_FLOAT_EQ(DespiteInteractionPriority(400.0f, true), 1.0f);
	EXPECT_GT(DespiteInteractionPriority(144.0f, false), 0.0f);
	EXPECT_LT(DespiteInteractionPriority(144.0f, false), 1.0f);
	EXPECT_GT(DespiteInteractionPriority(120.0f, false), DespiteInteractionPriority(160.0f, false));
}

TEST(DespiteCorrection, SmoothsSmallCorrectionWithoutDelayingSteadyMotion)
{
	CDespiteCorrectionSmoother Filter;
	for(int Tick = 0; Tick < 50; ++Tick)
		EXPECT_EQ(Filter.Update(vec2(0, 0), 0.02f, 100, 0, 80, 0), vec2(0, 0));
	const vec2 Offset = Filter.Update(vec2(6, 0), 0.02f, 100, 0, 80, 0);
	EXPECT_LT(Offset.x, 0.0f);
	EXPECT_GE(Offset.x, -6.0f);
	vec2 Settled;
	for(int Tick = 0; Tick < 50; ++Tick)
		Settled = Filter.Update(vec2(0, 0), 0.02f, 100, 0, 80, 0);
	EXPECT_NEAR(Settled.x, 0.0f, 0.001f);
}

TEST(DespiteCorrection, HundredMillisecondCorrectionTraceHasLowerStepEnergy)
{
	// 50 Hz movement, alternating four-unit prediction corrections every five
	// ticks (100 ms). This is a reproducible synthetic trace, not a live RTT claim.
	CDespiteCorrectionSmoother Filter;
	float PreviousNoise = 0.0f, PreviousRaw = 0.0f, PreviousDisplay = 0.0f;
	float RawEnergy = 0.0f, DisplayEnergy = 0.0f;
	for(int Tick = 1; Tick <= 300; ++Tick)
	{
		const float Noise = (Tick / 5) % 2 ? 2.0f : -2.0f;
		const float Raw = Tick * 6.0f + Noise;
		const vec2 Offset = Filter.Update(vec2(Noise - PreviousNoise, 0), 0.02f, 100, 8, 80, 0);
		const float Display = Raw + Offset.x;
		if(Tick > 10)
		{
			RawEnergy += std::pow(Raw - PreviousRaw - 6.0f, 2.0f);
			DisplayEnergy += std::pow(Display - PreviousDisplay - 6.0f, 2.0f);
		}
		EXPECT_LE(std::abs(Offset.x), 12.0f);
		PreviousNoise = Noise;
		PreviousRaw = Raw;
		PreviousDisplay = Display;
	}
	EXPECT_LT(DisplayEnergy, RawEnergy * 0.55f);
}

TEST(DespiteCorrection, ProtectingInteractionsDropsResidualVisualOffset)
{
	CDespiteCorrectionSmoother Filter;
	EXPECT_LT(Filter.Update(vec2(8, 0), 0.02f, 100, 10, 80, 0).x, 0.0f);
	EXPECT_EQ(Filter.Update(vec2(8, 0), 0.02f, 100, 10, 80, 1), vec2(0, 0));
	EXPECT_EQ(Filter.Update(vec2(0, 0), 0.02f, 100, 10, 80, 0), vec2(0, 0));
}

TEST(DespiteCorrection, TeleportResetAndDisabledModeCannotLeaveGhostOffset)
{
	CDespiteCorrectionSmoother Filter;
	EXPECT_LT(Filter.Update(vec2(6, 0), 0.02f, 100, 0, 80, 0).x, 0.0f);
	EXPECT_EQ(Filter.Update(vec2(1000, 0), 0.02f, 100, 0, 80, 0), vec2(0, 0));
	Filter.Update(vec2(6, 0), 0.02f, 100, 0, 80, 0);
	EXPECT_EQ(Filter.Update(vec2(0, 0), 0.02f, 100, 0, 0, 0), vec2(0, 0));
	Filter.Update(vec2(6, 0), 0.02f, 100, 0, 80, 0);
	Filter.Reset();
	EXPECT_EQ(Filter.Update(vec2(0, 0), 0.02f, 100, 0, 80, 0), vec2(0, 0));
}

TEST(DespiteCorrection, DecayDependsOnElapsedTimeAndOffsetRemainsBounded)
{
	CDespiteCorrectionSmoother Slow, Fast;
	Slow.Update(vec2(8, 0), 0.02f, 100, 5, 100, 0);
	Fast.Update(vec2(8, 0), 0.02f, 100, 5, 100, 0);
	vec2 A, B;
	for(int Frame = 0; Frame < 6; ++Frame)
		A = Slow.Update(vec2(0, 0), 1.0f / 60.0f, 100, 5, 100, 0);
	for(int Frame = 0; Frame < 24; ++Frame)
		B = Fast.Update(vec2(0, 0), 1.0f / 240.0f, 100, 5, 100, 0);
	EXPECT_NEAR(A.x, B.x, 0.0001f);
	EXPECT_LT(A.x, 0.0f);
	for(int Tick = 0; Tick < 200; ++Tick)
		EXPECT_LE(length(Slow.Update(vec2(10, 10), 0.02f, 100, 20, 100, 0)), 12.001f);
}

TEST(DespiteAntiping, HighPingSetupEnablesAimAndFeedbackWithoutExtraNetworkDelay)
{
	CConfig Config{};
	Config.m_ClPredictionMargin = 17;
	Config.m_TcRemoveAnti = 1;
	Config.m_TcUnpredOthersInFreeze = 1;
	Config.m_TcSwapGhosts = 1;
	Config.m_TcFastInputRepredictHook = 1;
	ApplyDespiteHighPingPreset(Config);
	EXPECT_TRUE(DespiteAntipingEnabled(Config));
	EXPECT_EQ(Config.m_ClSubTickAiming, 1);
	EXPECT_EQ(Config.m_ClPredictEvents, 1);
	EXPECT_EQ(Config.m_ClPredictDummy, 1);
	EXPECT_EQ(Config.m_TcFastInputRepredictHook, 0);
	EXPECT_EQ(Config.m_TcRemoveAnti, 0);
	EXPECT_EQ(Config.m_TcUnpredOthersInFreeze, 0);
	EXPECT_EQ(Config.m_TcSwapGhosts, 0);
	EXPECT_EQ(Config.m_ClPredictionMargin, 17);
	EXPECT_GT(Config.m_TcDespiteSmoothness, 65);
}

TEST(DespiteCorrection, SkippedPredictionTicksNeverSmoothOrdinaryMotion)
{
	CDespiteCorrectionSmoother Filter;
	EXPECT_LT(Filter.Update(vec2(6, 0), 0.02f, 100, 0, 80, 0).x, 0.0f);
	// At 30 FPS, two or more 50 Hz ticks can pass between predictions. An old
	// world advanced only once cannot be compared with the new final position.
	EXPECT_EQ(Filter.Update(vec2(6, 0), 0.04f, 100, 0, 80, 0, false), vec2(0, 0));
	EXPECT_EQ(Filter.Update(vec2(0, 0), 0.02f, 100, 0, 80, 0), vec2(0, 0));
}

TEST(DespiteInteraction, PrecisionPriorityAlsoOverridesLowConfidenceDistanceLimit)
{
	EXPECT_FLOAT_EQ(DespiteEffectiveConfidence(0.1f, 1.0f), 1.0f);
	EXPECT_FLOAT_EQ(DespiteEffectiveConfidence(0.1f, 0.0f), 0.1f);
	EXPECT_FLOAT_EQ(DespiteEffectiveConfidence(0.1f, 0.5f), 0.55f);
}
