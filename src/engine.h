#pragma once

#include "util.h"
#include "chessboard.h"
#include "policy.h"
#include <chrono>

class Node;

struct NodeStatistics
{
    float win = 0;
    float lost = 0;
    uint32_t trials = 0;
};

enum class MateVerification
{
    NONE,
    PROVEN,
    REFUTED,
    UNKNOWN
};

struct MctsMoveInfo
{
    Move move;
    double value = 0.0;
    int visits = 0;
    bool proven = false;
    MateVerification mateVerification = MateVerification::NONE;
    double aiPrior = 0.0;
};

class Engine
{
public:
    Engine();
    void start(void);
    uint64_t perft(int depth);
    void setFENCode(const char *fenstr);
    void move(Move m);
    Move go(int searchDepth = 12);
    Move goMCTS(int maxIterations = 10000);
private:
    FRIEND_TEST(mctsTest, SearchRestoresRootPosition);
    FRIEND_TEST(mctsTest, TerminalRolloutDistinguishesMateFromStalemate);
    FRIEND_TEST(mctsTest, StaticEvaluationUsesSideToMove);
    FRIEND_TEST(evaluationTest, TaperedEvaluationPrefersKingSafetyWithHeavyPieces);
    FRIEND_TEST(evaluationTest, TaperedEvaluationRewardsActiveEndgameKing);
    FRIEND_TEST(mctsTest, ForcedMateVerificationProvesMateInOne);
    FRIEND_TEST(mctsTest, ForcedMateVerificationRejectsQuietPosition);
    FRIEND_TEST(mctsTest, ForcingMateVerificationProvesMateInSix);
    FRIEND_TEST(mctsTest, MateVerificationHonorsTimeLimit);
    FRIEND_TEST(policyTest, MoveFeaturesRecognizeDevelopmentAndKingMove);
    FRIEND_TEST(policyTest, ProvenMateCreatesTrustedSample);
    FRIEND_TEST(policyTest, ExternalLabelAcceptsOnlyLegalMoves);
    double monteCarloSimulation(int depth, bool *mateFound = nullptr);
    MateVerification verifyForcedMate(
        Color attacker,
        int pliesRemaining,
        int &nodeBudget,
        bool checkingMovesOnly = false);
    double evaluatePosition();
    Move selectTacticalMove(const Moves &moves, int *selectedScore = nullptr);
    int tacticalMoveScore(Move move);
    PolicyFeatures policyFeatures(Move move);
    double learnedRelativePrior(const Node *node, Move selectedMove);
    std::vector<double> rootPolicyDistribution(const Node *root);
    bool collectPolicySample(
        const Node *root,
        int maxIterations,
        const std::array<std::string, 4> &checkpoints,
        int checkpointCount);
    bool collectExternalPolicySample(
        const std::string &bestMove,
        double confidence,
        bool persist = true);
    void handleUCI();
    std::string gameMoves;
    void parseMove(std::string move, Move *m);
    NodeStatistics monteCarloSearch(const char *fen, Move startingMove);
    ChessBoard board;
    Color enginePlayColor;
    int monteCarloDepth;
    double lastMctsValue = 0.0;
    int lastMctsVisits = 0;
    double mctsExploration = 1.5;
    TinyPolicyNetwork policy;
    bool useLearnedPolicy = false;
    bool policyLearning = false;
    double policyWeight = 0.5;
    int mateProofTimeMs = 2000;
    std::chrono::steady_clock::time_point mateProofDeadline{};
    bool mateProofDeadlineActive = false;
    bool lastPolicySampleCollected = false;
    bool lastPolicyTrained = false;
    std::vector<MctsMoveInfo> lastMctsMoves;
};
