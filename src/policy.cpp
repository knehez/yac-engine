#include "policy.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <limits>

namespace
{
constexpr std::size_t MAX_POLICY_SAMPLES = 20000;
constexpr char SAMPLE_MAGIC[8] = {'Y', 'A', 'C', 'P', 'S', '0', '1', '\0'};

template <typename T>
bool readValue(std::ifstream &input, T &value)
{
    return bool(input.read(reinterpret_cast<char *>(&value), sizeof(T)));
}

template <typename T>
void writeValue(std::ofstream &output, const T &value)
{
    output.write(reinterpret_cast<const char *>(&value), sizeof(T));
}
}

TinyPolicyNetwork::TinyPolicyNetwork(std::string modelPath, std::string samplesPath)
    : modelPath(std::move(modelPath)), samplesPath(std::move(samplesPath))
{
    initializeNeutral();
    loadModel();
    loadSamples();
}

void TinyPolicyNetwork::initializeNeutral()
{
    // A fresh model must be exactly neutral. Zero weights produce equal
    // logits for every legal move; symmetry is broken by the first gradient
    // because the output layer receives the move-dependent hidden gradient
    // after its small deterministic initialization below.
    for (std::size_t hidden1 = 0; hidden1 < POLICY_HIDDEN1_SIZE; ++hidden1)
    {
        hidden1Bias[hidden1] = 0.0;
        for (std::size_t feature = 0; feature < POLICY_FEATURE_COUNT; ++feature)
        {
            inputWeights[hidden1][feature] = 0.0;
        }
    }
    for (std::size_t hidden2 = 0; hidden2 < POLICY_HIDDEN2_SIZE; ++hidden2)
    {
        hidden2Bias[hidden2] = 0.0;
        // Deterministic non-zero downstream weights break symmetry during the
        // first gradient step while zero input weights keep a fresh model
        // exactly neutral for every legal move.
        outputWeights[hidden2] = 0.25 * std::sin(double(hidden2 + 1));
        for (std::size_t hidden1 = 0; hidden1 < POLICY_HIDDEN1_SIZE; ++hidden1)
        {
            hiddenWeights[hidden2][hidden1] =
                0.15 * std::sin(double((hidden2 + 1) * (hidden1 + 1)));
        }
    }
    outputBias = 0.0;
    updateCount = 0;
    trainedAtSampleCount = 0;
}

double TinyPolicyNetwork::logit(const PolicyFeatures &features) const
{
    std::array<double, POLICY_HIDDEN1_SIZE> hidden1Activation{};
    for (std::size_t hidden1 = 0; hidden1 < POLICY_HIDDEN1_SIZE; ++hidden1)
    {
        double activation = hidden1Bias[hidden1];
        for (std::size_t feature = 0; feature < POLICY_FEATURE_COUNT; ++feature)
        {
            activation += inputWeights[hidden1][feature] * features[feature];
        }
        hidden1Activation[hidden1] = std::tanh(activation);
    }

    double result = outputBias;
    for (std::size_t hidden2 = 0; hidden2 < POLICY_HIDDEN2_SIZE; ++hidden2)
    {
        double activation = hidden2Bias[hidden2];
        for (std::size_t hidden1 = 0; hidden1 < POLICY_HIDDEN1_SIZE; ++hidden1)
        {
            activation += hiddenWeights[hidden2][hidden1] * hidden1Activation[hidden1];
        }
        result += outputWeights[hidden2] * std::tanh(activation);
    }
    return std::clamp(result, -20.0, 20.0);
}

std::vector<double> TinyPolicyNetwork::distribution(const std::vector<PolicyFeatures> &moves) const
{
    std::vector<double> probabilities(moves.size(), 0.0);
    if (moves.empty())
    {
        return probabilities;
    }

    std::vector<double> logits;
    logits.reserve(moves.size());
    double maximum = -std::numeric_limits<double>::infinity();
    for (const auto &features : moves)
    {
        logits.push_back(logit(features));
        maximum = std::max(maximum, logits.back());
    }

    double sum = 0.0;
    for (std::size_t index = 0; index < logits.size(); ++index)
    {
        probabilities[index] = std::exp(logits[index] - maximum);
        sum += probabilities[index];
    }
    for (double &probability : probabilities)
    {
        probability /= sum;
    }
    return probabilities;
}

