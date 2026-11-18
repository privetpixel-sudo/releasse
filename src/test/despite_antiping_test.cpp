#include <game/client/components/tclient/despite_antiping.h>

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
