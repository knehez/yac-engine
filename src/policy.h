#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

constexpr std::size_t POLICY_FEATURE_COUNT = 18;
constexpr std::size_t POLICY_HIDDEN1_SIZE = 32;
constexpr std::size_t POLICY_HIDDEN2_SIZE = 16;

using PolicyFeatures = std::array<double, POLICY_FEATURE_COUNT>;

struct PolicyTrainingSample
{
    std::vector<PolicyFeatures> moves;
    std::vector<double> target;
    double confidence = 1.0;
};

// A deliberately small policy network. It does not evaluate positions and
// cannot override chess rules or proven search results; it only supplies an
// MCTS prior that decides which legal moves receive attention first.
class TinyPolicyNetwork
{
public:
    TinyPolicyNetwork(
        std::string modelPath = "yac_policy.model",
        std::string samplesPath = "yac_policy.samples");

    double logit(const PolicyFeatures &features) const;
    std::vector<double> distribution(const std::vector<PolicyFeatures> &moves) const;

    void addSample(PolicyTrainingSample sample, bool persist = true);
    bool flushSamples() const;
    bool trainIfReady(std::size_t minimumNewSamples = 50);
    bool trainNow();
    void clear();

    bool isTrained() const { return updateCount > 0; }
    std::size_t sampleCount() const { return samples.size(); }
    std::size_t totalSampleCount() const { return totalSamplesSeen; }
    std::size_t trainingUpdates() const { return updateCount; }

private:
    void initializeNeutral();
    bool train(std::size_t epochs, double learningRate);
    bool loadModel();
    bool saveModel() const;
    bool loadSamples();
    bool saveSamples() const;

    std::array<std::array<double, POLICY_FEATURE_COUNT>, POLICY_HIDDEN1_SIZE> inputWeights{};
    std::array<double, POLICY_HIDDEN1_SIZE> hidden1Bias{};
    std::array<std::array<double, POLICY_HIDDEN1_SIZE>, POLICY_HIDDEN2_SIZE> hiddenWeights{};
    std::array<double, POLICY_HIDDEN2_SIZE> hidden2Bias{};
    std::array<double, POLICY_HIDDEN2_SIZE> outputWeights{};
    double outputBias = 0.0;

    std::vector<PolicyTrainingSample> samples;
    std::size_t totalSamplesSeen = 0;
    std::size_t trainedAtSampleCount = 0;
    std::size_t updateCount = 0;
    std::string modelPath;
    std::string samplesPath;
};