void TinyPolicyNetwork::addSample(PolicyTrainingSample sample, bool persist)
{
    if (sample.moves.empty() || sample.moves.size() != sample.target.size())
    {
        return;
    }
    sample.confidence = std::clamp(sample.confidence, 0.0, 1.0);
    if (samples.size() == MAX_POLICY_SAMPLES)
    {
        samples.erase(samples.begin());
    }
    samples.push_back(std::move(sample));
    ++totalSamplesSeen;
    if (persist)
    {
        saveSamples();
    }
}

bool TinyPolicyNetwork::flushSamples() const
{
    return saveSamples();
}

bool TinyPolicyNetwork::trainIfReady(std::size_t minimumNewSamples)
{
    if (totalSamplesSeen < trainedAtSampleCount + minimumNewSamples)
    {
        return false;
    }
    return train(6, 0.015);
}

bool TinyPolicyNetwork::trainNow()
{
    return train(10, 0.012);
}

bool TinyPolicyNetwork::train(std::size_t epochs, double learningRate)
{
    if (samples.empty())
    {
        return false;
    }

    for (std::size_t epoch = 0; epoch < epochs; ++epoch)
    {
        for (const auto &sample : samples)
        {
            const auto probabilities = distribution(sample.moves);
            std::array<std::array<double, POLICY_FEATURE_COUNT>, POLICY_HIDDEN1_SIZE> inputGradient{};
            std::array<double, POLICY_HIDDEN1_SIZE> hidden1BiasGradient{};
            std::array<std::array<double, POLICY_HIDDEN1_SIZE>, POLICY_HIDDEN2_SIZE> hiddenGradient{};
            std::array<double, POLICY_HIDDEN2_SIZE> hidden2BiasGradient{};
            std::array<double, POLICY_HIDDEN2_SIZE> outputGradient{};
            double outputBiasGradient = 0.0;

            for (std::size_t move = 0; move < sample.moves.size(); ++move)
            {
                std::array<double, POLICY_HIDDEN1_SIZE> hidden1Activation{};
                for (std::size_t hidden1 = 0; hidden1 < POLICY_HIDDEN1_SIZE; ++hidden1)
                {
                    double activation = hidden1Bias[hidden1];
                    for (std::size_t feature = 0; feature < POLICY_FEATURE_COUNT; ++feature)
                    {
                        activation += inputWeights[hidden1][feature] * sample.moves[move][feature];
                    }
                    hidden1Activation[hidden1] = std::tanh(activation);
                }

                std::array<double, POLICY_HIDDEN2_SIZE> hidden2Activation{};
                for (std::size_t hidden2 = 0; hidden2 < POLICY_HIDDEN2_SIZE; ++hidden2)
                {
                    double activation = hidden2Bias[hidden2];
                    for (std::size_t hidden1 = 0; hidden1 < POLICY_HIDDEN1_SIZE; ++hidden1)
                    {
                        activation += hiddenWeights[hidden2][hidden1] * hidden1Activation[hidden1];
                    }
                    hidden2Activation[hidden2] = std::tanh(activation);
                }

                const double logitGradient = sample.confidence *
                                             (probabilities[move] - sample.target[move]);
                outputBiasGradient += logitGradient;
                std::array<double, POLICY_HIDDEN2_SIZE> hidden2Delta{};
                for (std::size_t hidden2 = 0; hidden2 < POLICY_HIDDEN2_SIZE; ++hidden2)
                {
                    outputGradient[hidden2] += logitGradient * hidden2Activation[hidden2];
                    hidden2Delta[hidden2] = logitGradient * outputWeights[hidden2] *
                                            (1.0 - hidden2Activation[hidden2] * hidden2Activation[hidden2]);
                    hidden2BiasGradient[hidden2] += hidden2Delta[hidden2];
                    for (std::size_t hidden1 = 0; hidden1 < POLICY_HIDDEN1_SIZE; ++hidden1)
                    {
                        hiddenGradient[hidden2][hidden1] +=
                            hidden2Delta[hidden2] * hidden1Activation[hidden1];
                    }
                }
                for (std::size_t hidden1 = 0; hidden1 < POLICY_HIDDEN1_SIZE; ++hidden1)
                {
                    double hidden1Delta = 0.0;
                    for (std::size_t hidden2 = 0; hidden2 < POLICY_HIDDEN2_SIZE; ++hidden2)
                    {
                        hidden1Delta += hidden2Delta[hidden2] * hiddenWeights[hidden2][hidden1];
                    }
                    hidden1Delta *= 1.0 - hidden1Activation[hidden1] * hidden1Activation[hidden1];
                    hidden1BiasGradient[hidden1] += hidden1Delta;
                    for (std::size_t feature = 0; feature < POLICY_FEATURE_COUNT; ++feature)
                    {
                        inputGradient[hidden1][feature] += hidden1Delta * sample.moves[move][feature];
                    }
                }
            }

            // Softmax cross-entropy already sums to a normalized class
            // gradient; dividing once more by the legal-move count would make
            // learning vanish in ordinary 30-50 move chess positions.
            const double scale = learningRate;
            outputBias -= scale * std::clamp(outputBiasGradient, -5.0, 5.0);
            for (std::size_t hidden2 = 0; hidden2 < POLICY_HIDDEN2_SIZE; ++hidden2)
            {
                outputWeights[hidden2] -= scale * std::clamp(outputGradient[hidden2], -5.0, 5.0);
                hidden2Bias[hidden2] -= scale * std::clamp(hidden2BiasGradient[hidden2], -5.0, 5.0);
                for (std::size_t hidden1 = 0; hidden1 < POLICY_HIDDEN1_SIZE; ++hidden1)
                {
                    hiddenWeights[hidden2][hidden1] -=
                        scale * std::clamp(hiddenGradient[hidden2][hidden1], -5.0, 5.0);
                }
            }
            for (std::size_t hidden1 = 0; hidden1 < POLICY_HIDDEN1_SIZE; ++hidden1)
            {
                hidden1Bias[hidden1] -= scale * std::clamp(hidden1BiasGradient[hidden1], -5.0, 5.0);
                for (std::size_t feature = 0; feature < POLICY_FEATURE_COUNT; ++feature)
                {
                    inputWeights[hidden1][feature] -=
                        scale * std::clamp(inputGradient[hidden1][feature], -5.0, 5.0);
                }
            }
        }
    }

    ++updateCount;
    trainedAtSampleCount = totalSamplesSeen;
    return saveModel();
}

