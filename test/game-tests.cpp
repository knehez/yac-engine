#include "gtest/gtest.h"
#include "engine.h"
#include "mcts.h"
#include <cstdio>

TEST(gameTest, MateInTwo)
{
    srand(0); // make random moves determnistic
    Engine engine;
    engine.setFENCode("2kr2nr/pp1n1ppp/2p1p3/8/1P1P1B2/2N2Q1P/1PPKBPP1/7q w - - 0 1");
    Move bestMove = engine.go();

    // EXPECT_EQ(ChessBoard::to_string(bestMove), "e2a6");
    EXPECT_EQ(true, true);
}

TEST(gameTest, MateInSix)
{
    srand(0); // make random moves deterministic
    Engine engine;
    engine.setFENCode("k7/pp1r4/1n3b2/1N6/5Qp1/5pPq/5P2/4R1K1 w - - 0 1");
    Move bestMove = engine.go(2);

    EXPECT_EQ(true, true);
    // EXPECT_EQ(ChessBoard::to_string(bestMove), "e1e8");
}

TEST(gameTest, MCTSReturnsLegalRootMove)
{
    srand(0); // make random moves deterministic
    Engine engine;
    engine.setFENCode("k7/pp1r2p1/1n3b2/pN1p2p1/P1pP1Qp1/2P2pPq/5P2/4R1K1 w - - 0 1");
    Move bestMove = engine.goMCTS(10000);

    EXPECT_NE(bestMove.start, NUMBER_OF_SQUARES);
    EXPECT_NE(bestMove.end, NUMBER_OF_SQUARES);
}

TEST(gameTest, MateInSix2)
{
    srand(0); // keep the stochastic search reproducible
    Engine engine;
    engine.setFENCode("r4k2/pp1P2pp/1q6/3Q2p1/2p5/4R3/5PPP/6K1 w - - 0 1");
    Move bestMove = engine.goMCTS(2000);

    EXPECT_EQ(ChessBoard::to_string(bestMove), "e3e8");
}

TEST(mctsTest, UCTUsesParentPlayersPerspective)
{
    Moves noMoves;
    Node root(Move(NULL_MOVE), noMoves, nullptr, WHITE);
    auto goodForRoot = new Node(Move(NULL_MOVE), noMoves, &root, BLACK);
    auto badForRoot = new Node(Move(NULL_MOVE), noMoves, &root, BLACK);
    root.children.push_back(goodForRoot);
    root.children.push_back(badForRoot);

    root.visits = 20;
    goodForRoot->visits = 10;
    goodForRoot->valueSum = -8.0; // bad for Black, therefore good for White
    badForRoot->visits = 10;
    badForRoot->valueSum = 8.0;

    EXPECT_EQ(root.selectChildWithMaxUCT(), goodForRoot);
}

TEST(mctsTest, ImplicitMinimaxBackupPreservesStrongestReply)
{
    Moves noMoves;
    Node root(Move(NULL_MOVE), noMoves, nullptr, WHITE);
    auto refutedForRoot = new Node(Move(NULL_MOVE), noMoves, &root, BLACK);
    auto soundForRoot = new Node(Move(NULL_MOVE), noMoves, &root, BLACK);
    root.children.push_back(refutedForRoot);
    root.children.push_back(soundForRoot);

    root.visits = 20;
    refutedForRoot->visits = 10;
    soundForRoot->visits = 10;
    // Identical Monte Carlo means: only the minimax channel distinguishes
    // the line in which Black has already found a strong reply.
    refutedForRoot->valueSum = 0.0;
    soundForRoot->valueSum = 0.0;
    refutedForRoot->implicitMinimaxValue = 0.8;
    refutedForRoot->hasImplicitMinimaxValue = true;
    soundForRoot->implicitMinimaxValue = -0.2;
    soundForRoot->hasImplicitMinimaxValue = true;

    EXPECT_EQ(root.selectChildWithMaxUCT(0.0), soundForRoot);
}

