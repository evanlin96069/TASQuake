#pragma once

#include <cstdint>
#include <memory>
#include <random>
#include <vector>
#include "libtasquake/vector.hpp"
#include "libtasquake/script_parse.hpp"
#include "libtasquake/script_playback.hpp"

namespace TASQuake {
    class Optimizer;

    class OptimizerAlgorithm {
    public:
        virtual void Mutate(TASScript* script, Optimizer* opt) = 0;
        virtual void ReportResult(double efficacy) {};
        virtual void Reset() = 0;
        virtual bool WantsToRun(TASScript* script) { return true; } // True if the algorithm could do something useful
        virtual bool WantsToContinue() = 0; // In some case the algorithm might want to run multiple iterations in a row
        virtual int IterationsExpected() { return 1; } // How many iterations the algorithm is expected to run
        // The iteration count is used to divided the time evenly between different algorithms
    };

    enum class BinarySearchState { 
        NoSearch, // No search is in progress
        MappingSpace, // Some improvement found, looking for max that gives worse result than current value
        BinarySearch, // Found min and max, now searching
        Finished
    };

    struct ValueEfficacyPair {
        double value = 0;
        double efficacy = 0;
    };

    enum class CliffState { NotCliffing, InProgress, Finished };

    // In finding a cliff, we expect the function to rise until it falls off a cliff
    // Therefore we can expect the value to steadily rise until we reach the edge
    struct CliffFinder {
        void Init(double edgeEfficacy, double edgePosition, double groundEfficacy, double groundPosition, double epsilon=1e-5);
        void Init(const std::vector<TASQuake::ValueEfficacyPair>& vec, double epsilon=1e-5);
        void Report(double result);
        double GetValue() const;
        void Reset();

        CliffState m_eState = CliffState::NotCliffing;
        double m_dEdgeEfficacy = 0;
        double m_dEdge = 0;
        double m_dGroundEfficacy = 0;
        double m_dGround = 0;
        double m_dEpsilon = 1e-5;
    };

    struct BinSearcher {
        // If min addition set, provides a lower bound for the next value to be tried
        void Init(double orig, double orig_efficacy, double max, double eps=1e-5);
        double GetValue() const;
        void Report(double result); // Report the result of the iteration in higher = better format
        void Reset();

        const std::uint32_t m_uMappingIterations = 5;
        CliffFinder m_Cliffer;

        double m_dRangeMax = 0;
        double m_dOriginalValue = 0;
        double m_dEPS = 1e-5;
        std::vector<ValueEfficacyPair> m_vecMapping;
        std::uint32_t m_uMappingIteration = 0;
        BinarySearchState m_eSearchState = BinarySearchState::NoSearch;
        bool m_bInitialized = false;
    };

    // The world's only stone that rolls uphill
    struct RollingStone {
        void Init(double efficacy, double startValue, double startDelta, double maxValue);
        bool ShouldContinue(double newEfficacy) const;
        void NextValue();

        double m_dMultiplicationFactor = 2;
        double m_dPrevEfficacy = 0;
        double m_dCurrentValue = 0;
        double m_dPrevDelta = 0;
        double m_dMax = 0;
    };

    enum class AlgorithmEnum { TurnOptimizer, RNGStrafer, RNGBlockMover, RNGShooter, StrafeAdjuster, FrameBlockMover };

    class TurnOptimizer : public OptimizerAlgorithm {
    public:
        virtual void Mutate(TASScript* script, Optimizer* opt) override;
        virtual void Reset() override;
        virtual bool WantsToRun(TASScript* script) override;
        virtual bool WantsToContinue() override;
        virtual int IterationsExpected() override { return 1; }
        virtual void ReportResult(double efficacy) override;
    private:
        void Init(TASScript* script, Optimizer* opt);
        std::int32_t m_iTurnIndex = -1;
        std::int32_t m_iStrafeIndex = -1;
        float m_fOrigStrafeYaw = 0.0f;
        double m_dSearchMax = 0.0;
        BinSearcher m_Searcher;
    };