void TinyPolicyNetwork::clear()
{
    samples.clear();
    totalSamplesSeen = 0;
    initializeNeutral();
    std::remove(modelPath.c_str());
    std::remove(samplesPath.c_str());
}

bool TinyPolicyNetwork::loadModel()
{
    std::ifstream input(modelPath);
    std::string magic;
    if (!(input >> magic) || magic != "YAC_POLICY_V2")
    {
        return false;
    }
    std::size_t featureCount = 0;
    std::size_t hidden1Size = 0;
    std::size_t hidden2Size = 0;
    if (!(input >> featureCount >> hidden1Size >> hidden2Size) ||
        featureCount != POLICY_FEATURE_COUNT ||
        hidden1Size != POLICY_HIDDEN1_SIZE ||
        hidden2Size != POLICY_HIDDEN2_SIZE)
    {
        initializeNeutral();
        return false;
    }
    if (!(input >> updateCount >> trainedAtSampleCount))
    {
        initializeNeutral();
        return false;
    }
    for (auto &row : inputWeights)
    {
        for (double &weight : row)
        {
            input >> weight;
        }
    }
    for (double &bias : hidden1Bias)
    {
        input >> bias;
    }
    for (auto &row : hiddenWeights)
    {
        for (double &weight : row)
        {
            input >> weight;
        }
    }
    for (double &bias : hidden2Bias)
    {
        input >> bias;
    }
    for (double &weight : outputWeights)
    {
        input >> weight;
    }
    input >> outputBias;
    if (!input)
    {
        initializeNeutral();
        return false;
    }
    return true;
}