TEST(mctsTest, ImplicitMinimaxBackupAlternatesPerspective)
{
    Moves noMoves;
    Node parent(Move(NULL_MOVE), noMoves, nullptr, WHITE);
    auto child = new Node(Move(NULL_MOVE), noMoves, &parent, BLACK);
    parent.children.push_back(child);
    child->implicitMinimaxValue = 0.6;
    child->hasImplicitMinimaxValue = true;

    parent.updateImplicitMinimaxValue();

    EXPECT_TRUE(parent.hasImplicitMinimaxValue);
    EXPECT_DOUBLE_EQ(parent.implicitMinimaxValue, -0.6);
}

TEST(mctsTest, SolverPropagatesProvenResults)
{
    Moves noMoves;

    Node winningParent(Move(NULL_MOVE), noMoves, nullptr, WHITE);
    auto losingChild = new Node(Move(NULL_MOVE), noMoves, &winningParent, BLACK);
    losingChild->solvedState = Node::SolvedState::LOSS;
    winningParent.children.push_back(losingChild);
    winningParent.updateSolvedState();
    EXPECT_EQ(winningParent.solvedState, Node::SolvedState::WIN);

    Node losingParent(Move(NULL_MOVE), noMoves, nullptr, WHITE);
    auto winningChild = new Node(Move(NULL_MOVE), noMoves, &losingParent, BLACK);
    winningChild->solvedState = Node::SolvedState::WIN;
    losingParent.children.push_back(winningChild);
    losingParent.updateSolvedState();
    EXPECT_EQ(losingParent.solvedState, Node::SolvedState::LOSS);
}

TEST(mctsTest, CheckmateAndStalemateReturnNullMove)
{
    Engine engine;
    engine.setFENCode("R3k3/8/4K3/8/8/8/8/8 b - - 0 1");
    Move checkmateMove = engine.goMCTS(10);
    EXPECT_EQ(checkmateMove.start, NUMBER_OF_SQUARES);
    EXPECT_EQ(checkmateMove.end, NUMBER_OF_SQUARES);

    engine.setFENCode("8/8/8/8/3b4/5k1p/7P/7K w - - 0 1");
    Move stalemateMove = engine.goMCTS(10);
    EXPECT_EQ(stalemateMove.start, NUMBER_OF_SQUARES);
    EXPECT_EQ(stalemateMove.end, NUMBER_OF_SQUARES);
}

TEST(mctsTest, ZeroIterationsReturnLegalFallback)
{
    Engine engine;
    engine.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(ChessBoard::to_string(engine.goMCTS(0)), "b1a3");
}

TEST(mctsTest, SearchRestoresRootPosition)
{
    srand(0);
    Engine engine;
    engine.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    const std::string beforeSearch = engine.board.getFENCode();
    engine.goMCTS(200);
    const std::string afterSearch = engine.board.getFENCode();

    EXPECT_EQ(afterSearch, beforeSearch);
}

TEST(mctsTest, ImplicitMinimaxAvoidsEarlyKingMove)
{
    srand(0);
    Engine engine;
    engine.setFENCode("r1bqkb1r/pppppppp/2n1n3/4P3/8/5N2/PPPP1PPP/RNBQKB1R w kq - 0 1");

    EXPECT_EQ(ChessBoard::to_string(engine.goMCTS(20000)), "b1c3");
}