    class RNGShooter : public OptimizerAlgorithm {
    public:
        virtual void Mutate(TASScript* script, Optimizer* opt) override;
        virtual void Reset() override;
        virtual bool WantsToRun(TASScript* script) override;
        virtual bool WantsToContinue() override;
        virtual int IterationsExpected() override { return 1; }
        virtual void ReportResult(double efficacy) override;
    };

    class RNGStrafer : public OptimizerAlgorithm {
    public:
        virtual void Mutate(TASScript* script, Optimizer* opt) override;
        virtual void Reset() override;
        virtual bool WantsToRun(TASScript* script) override;
        virtual bool WantsToContinue() override;
        virtual int IterationsExpected() override { return 1; }
        virtual void ReportResult(double efficacy) override;
    };

    class RNGBlockMover : public OptimizerAlgorithm {
    public:
        virtual void Mutate(TASScript* script, Optimizer* opt) override;
        virtual void Reset() override;
        virtual bool WantsToRun(TASScript* script) override;
        virtual bool WantsToContinue() override;
        virtual int IterationsExpected() override { return 1; }
        virtual void ReportResult(double efficacy) override;
    };

    class StrafeAdjuster : public OptimizerAlgorithm {
    public:
        virtual void Mutate(TASScript* script, Optimizer* opt) override;
        virtual void Reset() override;
        virtual bool WantsToRun(TASScript* script) override;
        virtual bool WantsToContinue() override;
        virtual int IterationsExpected() override { return 1; }
        virtual void ReportResult(double efficacy) override;
    private:
        // These values are only non-default in the case that the algorithm found some improvement
        // and now wants to RollingStone over it
        std::int32_t m_iCurrentBlockIndex = -1;
        RollingStone m_Stone;
    };

    class FrameBlockMover : public OptimizerAlgorithm {
    public:
        virtual void Mutate(TASScript* script, Optimizer* opt) override;
        virtual void Reset() override;
        virtual bool WantsToRun(TASScript* script) override;
        virtual bool WantsToContinue() override;
        virtual int IterationsExpected() override { return 1; }
        virtual void ReportResult(double efficacy) override;
    private:
        // These values are only non-default in the case that the algorithm found some improvement
        // and now wants to RollingStone over it
        std::int32_t m_iCurrentBlockIndex = -1;
        RollingStone m_Stone;
    };

    std::vector<double> GetCompoundingProbs(const std::vector<std::shared_ptr<OptimizerAlgorithm>> algorithms);
    size_t SelectIndex(double value, const std::vector<double>& compoundingProbs);

    struct FrameData {
        Vector pos;
        double m_dVelTheta = 999.0;
        void FindSmallestStrafeYawIncrements(float strafe_yaw, float& min, float& max) const; 
    };

    struct ExtendedFrameData {
        FrameData m_frameData;
        float m_fHP = 100;
        float m_fAP = 0;
        double m_dTime = 0.0;
        bool m_bDied = false;
        bool m_bIntermission = false;
        bool m_bTeleported = false;
    };

    enum class OptimizerState { ContinueIteration, NewIteration, Stop };
    enum class OptimizerGoal { Undetermined, PlusX, NegX, PlusY, NegY, Time, PlusZ, NegZ, Kills, Teleporter };

    const char* OptimizerGoalStr(OptimizerGoal goal);
    struct RunConditions;

    struct OptimizerRun {
        double m_dEfficacy = std::numeric_limits<double>::lowest();
        PlaybackInfo playbackInfo;
        bool m_bFinishedLevel = false;
        bool m_bDied = false;
        double m_dLevelTime = 0.0;
        double m_dTeleportTime = 1000.0;
        std::vector<FrameData> m_vecData;
        std::uint32_t m_uKills = 0;
        std::uint32_t m_uSecrets = 0;
        std::uint32_t m_uCenterPrints = 0;
        float m_fHP = 100.0f;
        float m_fAP = 0;

