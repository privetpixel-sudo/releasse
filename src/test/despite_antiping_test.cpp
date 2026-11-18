#include <game/client/components/tclient/despite_antiping.h>
#include <game/client/components/tclient/despite_button_motion.h>

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

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
