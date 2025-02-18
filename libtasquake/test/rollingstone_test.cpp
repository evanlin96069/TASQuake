#include "catch_amalgamated.hpp"
#include "bench.hpp"
#include "libtasquake/optimizer.hpp"

TEST_CASE("Stone rolls up slope")
{
    TASQuake::RollingStone stone;
    stone.Init(0, 1, 1, 10);
    for(size_t i=1; stone.ShouldContinue(i); ++i) {
        stone.NextValue();
    }
    REQUIRE(stone.m_dCurrentValue == 10);
}

static void CornerFunc(TASQuake::Player* player) {
    TASQuake::MemorylessSim(player);
    if(player->m_vecPos.x < 10.0) {
        player->m_vecPos.y = std::min(player->m_vecPos.y, 10.0f);
    }
}

static void PinHoleFunc(TASQuake::Player* player) {
    TASQuake::MemorylessSim(player);
    const float PIN_SIZE = 1.0f;
    // prevent players from moving past 50 y if they dont pass within the pinhole
    if(player->m_vecPos.y > 50 && player->m_vecPos.y < 52 && std::abs(player->m_vecPos.x) > PIN_SIZE / 2) {
        player->m_vecPos.y = 50;
    }
}

TEST_CASE("PinHoleBench") {
    PlaybackInfo info;
    FrameBlock block1;
    block1.convars["tas_strafe_yaw"] = 0.0;
    block1.convars["tas_strafe_type"] = 3.0f;
    block1.convars["tas_strafe"] = 1.0;

    FrameBlock block2;
    block2.convars["tas_strafe_yaw"] = 90.0;
    block2.frame = 100;

    info.current_script.blocks.push_back(block1);
    info.current_script.blocks.push_back(block2);

    TASQuake::OptimizerSettings settings;
    settings.m_iEndOffset = 37;
    settings.m_uResetToBestIterations = 1;
    settings.m_uGiveUpAfterNoProgress = 999;
    settings.m_vecAlgorithmData.push_back(TASQuake::AlgorithmEnum::StrafeAdjuster);
    settings.m_vecAlgorithmData.push_back(TASQuake::AlgorithmEnum::RNGStrafer);
    settings.m_vecAlgorithmData.push_back(TASQuake::AlgorithmEnum::RNGBlockMover);
    settings.m_vecAlgorithmData.push_back(TASQuake::AlgorithmEnum::FrameBlockMover);
    settings.m_vecAlgorithmData.push_back(TASQuake::AlgorithmEnum::TurnOptimizer);

    std::printf("Pin hole bench: ");
    TASQuake::BenchTest(PinHoleFunc, &settings, &info);
}

TEST_CASE("Turn bench") {
    PlaybackInfo info;
    FrameBlock block1;
    block1.convars["tas_strafe_yaw"] = 0.0;
    block1.convars["tas_strafe"] = 1.0;

    FrameBlock block2;
    block2.convars["tas_strafe_yaw"] = 90.0;
    block2.frame = 100;

    info.current_script.blocks.push_back(block1);
    info.current_script.blocks.push_back(block2);

    TASQuake::OptimizerSettings settings;
    settings.m_bUseNodes = false;
    settings.m_iEndOffset = 37;
    settings.m_uResetToBestIterations = 1;
    settings.m_vecAlgorithmData.push_back(TASQuake::AlgorithmEnum::StrafeAdjuster);
    settings.m_vecAlgorithmData.push_back(TASQuake::AlgorithmEnum::RNGStrafer);
    settings.m_vecAlgorithmData.push_back(TASQuake::AlgorithmEnum::RNGBlockMover);
    settings.m_vecAlgorithmData.push_back(TASQuake::AlgorithmEnum::FrameBlockMover);
    settings.m_vecAlgorithmData.push_back(TASQuake::AlgorithmEnum::TurnOptimizer);

    std::printf("Turn bench: ");
    TASQuake::BenchTest(CornerFunc, &settings, &info);
}

TEST_CASE("Optimizer bench") {
    PlaybackInfo info;
    FrameBlock block1;
    block1.convars["tas_strafe_yaw"] = 0.0;
    block1.convars["tas_strafe"] = 1.0;

    FrameBlock block2;
    block2.convars["tas_strafe_yaw"] = 90.0;
    block2.frame = 100;

    info.current_script.blocks.push_back(block1);
    info.current_script.blocks.push_back(block2);

    TASQuake::OptimizerSettings settings;
    settings.m_bUseNodes = false;
    settings.m_iEndOffset = 37;
    settings.m_uResetToBestIterations = 1;
    settings.m_vecAlgorithmData.push_back(TASQuake::AlgorithmEnum::FrameBlockMover);

    std::printf("Frame block mover bench: ");
    TASQuake::BenchTest(CornerFunc, &settings, &info);
}