        void ResetIteration();
        void CalculateEfficacy(OptimizerGoal goal, const RunConditions* conditions);
        double RunEfficacy() const { return m_dEfficacy; };
        bool IsBetterThan(const OptimizerRun& run) const;
        void StrafeBounds(size_t blockIndex, float& min, float& max) const;
        void WriteToBuffer(TASQuakeIO::BufferWriteInterface& writer) const;
        void ReadFromBuffer(TASQuakeIO::BufferReadInterface& reader);
    };

    struct OptimizerSettings;

    struct RunConditions {
        bool m_bInitialized = false;
        std::vector<Vector> m_vecNodes;
        std::uint32_t m_uKills = 0;
        std::uint32_t m_uSecrets = 0;
        std::uint32_t m_uCenterPrints = 0;
        float m_fTotalHP = 0;

        void Init(const OptimizerRun* run, const OptimizerSettings* settings);
        bool FulfillsConditions(const OptimizerRun* run) const;
        void Reset();
    };

    OptimizerGoal AutoGoal(const Vector& secondLast, const Vector& last);
    OptimizerGoal AutoGoal(const OptimizerRun& run);
    double ConvertTimeToEfficacy(double time);
    double ConvertEfficacyToTime(double efficacy);

    struct OptimizerSettings {
        // Determine which algorithms should be used
        OptimizerGoal m_Goal = OptimizerGoal::Undetermined; // Automatically determined by first run
        std::uint32_t m_uResetToBestIterations = 3;
        std::uint32_t m_uGiveUpAfterNoProgress = 999;
        std::int32_t m_iEndOffset = 36; // Where to end the optimizer path as frames from the last frame
        std::int32_t m_iFrames = -1; // If positive, determines the number of frames
        std::vector<AlgorithmEnum> m_vecAlgorithmData;
        std::vector<std::shared_ptr<OptimizerAlgorithm>> m_vecAlgorithms;
        std::vector<Vector> m_vecInputNodes; // We have some baseline version to compare against
        bool m_bSecondaryGoals = false; // Require secondary goals to match the initial run
        bool m_bUseNodes = true; // Require subsequent runs to match the initial run in terms of path taken

        void WriteToBuffer(TASQuakeIO::BufferWriteInterface& writer) const;
        void ReadFromBuffer(TASQuakeIO::BufferReadInterface& reader);
    };

    void InitAlgorithms(const OptimizerSettings* settings, std::vector<std::shared_ptr<OptimizerAlgorithm>>& m_vecAlgorithms);

    struct Optimizer {
        void ResetIteration();
        // Gets the current frame block or null if no block for current frame
        const FrameBlock* GetCurrentFrameBlock() const;
        // Runner calls this after every frame
        OptimizerState OnRunnerFrame(const ExtendedFrameData* data);
        void _FinishIteration(OptimizerState& state);
        // Run the init function with the actual full script, the optimizer figures out the relevant bit from playbackInfo
        bool Init(const PlaybackInfo* playback, const OptimizerSettings* settings);
        void Seed(std::uint32_t value);
        double Random(double min, double max);
        int32_t RandomInt(int32_t min, int32_t max);
        size_t RandomizeIndex();

        std::vector<std::shared_ptr<OptimizerAlgorithm>> m_vecAlgorithms;
        OptimizerRun m_currentBest; // The current best run
        OptimizerRun m_currentRun; // The current run
        OptimizerSettings m_settings; // Current settings for the optimizer
        std::mt19937 m_RNG;
        std::vector<double> m_vecCompoundingProbs;
        std::int32_t m_iCurrentAlgorithm = -1;
        std::uint32_t m_uLastFrame = 1;
        std::uint32_t m_uIteration = 0;
        double m_dMaxTime = 0;
        std::uint32_t m_uIterationsWithoutProgress = 0; // How many iterations have been ran without progress, determines when we should reset back to best
        RunConditions m_runConditions;
    };
}