bool TinyPolicyNetwork::saveModel() const
{
    std::ofstream output(modelPath, std::ios::trunc);
    if (!output)
    {
        return false;
    }
    output.precision(17);
    output << "YAC_POLICY_V2\n"
           << POLICY_FEATURE_COUNT << ' ' << POLICY_HIDDEN1_SIZE << ' ' << POLICY_HIDDEN2_SIZE << '\n'
           << updateCount << ' ' << trainedAtSampleCount << '\n';
    for (const auto &row : inputWeights)
    {
        for (double weight : row)
        {
            output << weight << ' ';
        }
        output << '\n';
    }
    for (double bias : hidden1Bias)
    {
        output << bias << ' ';
    }
    output << '\n';
    for (const auto &row : hiddenWeights)
    {
        for (double weight : row)
        {
            output << weight << ' ';
        }
        output << '\n';
    }
    for (double bias : hidden2Bias)
    {
        output << bias << ' ';
    }
    output << '\n';
    for (double weight : outputWeights)
    {
        output << weight << ' ';
    }
    output << '\n' << outputBias << '\n';
    return bool(output);
}

bool TinyPolicyNetwork::loadSamples()
{
    std::ifstream input(samplesPath, std::ios::binary);
    char magic[8]{};
    if (!input.read(magic, sizeof(magic)) ||
        !std::equal(std::begin(magic), std::end(magic), std::begin(SAMPLE_MAGIC)))
    {
        return false;
    }
    std::uint64_t storedTotal = 0;
    std::uint32_t count = 0;
    if (!readValue(input, storedTotal) || !readValue(input, count) || count > MAX_POLICY_SAMPLES)
    {
        return false;
    }

    std::vector<PolicyTrainingSample> loaded;
    loaded.reserve(count);
    for (std::uint32_t sampleIndex = 0; sampleIndex < count; ++sampleIndex)
    {
        PolicyTrainingSample sample;
        std::uint32_t moveCount = 0;
        if (!readValue(input, sample.confidence) || !readValue(input, moveCount) || moveCount > 256)
        {
            return false;
        }
        sample.moves.resize(moveCount);
        sample.target.resize(moveCount);
        for (std::uint32_t move = 0; move < moveCount; ++move)
        {
            for (std::size_t feature = 0; feature < POLICY_FEATURE_COUNT; ++feature)
            {
                float value = 0.0f;
                if (!readValue(input, value))
                {
                    return false;
                }
                sample.moves[move][feature] = value;
            }
            float target = 0.0f;
            if (!readValue(input, target))
            {
                return false;
            }
            sample.target[move] = target;
        }
        loaded.push_back(std::move(sample));
    }
    samples = std::move(loaded);
    totalSamplesSeen = std::max<std::size_t>(storedTotal, samples.size());
    return true;
}

bool TinyPolicyNetwork::saveSamples() const
{
    std::ofstream output(samplesPath, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        return false;
    }
    output.write(SAMPLE_MAGIC, sizeof(SAMPLE_MAGIC));
    const std::uint64_t storedTotal = totalSamplesSeen;
    const std::uint32_t count = static_cast<std::uint32_t>(samples.size());
    writeValue(output, storedTotal);
    writeValue(output, count);
    for (const auto &sample : samples)
    {
        writeValue(output, sample.confidence);
        const std::uint32_t moveCount = static_cast<std::uint32_t>(sample.moves.size());
        writeValue(output, moveCount);
        for (std::size_t move = 0; move < sample.moves.size(); ++move)
        {
            for (double feature : sample.moves[move])
            {
                const float value = static_cast<float>(feature);
                writeValue(output, value);
            }
            const float target = static_cast<float>(sample.target[move]);
            writeValue(output, target);
        }
    }
    return bool(output);
}