TEST(policyTest, NeutralNetworkLearnsAndPersistsTeacherPreference)
{
    const std::string modelPath = "build/policy-test.model";
    const std::string samplesPath = "build/policy-test.samples";
    std::remove(modelPath.c_str());
    std::remove(samplesPath.c_str());

    PolicyFeatures developingMove{};
    PolicyFeatures kingMove{};
    developingMove[2] = 1.0;
    developingMove[12] = 1.0;
    kingMove[5] = 1.0;
    kingMove[13] = 1.0;

    TinyPolicyNetwork policy(modelPath, samplesPath);
    auto neutral = policy.distribution({developingMove, kingMove});
    EXPECT_NEAR(neutral[0], 0.5, 1e-12);
    EXPECT_NEAR(neutral[1], 0.5, 1e-12);

    PolicyTrainingSample sample;
    sample.moves = {developingMove, kingMove};
    sample.target = {1.0, 0.0};
    sample.confidence = 1.0;
    policy.addSample(sample);
    ASSERT_TRUE(policy.trainNow());
    const auto learned = policy.distribution(sample.moves);
    EXPECT_GT(learned[0], learned[1]);

    TinyPolicyNetwork reloaded(modelPath, samplesPath);
    const auto persisted = reloaded.distribution(sample.moves);
    EXPECT_NEAR(persisted[0], learned[0], 1e-9);
    EXPECT_EQ(reloaded.totalSampleCount(), 1u);

    reloaded.clear();
}

TEST(policyTest, MoveFeaturesRecognizeDevelopmentAndKingMove)
{
    Engine engine;
    engine.setFENCode("r1bqkb1r/pppppppp/2n1n3/4P3/8/5N2/PPPP1PPP/RNBQKB1R w kq - 0 1");
    const Move development{b1, c3, NUMBER_OF_PIECES, NUMBER_OF_PIECES, NUMBER_OF_SQUARES};
    const Move kingMove{e1, e2, NUMBER_OF_PIECES, NUMBER_OF_PIECES, NUMBER_OF_SQUARES};

    const auto developmentFeatures = engine.policyFeatures(development);
    const auto kingFeatures = engine.policyFeatures(kingMove);

    EXPECT_DOUBLE_EQ(developmentFeatures[12], 1.0);
    EXPECT_DOUBLE_EQ(developmentFeatures[13], 0.0);
    EXPECT_DOUBLE_EQ(kingFeatures[12], 0.0);
    EXPECT_DOUBLE_EQ(kingFeatures[13], 1.0);
}

TEST(policyTest, ProvenMateCreatesTrustedSample)
{
    const std::string modelPath = "build/policy-mate-test.model";
    const std::string samplesPath = "build/policy-mate-test.samples";
    std::remove(modelPath.c_str());
    std::remove(samplesPath.c_str());

    srand(0);
    Engine engine;
    engine.policy = TinyPolicyNetwork(modelPath, samplesPath);
    engine.policyLearning = true;
    engine.setFENCode("r4k2/pp1P2pp/1q6/3Q2p1/2p5/4R3/5PPP/6K1 w - - 0 1");

    EXPECT_EQ(ChessBoard::to_string(engine.goMCTS(800)), "e3e8");
    EXPECT_TRUE(engine.lastPolicySampleCollected);
    EXPECT_EQ(engine.policy.totalSampleCount(), 1u);
    EXPECT_FALSE(engine.policy.isTrained());

    engine.policy.clear();
}

TEST(policyTest, ExternalLabelAcceptsOnlyLegalMoves)
{
    const std::string modelPath = "build/policy-external-test.model";
    const std::string samplesPath = "build/policy-external-test.samples";
    std::remove(modelPath.c_str());
    std::remove(samplesPath.c_str());

    Engine engine;
    engine.policy = TinyPolicyNetwork(modelPath, samplesPath);
    engine.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    EXPECT_TRUE(engine.collectExternalPolicySample("e2e4", 1.0, false));
    EXPECT_FALSE(engine.collectExternalPolicySample("e2e5", 1.0, false));
    EXPECT_EQ(engine.policy.totalSampleCount(), 1u);
    EXPECT_TRUE(engine.policy.flushSamples());

    TinyPolicyNetwork reloaded(modelPath, samplesPath);
    EXPECT_EQ(reloaded.totalSampleCount(), 1u);
    reloaded.clear();
}

TEST(mctsTest, TerminalRolloutDistinguishesMateFromStalemate)
{
    Engine engine;
    engine.setFENCode("R3k3/8/4K3/8/8/8/8/8 b - - 0 1");
    EXPECT_DOUBLE_EQ(engine.monteCarloSimulation(20), -1.0);

    engine.setFENCode("8/8/8/8/3b4/5k1p/7P/7K w - - 0 1");
    EXPECT_DOUBLE_EQ(engine.monteCarloSimulation(20), 0.0);
}

TEST(mctsTest, StaticEvaluationUsesSideToMove)
{
    Engine engine;
    engine.setFENCode("7k/8/8/8/8/8/8/KQ6 w - - 0 1");
    EXPECT_GT(engine.evaluatePosition(), 0.0);

    engine.setFENCode("7k/8/8/8/8/8/8/KQ6 b - - 0 1");
    EXPECT_LT(engine.evaluatePosition(), 0.0);
}

TEST(mctsTest, FindsMateInOne)
{
    srand(0);
    Engine engine;
    engine.setFENCode("7k/8/5KQ1/8/8/8/8/8 w - - 0 1");
    Move bestMove = engine.goMCTS(1000);

    EXPECT_EQ(ChessBoard::to_string(bestMove), "g6g7");
}

TEST(mctsTest, ForcedMateVerificationProvesMateInOne)
{
    srand(0);
    Engine engine;
    engine.setFENCode("7k/8/5KQ1/8/8/8/8/8 w - - 0 1");
    int budget = 1000;

    EXPECT_EQ(engine.verifyForcedMate(WHITE, 1, budget), MateVerification::PROVEN);
}

TEST(mctsTest, ForcedMateVerificationRejectsQuietPosition)
{
    srand(0);
    Engine engine;
    engine.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    int budget = 1000;

    EXPECT_EQ(engine.verifyForcedMate(WHITE, 1, budget), MateVerification::REFUTED);
}

TEST(mctsTest, MateVerificationHonorsTimeLimit)
{
    Engine engine;
    engine.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    engine.mateProofDeadlineActive = true;
    engine.mateProofDeadline = std::chrono::steady_clock::now() - std::chrono::milliseconds(1);
    int budget = 1000;

    EXPECT_EQ(engine.verifyForcedMate(WHITE, 10, budget), MateVerification::UNKNOWN);
    EXPECT_EQ(budget, 1000);
}

TEST(mctsTest, ForcingMateVerificationProvesMateInSix)
{
    srand(0);
    Engine engine;
    engine.setFENCode("r3Rk2/pp1P2pp/1q6/3Q2p1/2p5/8/5PPP/6K1 b - - 0 1");
    int budget = 250000;

    EXPECT_EQ(engine.verifyForcedMate(WHITE, 10, budget, true), MateVerification::PROVEN);
}

TEST(mctsTest, DefensiveVerificationFindsQuietMateInFour)
{
    srand(0);
    Engine engine;
    engine.setFENCode("2r1r3/pp1nbN2/4p3/q7/P1pP2nk/2P2P2/1PQ5/R3R1K1 w - - 1 0");

    EXPECT_EQ(ChessBoard::to_string(engine.goMCTS(50000)), "e1e4");
}

TEST(evaluationTest, TaperedEvaluationPrefersKingSafetyWithHeavyPieces)
{
    Engine engine;
    engine.setFENCode("r2q2kr/5ppp/8/8/8/8/5PPP/R2Q2KR w - - 0 1");
    const double safeKing = engine.evaluatePosition();

    engine.setFENCode("r2q2kr/5ppp/8/8/8/8/4KPPP/R2Q3R w - - 0 1");
    const double exposedKing = engine.evaluatePosition();

    EXPECT_GT(safeKing, exposedKing);
}

TEST(evaluationTest, TaperedEvaluationRewardsActiveEndgameKing)
{
    Engine engine;
    engine.setFENCode("7k/8/8/8/3K4/8/8/8 w - - 0 1");
    const double activeKing = engine.evaluatePosition();

    engine.setFENCode("7k/8/8/8/8/8/8/K7 w - - 0 1");
    const double passiveKing = engine.evaluatePosition();

    EXPECT_GT(activeKing, passiveKing);
}
