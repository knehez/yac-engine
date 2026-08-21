#include "engine.h"
#include <thread>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <cctype>
#include "mcts.h"

namespace
{
std::string toUCIMove(Move move)
{
    if (move.start == NUMBER_OF_SQUARES || move.end == NUMBER_OF_SQUARES)
    {
        return "0000";
    }

    std::string result;
    result += static_cast<char>(algebraicFile(move.start));
    result += static_cast<char>(algebraicRank(move.start));
    result += static_cast<char>(algebraicFile(move.end));
    result += static_cast<char>(algebraicRank(move.end));
    if (move.promotion != NUMBER_OF_PIECES)
    {
        result += static_cast<char>(std::tolower(piecesChars[move.promotion]));
    }
    return result;
}
}

Engine::Engine() = default;

void Engine::setFENCode(const char *fen)
{
    board.setInitialState();
    board.setFENCode(fen);
}

void Engine::move(Move m)
{
    board.validateMove(&m); // TODO: invalid move?
    board.move(m);
}

void Engine::handleUCI()
{
    std::string strUCI;
    bool policyImportBatch = false;
    std::size_t policyImportAccepted = 0;

    while (getline(std::cin, strUCI))
    {
        if (strUCI == "uci")
        {
            std::cout << "id name YaC-Engine\n";
            std::cout << "id author Karoly Nehez\n";
            std::cout << "option name Seed type spin default 0 min 0 max 2147483647\n";
            std::cout << "option name Exploration type spin default 150 min 25 max 500\n";
            std::cout << "option name LearnedPolicy type check default false\n";
            std::cout << "option name PolicyLearning type check default false\n";
            std::cout << "option name PolicyWeight type spin default 50 min 0 max 100\n";
            std::cout << "option name MateProofTimeMs type spin default 2000 min 50 max 30000\n";
            std::cout << "option name TrainPolicy type button\n";
            std::cout << "option name ClearPolicy type button\n";
            std::cout << "uciok\n";
            continue;
        }
        if (strUCI == "isready")
        {
            std::cout << "readyok\n";
            continue;
        }
        if (strUCI == "ucinewgame")
        {
            board.setInitialState();
            board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            continue;
        }
        if (strUCI == "quit")
        {
            if (policyImportBatch)
            {
                policy.flushSamples();
            }
            break;
        }
        if (strUCI.rfind("setoption name Seed value ", 0) == 0)
        {
            const auto valueText = strUCI.substr(std::string("setoption name Seed value ").size());
            try
            {
                srand(static_cast<unsigned>(std::stoul(valueText)));
            }
            catch (const std::exception &)
            {
                std::cout << "info string invalid Seed value\n" << std::flush;
            }
            continue;
        }
        if (strUCI.rfind("setoption name Exploration value ", 0) == 0)
        {
            const auto valueText = strUCI.substr(std::string("setoption name Exploration value ").size());
            try
            {
                const int percentage = std::clamp(std::stoi(valueText), 25, 500);
                mctsExploration = percentage / 100.0;
            }
            catch (const std::exception &)
            {
                std::cout << "info string invalid Exploration value\n" << std::flush;
            }
            continue;
        }
        if (strUCI.rfind("setoption name LearnedPolicy value ", 0) == 0)
        {
            const auto value = strUCI.substr(std::string("setoption name LearnedPolicy value ").size());
            useLearnedPolicy = value == "true" || value == "1" || value == "on";
            continue;
        }
        if (strUCI.rfind("setoption name PolicyLearning value ", 0) == 0)
        {
            const auto value = strUCI.substr(std::string("setoption name PolicyLearning value ").size());
            policyLearning = value == "true" || value == "1" || value == "on";
            continue;
        }
        if (strUCI.rfind("setoption name PolicyWeight value ", 0) == 0)
        {
            const auto valueText = strUCI.substr(std::string("setoption name PolicyWeight value ").size());
            try
            {
                policyWeight = std::clamp(std::stoi(valueText), 0, 100) / 100.0;
            }
            catch (const std::exception &)
            {
                std::cout << "info string invalid PolicyWeight value\n" << std::flush;
            }
            continue;
        }
        if (strUCI.rfind("setoption name MateProofTimeMs value ", 0) == 0)
        {
            const auto valueText = strUCI.substr(std::string("setoption name MateProofTimeMs value ").size());
            try
            {
                mateProofTimeMs = std::clamp(std::stoi(valueText), 50, 30000);
            }
            catch (const std::exception &)
            {
                std::cout << "info string invalid MateProofTimeMs value\n" << std::flush;
            }
            continue;
        }
        if (strUCI == "setoption name TrainPolicy")
        {
            const bool trained = policy.trainNow();
            std::cout << "info string policy training " << (trained ? "complete" : "no-samples") << "\n" << std::flush;
            continue;
        }
        if (strUCI == "setoption name ClearPolicy")
        {
            policy.clear();
            std::cout << "info string policy cleared\n" << std::flush;
            continue;
        }
        if (strUCI == "policystatus")
        {
            std::cout << "policy enabled " << (useLearnedPolicy ? 1 : 0)
                      << " learning " << (policyLearning ? 1 : 0)
                      << " trained " << (policy.isTrained() ? 1 : 0)
                      << " samples " << policy.totalSampleCount()
                      << " updates " << policy.trainingUpdates()
                      << " lastsample " << (lastPolicySampleCollected ? 1 : 0)
                      << " lasttrained " << (lastPolicyTrained ? 1 : 0)
                      << "\n" << std::flush;
            continue;
        }
        if (strUCI == "policyimport begin")
        {
            policyImportBatch = true;
            policyImportAccepted = 0;
            std::cout << "policyimport ready\n" << std::flush;
            continue;
        }
        if (strUCI == "policyimport end")
        {
            const bool saved = policy.flushSamples();
            policyImportBatch = false;
            std::cout << "policyimport complete accepted " << policyImportAccepted
                      << " saved " << (saved ? 1 : 0) << "\n" << std::flush;
            continue;
        }
        if (strUCI.rfind("policylabel ", 0) == 0)
        {
            std::stringstream labelStream(strUCI);
            std::string command;
            std::string bestMove;
            double confidence = 1.0;
            labelStream >> command >> bestMove;
            if (!(labelStream >> confidence))
            {
                confidence = 1.0;
            }
            const bool accepted = collectExternalPolicySample(
                bestMove, confidence, !policyImportBatch);
            if (accepted)
            {
                ++policyImportAccepted;
            }
            std::cout << "policylabel " << (accepted ? "accepted" : "rejected")
                      << " samples " << policy.totalSampleCount() << "\n" << std::flush;
            continue;
        }
        if (strUCI == "fen")
        {
            std::cout << "fen " << board.getFENCode() << "\n" << std::flush;
            continue;
        }
        if (strUCI == "legalmoves")
        {
            Moves moves;
            moves = board.generateMoves(moves);
            std::cout << "legalmoves";
            for (int i = 0; i < moves.length; ++i)
            {
                std::cout << " " << toUCIMove(moves.move[i]);
            }
            std::cout << "\n" << std::flush;
            continue;
        }
        if (strUCI == "evaluate")
        {
            const int centipawns = static_cast<int>(std::round(evaluatePosition() * 1000.0));
            std::cout << "info score cp " << centipawns << "\n" << std::flush;
            continue;
        }
        if (strUCI.rfind("go", 0) == 0)
        {
            int iterations = 10000;
            std::stringstream goStream(strUCI);
            std::string token;
            while (goStream >> token)
            {
                if ((token == "nodes" || token == "iterations") && goStream >> iterations)
                {
                    break;
                }
            }
            iterations = std::max(1, iterations);
            auto bestMove = goMCTS(iterations);
            const int centipawns = static_cast<int>(std::round(lastMctsValue * 1000.0));
            for (size_t i = 0; i < lastMctsMoves.size(); ++i)
            {
                const auto &candidate = lastMctsMoves[i];
                const int candidateCp = static_cast<int>(std::round(candidate.value * 1000.0));
                const char *annotation = "";
                if (candidate.mateVerification == MateVerification::PROVEN)
                {
                    annotation = " string mate-proven";
                }
                else if (candidate.mateVerification == MateVerification::REFUTED)
                {
                    annotation = " string mate-refuted";
                }
                else if (candidate.mateVerification == MateVerification::UNKNOWN)
                {
                    annotation = " string mate-unknown";
                }
                else if (candidate.proven)
                {
                    annotation = " string proven";
                }
                std::cout << "info multipv " << (i + 1)
                          << " score cp " << candidateCp
                          << " nodes " << candidate.visits
                          << annotation
                          << " aiprior " << static_cast<int>(std::round(candidate.aiPrior * 10000.0))
                          << " pv " << toUCIMove(candidate.move) << "\n";
            }
            std::cout << "info score cp " << centipawns << " nodes " << lastMctsVisits
                      << " pv " << toUCIMove(bestMove) << "\n";
            std::cout << "bestmove " << toUCIMove(bestMove) << "\n" << std::flush;
            continue;
        }
        if (strUCI.rfind("position", 0) == 0)
        {
            std::string positionSpec = strUCI.substr(8);
            const auto firstContent = positionSpec.find_first_not_of(" \t\r\n");
            const auto lastContent = positionSpec.find_last_not_of(" \t\r\n");
            positionSpec = firstContent == std::string::npos
                               ? ""
                               : positionSpec.substr(firstContent, lastContent - firstContent + 1);
            const auto movesMarker = positionSpec.find(" moves ");
            const std::string positionPart = movesMarker == std::string::npos
                                                 ? positionSpec
                                                 : positionSpec.substr(0, movesMarker);
            const std::string movesPart = movesMarker == std::string::npos
                                              ? ""
                                              : positionSpec.substr(movesMarker + 7);

            board.setInitialState();
            if (positionPart == "startpos")
            {
                board.setFENCode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            }
            else if (positionPart.rfind("fen ", 0) == 0)
            {
                board.setFENCode(positionPart.substr(4).c_str());
            }

            if (!movesPart.empty())
            {
                std::stringstream ss(movesPart);
                std::string moveText;
                while (ss >> moveText)
                {
                    Move m;
                    parseMove(moveText, &m);
                    if (!board.validateMove(&m))
                    {
                        std::cout << "Invalid move: " << board.to_string(m) << "\n";
                        break;
                    }
                    board.move(m);
                }
            }
            continue;
        }
    }
}

void Engine::start()
{
    std::thread uci(&Engine::handleUCI, this);
    uci.join();
}

Move Engine::goMCTS(int maxIterations)
{
    lastMctsMoves.clear();
    lastPolicySampleCollected = false;
    lastPolicyTrained = false;
    Moves moves;
    moves = board.generateMoves(moves);
    enginePlayColor = board.getColor();

    // Checkmate and stalemate are terminal positions: neither has a move.
    if (moves.length == 0)
    {
        lastMctsValue = 0.0;
        lastMctsVisits = 0;
        return Move(NULL_MOVE);
    }

    // A zero-iteration search still has to return a valid legal move.
    if (maxIterations <= 0)
    {
        lastMctsValue = evaluatePosition();
        lastMctsVisits = 0;
        return moves.move[0];
    }

    auto root = std::unique_ptr<Node>(new Node(Move(NULL_MOVE), moves, nullptr, board.getColor()));
    const int rootPhase =
        countPiecesOnBoard(board.getBoard('N') | board.getBoard('n') |
                           board.getBoard('B') | board.getBoard('b')) +
        2 * countPiecesOnBoard(board.getBoard('R') | board.getBoard('r')) +
        4 * countPiecesOnBoard(board.getBoard('Q') | board.getBoard('q'));
    const int rolloutDepth = rootPhase >= 22 ? 0 : (rootPhase >= 14 ? 8 : 20);
    std::array<std::string, 4> policyCheckpoints{};
    int policyCheckpointCount = 0;

    for (int i = 0; i < maxIterations; i++)
    {
        auto node = root.get();
        int treeMovesMade = 0;
        // selection
        while (node->moves.length == 0 && node->has_children())
        {
            node = node->selectChildWithMaxUCT(mctsExploration);
            board.move(node->currentMove);
            treeMovesMade++;
        }

        // expand node with one random move
        if (node->moves.length > 0)
        {
            int tacticalScore = 0;
            auto randomMove = selectTacticalMove(node->moves, &tacticalScore);
            const double relativePolicyPrior = learnedRelativePrior(node, randomMove);
            board.move(randomMove);
            treeMovesMade++;
            moves = board.generateMoves(moves);
            node = node->addChild(randomMove, moves, board.getColor());
            node->policyPrior = relativePolicyPrior;
            // Quiet mating threats must retain enough search budget to be
            // revisited after an initially unlucky rollout. Tactical moves
            // still receive a larger prior, but the old 0.05 floor was too
            // small and effectively starved quiet root moves.
            const double tacticalPrior = std::clamp(tacticalScore / 10000.0, 0.35, 5.0);
            const double learnedPrior = 0.35 * relativePolicyPrior;
            node->prior = useLearnedPolicy && policy.isTrained()
                              ? (1.0 - policyWeight) * tacticalPrior + policyWeight * learnedPrior
                              : tacticalPrior;
            if (moves.length == 0)
            {
                node->solvedState = board.isInCheck() ? Node::SolvedState::LOSS : Node::SolvedState::DRAW;
            }
        }

        // The rollout result is from the current node's player-to-move
        // perspective. Non-terminal rollouts receive a static evaluation.
        // With a phase-aware leaf evaluator, long opening playouts add more
        // noise than useful information. Tactical low-material positions keep
        // the deeper rollout that is needed by the existing mate regressions.
        bool rolloutMateFound = false;
        double result = monteCarloSimulation(rolloutDepth, &rolloutMateFound);

        // Seed the implicit minimax channel with the leaf evaluation. It is
        // backed up separately from the Monte Carlo mean below.
        node->implicitMinimaxValue = result;
        node->hasImplicitMinimaxValue = true;

        // Remember which root move produced a sampled mate. A rollout mate is
        // only a candidate; it becomes proven only after every defensive
        // reply has been checked by verifyForcedMate below.
        Node *rootChild = node;
        while (rootChild != nullptr && rootChild->parent != root.get())
        {
            rootChild = rootChild->parent;
        }

        // Update values back to the root. Players alternate at every level,
        // so the result changes sign between parent and child.
        while (node != nullptr)
        {
            if (node == root.get() && rolloutMateFound && result > 0.0 && rootChild != nullptr)
            {
                rootChild->rolloutMateFound = true;
            }
            node->visits++;
            node->valueSum += result;
            node->updateSolvedState();
            node->updateImplicitMinimaxValue();
            result = -result;
            node = node->parent;
        }

        // Restore the exact root position independently of backpropagation.
        while (treeMovesMade-- > 0)
        {
            board.undoMove();
        }

        while (policyCheckpointCount < 4 &&
               static_cast<long long>(i + 1) * 4 >=
                   static_cast<long long>(policyCheckpointCount + 1) * maxIterations)
        {
            if (!root->children.empty())
            {
                const auto leader = *std::max_element(
                    root->children.begin(), root->children.end(), [](Node *a, Node *b) {
                        return a->visits < b->visits;
                    });
                policyCheckpoints[policyCheckpointCount] = ChessBoard::to_string(leader->currentMove);
            }
            ++policyCheckpointCount;
        }

        if (root->solvedState == Node::SolvedState::WIN)
        {
            break;
        }
    }

    // Verify only moves for which MCTS actually sampled a mating line. A
    // forcing pass searches checking continuations up to mate-in-six first;
    // the shorter unrestricted pass remains as a fallback for quiet mates.
    // Both share a bounded budget, so non-mating positions cannot explode.
    mateProofDeadline = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(mateProofTimeMs);
    mateProofDeadlineActive = true;
    int forcingMateBudget = 500000;
    struct MateCandidate
    {
        Node *node;
        bool givesCheck;
        int tacticalScore;
        double rootValue;
    };
    std::vector<MateCandidate> mateCandidates;
    for (auto child : root->children)
    {
        if (child->rolloutMateFound)
        {
            board.move(child->currentMove);
            const bool givesCheck = board.isInCheck();
            board.undoMove();
            const double rootValue = child->visits == 0 ? 0.0 : -child->valueSum / child->visits;
            mateCandidates.push_back({child, givesCheck, tacticalMoveScore(child->currentMove), rootValue});
        }
    }
    std::sort(mateCandidates.begin(), mateCandidates.end(), [](const MateCandidate &a, const MateCandidate &b) {
        if (a.givesCheck != b.givesCheck)
        {
            return a.givesCheck;
        }
        if (a.tacticalScore != b.tacticalScore)
        {
            return a.tacticalScore > b.tacticalScore;
        }
        if (a.rootValue != b.rootValue)
        {
            return a.rootValue > b.rootValue;
        }
        return a.node->visits > b.node->visits;
    });
    bool mateProven = false;
    for (const auto &candidate : mateCandidates)
    {
        Node *child = candidate.node;
        if (forcingMateBudget <= 0)
        {
            child->mateVerification = MateVerification::UNKNOWN;
            continue;
        }
        board.move(child->currentMove);
        child->mateVerification = verifyForcedMate(enginePlayColor, 10, forcingMateBudget, true);
        board.undoMove();
        if (child->mateVerification == MateVerification::PROVEN)
        {
            // The child is an opponent-to-move position in which that player
            // is proven to lose, matching the MCTS-Solver convention.
            child->solvedState = Node::SolvedState::LOSS;
            root->updateSolvedState();
            mateProven = true;
            break;
        }
    }

    if (!mateProven)
    {
        // A separate small unrestricted pass preserves quiet mating moves
        // such as e1e4; long forcing candidates cannot consume this budget.
        int generalMateBudget = 20000;
        std::sort(mateCandidates.begin(), mateCandidates.end(), [](const MateCandidate &a, const MateCandidate &b) {
            if (a.rootValue != b.rootValue)
            {
                return a.rootValue > b.rootValue;
            }
            return a.node->visits > b.node->visits;
        });
        for (const auto &candidate : mateCandidates)
        {
            Node *child = candidate.node;
            if (generalMateBudget <= 0)
            {
                if (child->mateVerification == MateVerification::NONE)
                {
                    child->mateVerification = MateVerification::UNKNOWN;
                }
                continue;
            }
            board.move(child->currentMove);
            child->mateVerification = verifyForcedMate(enginePlayColor, 6, generalMateBudget);
            board.undoMove();
            if (child->mateVerification == MateVerification::PROVEN)
            {
                child->solvedState = Node::SolvedState::LOSS;
                root->updateSolvedState();
                break;
            }
        }
    }
    mateProofDeadlineActive = false;

    if (policyLearning)
    {
        lastPolicySampleCollected = collectPolicySample(
            root.get(), maxIterations, policyCheckpoints, policyCheckpointCount);
        if (lastPolicySampleCollected)
        {
            // Training is deliberately delayed until enough independent
            // teacher searches have accumulated. The model was frozen for
            // the complete search that generated the newest sample.
            lastPolicyTrained = policy.trainIfReady();
        }
    }

    // The robust-child rule is less noisy than selecting by raw wins or by
    // the deepest descendant: play the root move visited most often.
    auto provenWin = std::find_if(root->children.begin(), root->children.end(), [](Node *child) {
        return child->solvedState == Node::SolvedState::LOSS;
    });
    lastMctsVisits = root->visits;
    lastMctsValue = root->visits == 0 ? 0.0 : root->valueSum / root->visits;
    const auto rootPolicy = rootPolicyDistribution(root.get());
    for (std::size_t childIndex = 0; childIndex < root->children.size(); ++childIndex)
    {
        auto child = root->children[childIndex];
        double value = child->visits == 0 ? 0.0 : -child->valueSum / child->visits;
        bool proven = false;
        if (child->solvedState == Node::SolvedState::LOSS)
        {
            value = 1.0;
            proven = true;
        }
        else if (child->solvedState == Node::SolvedState::WIN)
        {
            value = -1.0;
            proven = true;
        }
        else if (child->solvedState == Node::SolvedState::DRAW)
        {
            value = 0.0;
            proven = true;
        }
        const double aiPrior = childIndex < rootPolicy.size() ? rootPolicy[childIndex] : 0.0;
        lastMctsMoves.push_back(
            {child->currentMove, value, child->visits, proven, child->mateVerification, aiPrior});
    }
    // First form a reliable Multi-PV candidate set from the most explored
    // root moves. Otherwise a move with a single lucky rollout could appear
    // above thoroughly searched alternatives.
    std::sort(lastMctsMoves.begin(), lastMctsMoves.end(), [](const MctsMoveInfo &a, const MctsMoveInfo &b) {
        const bool aProvenWin = a.proven && a.value > 0.99;
        const bool bProvenWin = b.proven && b.value > 0.99;
        if (aProvenWin != bProvenWin)
        {
            return aProvenWin;
        }
        if (a.visits != b.visits)
        {
            return a.visits > b.visits;
        }
        return a.value > b.value;
    });
    if (lastMctsMoves.size() > 8)
    {
        lastMctsMoves.resize(8);
    }
    // The visible table is a score table, so rank the sufficiently explored
    // candidates by their displayed value. Visits break equal scores.
    std::sort(lastMctsMoves.begin(), lastMctsMoves.end(), [](const MctsMoveInfo &a, const MctsMoveInfo &b) {
        const bool aProvenWin = a.proven && a.value > 0.99;
        const bool bProvenWin = b.proven && b.value > 0.99;
        if (aProvenWin != bProvenWin)
        {
            return aProvenWin;
        }
        if (a.value != b.value)
        {
            return a.value > b.value;
        }
        return a.visits > b.visits;
    });
    if (provenWin != root->children.end())
    {
        lastMctsValue = 1.0;
        return (*provenWin)->currentMove;
    }

    auto best = *std::max_element(root->children.begin(), root->children.end(), [](Node *a, Node *b) {
        if (a->visits != b->visits)
        {
            return a->visits < b->visits;
        }
        const double aRootValue = a->visits == 0 ? 0.0 : -a->valueSum / a->visits;
        const double bRootValue = b->visits == 0 ? 0.0 : -b->valueSum / b->visits;
        return aRootValue < bRootValue;
    });

    return best->currentMove;
}

Move Engine::go(int searchDepth)
{
    Moves moves;
    std::vector<NodeStatistics> stats;

    monteCarloDepth = searchDepth;
    enginePlayColor = board.getColor();

    moves = board.generateMoves(moves);

    for (int i = 0; i < moves.length; i++)
    {
        stats.push_back(monteCarloSearch(board.getFENCode().c_str(), moves.move[i]));
    }
    std::cout << "\n";

    std::vector<float> results;
    std::vector<std::pair<float, int>> sortedResults;

    for (int i = 0; i < moves.length; i++)
    {
        float res = ((float)stats[i].win - (float)stats[i].lost) / (float)stats[i].trials;
        results.push_back(res);
        sortedResults.push_back(std::make_pair(results[i], i));
    }

    sort(sortedResults.begin(), sortedResults.end(), [](std::pair<float, int> a, std::pair<float, int> b) {
        return a.first > b.first;
    });

    for (int i = 0; i < sortedResults.size(); i++)
    {
        int index = sortedResults[i].second;
        std::cout << board.to_string(moves.move[index]) << " wins:" << stats[index].win
                  << " losts:" << stats[index].lost
                  << " trials:" << stats[index].trials << " -> " << results[index] << "\n";
    }

    board.move(moves.move[sortedResults[0].second]);
    return moves.move[sortedResults[0].second];
}

NodeStatistics Engine::monteCarloSearch(const char *fen, Move startingMove)
{
    NodeStatistics stat;

    board.move(startingMove);

    for (int i = 0; i < 1000; i++, stat.trials++)
    {
        // After startingMove the opponent is to move, so negate the result
        // to express it from the engine's point of view.
        const double value = -monteCarloSimulation(monteCarloDepth);
        if (value > 0.0)
        {
            stat.win += static_cast<float>(value);
        }
        else
        {
            stat.lost += static_cast<float>(-value);
        }
    }

    board.undoMove();

    return stat;
}

double Engine::evaluatePosition()
{
    // A cheap tapered evaluation: all terms below are computed directly from
    // the twelve piece bitboards. No legal move generation is performed here,
    // because this function runs at the end of every MCTS rollout.
    constexpr int maxPhase = 24;
    constexpr int middleGameValue[6] = {100, 320, 330, 500, 900, 0};
    constexpr int endGameValue[6] = {120, 300, 320, 520, 900, 0};
    constexpr int phaseValue[6] = {0, 1, 1, 2, 4, 0};
    constexpr char whitePieces[6] = {'P', 'N', 'B', 'R', 'Q', 'K'};
    constexpr char blackPieces[6] = {'p', 'n', 'b', 'r', 'q', 'k'};

    const uint64_t whitePawns = board.getBoard('P');
    const uint64_t blackPawns = board.getBoard('p');
    int phase = 0;
    int whiteMiddleGame = 0;
    int whiteEndGame = 0;
    int blackMiddleGame = 0;
    int blackEndGame = 0;

    auto evaluateSide = [&](Color color, int &middleGame, int &endGame) {
        const char *pieces = color == WHITE ? whitePieces : blackPieces;
        const uint64_t ownPawns = color == WHITE ? whitePawns : blackPawns;
        const uint64_t enemyPawns = color == WHITE ? blackPawns : whitePawns;

        for (int pieceType = 0; pieceType < 6; ++pieceType)
        {
            uint64_t piecesBoard = board.getBoard(pieces[pieceType]);
            const int pieceCount = countPiecesOnBoard(piecesBoard);
            middleGame += pieceCount * middleGameValue[pieceType];
            endGame += pieceCount * endGameValue[pieceType];
            phase += pieceCount * phaseValue[pieceType];

            while (piecesBoard != 0)
            {
                const Position square = bitScan64(piecesBoard);
                piecesBoard &= piecesBoard - 1;
                const int file = square & 7;
                const int absoluteRank = square >> 3;
                const int relativeRank = color == WHITE ? absoluteRank : 7 - absoluteRank;
                const int centerDistance = std::abs(2 * file - 7) + std::abs(2 * absoluteRank - 7);
                const int centerBonus = 14 - centerDistance;

                switch (pieceType)
                {
                case 0: // Pawns: central and advanced pawns become more valuable.
                {
                    middleGame += 2 * centerBonus + 3 * relativeRank;
                    endGame += centerBonus + 4 * relativeRank * relativeRank;

                    uint64_t relevantFiles = FILES[file];
                    if (file > 0)
                        relevantFiles |= FILES[file - 1];
                    if (file < 7)
                        relevantFiles |= FILES[file + 1];
                    uint64_t squaresAhead = 0;
                    for (int targetRank = relativeRank + 1; targetRank < 8; ++targetRank)
                    {
                        const int boardRank = color == WHITE ? targetRank : 7 - targetRank;
                        squaresAhead |= 0xffULL << (8 * boardRank);
                    }
                    if ((enemyPawns & relevantFiles & squaresAhead) == 0)
                    {
                        middleGame += 4 * relativeRank;
                        endGame += 6 * relativeRank * relativeRank;
                    }
                    break;
                }
                case 1: // Knights need central squares and room to develop.
                    middleGame += 4 * centerBonus - (relativeRank == 0 ? 8 : 0);
                    endGame += 3 * centerBonus;
                    break;
                case 2: // Bishops retain value in both phases.
                    middleGame += 2 * centerBonus + (relativeRank > 0 ? 4 : 0);
                    endGame += 2 * centerBonus;
                    break;
                case 3: // Rooks prefer open files and the seventh rank.
                    middleGame += relativeRank == 6 ? 15 : 0;
                    endGame += relativeRank == 6 ? 25 : 0;
                    if ((ownPawns & FILES[file]) == 0)
                    {
                        middleGame += 8;
                        endGame += 6;
                        if ((enemyPawns & FILES[file]) == 0)
                        {
                            middleGame += 8;
                            endGame += 6;
                        }
                    }
                    break;
                case 4: // Avoid over-rewarding early queen centralization.
                    middleGame += centerBonus / 2;
                    endGame += centerBonus;
                    break;
                case 5: // Safe in the middlegame, active in the endgame.
                {
                    middleGame -= 4 * centerBonus;
                    endGame += 4 * centerBonus;
                    if (relativeRank == 0 && (file == 2 || file == 6))
                        middleGame += 24;
                    else if (relativeRank == 0 && file == 4)
                        middleGame += 8;

                    int shield = 0;
                    for (int fileOffset = -1; fileOffset <= 1; ++fileOffset)
                    {
                        const int shieldFile = file + fileOffset;
                        const int shieldRank = absoluteRank + (color == WHITE ? 1 : -1);
                        if (shieldFile >= 0 && shieldFile < 8 && shieldRank >= 0 && shieldRank < 8 &&
                            (ownPawns & (1ULL << (8 * shieldRank + shieldFile))) != 0)
                        {
                            shield++;
                        }
                    }
                    middleGame += 14 * shield;
                    break;
                }
                }
            }
        }

        // Pawn structure is phase-sensitive but independent of move history.
        for (int file = 0; file < 8; ++file)
        {
            const int pawnsOnFile = countPiecesOnBoard(ownPawns & FILES[file]);
            if (pawnsOnFile > 1)
            {
                middleGame -= 12 * (pawnsOnFile - 1);
                endGame -= 18 * (pawnsOnFile - 1);
            }
            if (pawnsOnFile > 0)
            {
                const bool hasLeftNeighbour = file > 0 && (ownPawns & FILES[file - 1]) != 0;
                const bool hasRightNeighbour = file < 7 && (ownPawns & FILES[file + 1]) != 0;
                if (!hasLeftNeighbour && !hasRightNeighbour)
                {
                    middleGame -= 10 * pawnsOnFile;
                    endGame -= 12 * pawnsOnFile;
                }
            }
        }

        if (countPiecesOnBoard(board.getBoard(pieces[2])) >= 2)
        {
            middleGame += 25;
            endGame += 40;
        }
    };

    evaluateSide(WHITE, whiteMiddleGame, whiteEndGame);
    evaluateSide(BLACK, blackMiddleGame, blackEndGame);

    phase = std::clamp(phase, 0, maxPhase);
    const int middleGameScore = whiteMiddleGame - blackMiddleGame;
    const int endGameScore = whiteEndGame - blackEndGame;
    const int scoreForWhite =
        (middleGameScore * phase + endGameScore * (maxPhase - phase)) / maxPhase;
    const double scoreForPlayerToMove = board.getColor() == WHITE ? scoreForWhite : -scoreForWhite;
    return std::tanh(scoreForPlayerToMove / 1200.0);
}

PolicyFeatures Engine::policyFeatures(Move move)
{
    PolicyFeatures features{};
    const Piece movingPiece = board.getPieceAt(move.start);
    const int pieceType = movingPiece == NUMBER_OF_PIECES ? 0 : int(movingPiece) / 2;
    if (pieceType >= 0 && pieceType < 6)
    {
        features[pieceType] = 1.0;
    }

    static constexpr double pieceValueByType[6] = {100.0, 500.0, 320.0, 330.0, 900.0, 0.0};
    const auto centrality = [](Position position) {
        const int file = int(position) & 7;
        const int boardRank = int(position) >> 3;
        return (7.0 - std::abs(2.0 * file - 7.0) - std::abs(2.0 * boardRank - 7.0)) / 7.0;
    };

    const int startRank = int(move.start) >> 3;
    const int endRank = int(move.end) >> 3;
    const Color mover = board.getColor();
    const double startCentrality = centrality(move.start);
    const double endCentrality = centrality(move.end);
    features[6] = move.captured == NUMBER_OF_PIECES
                      ? 0.0
                      : pieceValueByType[int(move.captured) / 2] / 900.0;
    features[7] = move.promotion == NUMBER_OF_PIECES
                      ? 0.0
                      : pieceValueByType[int(move.promotion) / 2] / 900.0;
    features[8] = startCentrality;
    features[9] = endCentrality;
    features[10] = (endCentrality - startCentrality) / 2.0;
    features[11] = ((mover == WHITE ? 1.0 : -1.0) * (endRank - startRank)) / 7.0;
    const bool minorPiece = pieceType == 2 || pieceType == 3;
    const bool startsAtHome = mover == WHITE ? startRank == 0 : startRank == 7;
    const bool leavesHome = mover == WHITE ? endRank > 0 : endRank < 7;
    features[12] = minorPiece && startsAtHome && leavesHome ? 1.0 : 0.0;
    features[13] = pieceType == 5 ? 1.0 : 0.0;
    features[14] = pieceType == 4 ? 1.0 : 0.0;
    features[15] = pieceType == 5 && std::abs((int(move.end) & 7) - (int(move.start) & 7)) == 2
                       ? 1.0
                       : 0.0;
    const int phase =
        countPiecesOnBoard(board.getBoard('N') | board.getBoard('n') |
                           board.getBoard('B') | board.getBoard('b')) +
        2 * countPiecesOnBoard(board.getBoard('R') | board.getBoard('r')) +
        4 * countPiecesOnBoard(board.getBoard('Q') | board.getBoard('q'));
    features[16] = std::clamp(phase / 24.0, 0.0, 1.0);
    features[17] = mover == WHITE ? endRank / 7.0 : (7 - endRank) / 7.0;
    return features;
}

double Engine::learnedRelativePrior(const Node *node, Move selectedMove)
{
    if (!useLearnedPolicy || !policy.isTrained())
    {
        return 1.0;
    }

    std::vector<Move> legalMoves;
    legalMoves.reserve(node->children.size() + node->moves.length);
    for (const auto child : node->children)
    {
        legalMoves.push_back(child->currentMove);
    }
    for (int index = 0; index < node->moves.length; ++index)
    {
        legalMoves.push_back(node->moves.move[index]);
    }

    std::vector<PolicyFeatures> featureVectors;
    featureVectors.reserve(legalMoves.size());
    std::size_t selectedIndex = 0;
    for (std::size_t index = 0; index < legalMoves.size(); ++index)
    {
        featureVectors.push_back(policyFeatures(legalMoves[index]));
        if (legalMoves[index] == selectedMove)
        {
            selectedIndex = index;
        }
    }
    const auto probabilities = policy.distribution(featureVectors);
    if (probabilities.empty())
    {
        return 1.0;
    }
    return std::clamp(probabilities[selectedIndex] * probabilities.size(), 0.25, 4.0);
}

std::vector<double> Engine::rootPolicyDistribution(const Node *root)
{
    std::vector<PolicyFeatures> featureVectors;
    featureVectors.reserve(root->children.size());
    for (const auto child : root->children)
    {
        featureVectors.push_back(policyFeatures(child->currentMove));
    }
    if (!policy.isTrained())
    {
        return std::vector<double>(
            featureVectors.size(), featureVectors.empty() ? 0.0 : 1.0 / featureVectors.size());
    }
    return policy.distribution(featureVectors);
}

bool Engine::collectPolicySample(
    const Node *root,
    int maxIterations,
    const std::array<std::string, 4> &checkpoints,
    int checkpointCount)
{
    if (root->children.empty())
    {
        return false;
    }

    auto provenWin = std::find_if(root->children.begin(), root->children.end(), [](const Node *child) {
        return child->solvedState == Node::SolvedState::LOSS;
    });
    const bool exactTarget = provenWin != root->children.end();
    if (!exactTarget)
    {
        if (maxIterations < 100000 || checkpointCount < 4 || checkpoints[3].empty())
        {
            return false;
        }
        // Reject unstable teachers. Agreement over the last three quarters
        // is required; agreement from the first quarter receives more weight.
        if (checkpoints[1] != checkpoints[2] || checkpoints[2] != checkpoints[3])
        {
            return false;
        }
    }

    PolicyTrainingSample sample;
    sample.moves.reserve(root->children.size());
    sample.target.assign(root->children.size(), 0.0);
    for (const auto child : root->children)
    {
        sample.moves.push_back(policyFeatures(child->currentMove));
    }

    if (exactTarget)
    {
        const std::size_t index = std::distance(root->children.begin(), provenWin);
        sample.target[index] = 1.0;
        sample.confidence = 1.0;
    }
    else
    {
        double total = 0.0;
        for (std::size_t index = 0; index < root->children.size(); ++index)
        {
            // A slightly softened visit distribution avoids declaring every
            // non-leading legal move categorically wrong.
            sample.target[index] = std::pow(std::max(1, root->children[index]->visits), 0.8);
            total += sample.target[index];
        }
        for (double &target : sample.target)
        {
            target /= total;
        }
        sample.confidence = checkpoints[0] == checkpoints[3] ? 0.8 : 0.65;
        if (useLearnedPolicy)
        {
            // A teacher influenced by the current student is still useful,
            // but receives less weight to reduce self-reinforcing mistakes.
            sample.confidence *= 0.75;
        }
    }

    policy.addSample(std::move(sample));
    return true;
}

bool Engine::collectExternalPolicySample(
    const std::string &bestMove,
    double confidence,
    bool persist)
{
    Moves legalMoves;
    legalMoves = board.generateMoves(legalMoves);
    if (legalMoves.length == 0)
    {
        return false;
    }

    PolicyTrainingSample sample;
    sample.moves.reserve(static_cast<std::size_t>(legalMoves.length));
    sample.target.assign(static_cast<std::size_t>(legalMoves.length), 0.0);
    sample.confidence = std::clamp(confidence, 0.0, 1.0);

    int targetIndex = -1;
    for (int index = 0; index < legalMoves.length; ++index)
    {
        const Move candidate = legalMoves.move[index];
        sample.moves.push_back(policyFeatures(candidate));
        if (toUCIMove(candidate) == bestMove)
        {
            targetIndex = index;
        }
    }
    if (targetIndex < 0)
    {
        return false;
    }

    sample.target[static_cast<std::size_t>(targetIndex)] = 1.0;
    policy.addSample(std::move(sample), persist);
    return true;
}

int Engine::tacticalMoveScore(Move move)
{
    static const int pieceValues[NUMBER_OF_PIECES] = {
        100, 100, 500, 500, 320, 320, 330, 330, 900, 900, 0, 0};

    const Piece movingPiece = board.getPieceAt(move.start);
    int score = rand() % 16; // Preserve variety between equally tactical moves.

    if (move.captured != NUMBER_OF_PIECES)
    {
        score += 10 * pieceValues[move.captured] - pieceValues[movingPiece];
    }
    if (move.promotion != NUMBER_OF_PIECES)
    {
        score += pieceValues[move.promotion] - 100 + 600;
    }

    board.move(move);
    const bool givesCheck = board.isInCheck();
    if (givesCheck)
    {
        Moves replies;
        replies = board.generateMoves(replies);
        if (replies.length == 0)
        {
            score = 1000000;
        }
        else
        {
            // For checks, forcing power dominates short-term material gain.
            // Random tie-breaking lets rollouts sample alternative forcing
            // continuations instead of always taking a greedy promotion.
            score = 50000 - 2000 * replies.length + rand() % 2000;
        }
    }
    board.undoMove();

    return score;
}

Move Engine::selectTacticalMove(const Moves &moves, int *selectedScore)
{
    int bestIndex = 0;
    int bestScore = std::numeric_limits<int>::min();

    for (int i = 0; i < moves.length; ++i)
    {
        const int score = tacticalMoveScore(moves.move[i]);
        if (score > bestScore)
        {
            bestScore = score;
            bestIndex = i;
        }
    }

    if (selectedScore != nullptr)
    {
        *selectedScore = bestScore;
    }
    return moves.move[bestIndex];
}

MateVerification Engine::verifyForcedMate(
    Color attacker,
    int pliesRemaining,
    int &nodeBudget,
    bool checkingMovesOnly)
{
    if (mateProofDeadlineActive && std::chrono::steady_clock::now() >= mateProofDeadline)
    {
        return MateVerification::UNKNOWN;
    }
    if (nodeBudget-- <= 0)
    {
        return MateVerification::UNKNOWN;
    }

    Moves moves;
    moves = board.generateMoves(moves);
    if (moves.length == 0)
    {
        const bool checkmate = board.isInCheck();
        const bool attackerDeliveredMate = checkmate && board.getColor() != attacker;
        return attackerDeliveredMate ? MateVerification::PROVEN : MateVerification::REFUTED;
    }
    if (pliesRemaining <= 0)
    {
        return checkingMovesOnly ? MateVerification::UNKNOWN : MateVerification::REFUTED;
    }

    struct ProofMove
    {
        int score;
        Move move;
    };

    const bool attackerToMove = board.getColor() == attacker;
    std::vector<ProofMove> orderedMoves;
    orderedMoves.reserve(moves.length);
    for (int i = 0; i < moves.length; ++i)
    {
        if (checkingMovesOnly && attackerToMove)
        {
            board.move(moves.move[i]);
            const bool givesCheck = board.isInCheck();
            board.undoMove();
            if (!givesCheck)
            {
                continue;
            }
        }
        orderedMoves.push_back({tacticalMoveScore(moves.move[i]), moves.move[i]});
    }
    std::sort(orderedMoves.begin(), orderedMoves.end(), [](const auto &a, const auto &b) {
        return a.score > b.score;
    });

    if (orderedMoves.empty())
    {
        return checkingMovesOnly ? MateVerification::UNKNOWN : MateVerification::REFUTED;
    }

    bool sawUnknown = false;
    for (const auto &scoredMove : orderedMoves)
    {
        board.move(scoredMove.move);
        const MateVerification childResult =
            verifyForcedMate(attacker, pliesRemaining - 1, nodeBudget, checkingMovesOnly);
        board.undoMove();

        if (attackerToMove && childResult == MateVerification::PROVEN)
        {
            return MateVerification::PROVEN; // One mating continuation is enough.
        }
        if (!attackerToMove && childResult == MateVerification::REFUTED)
        {
            return MateVerification::REFUTED; // The defender found an escape.
        }
        sawUnknown |= childResult == MateVerification::UNKNOWN;
        if (nodeBudget <= 0)
        {
            sawUnknown = true;
            break;
        }
    }

    if (sawUnknown)
    {
        return MateVerification::UNKNOWN;
    }
    if (attackerToMove && checkingMovesOnly)
    {
        // Failure to prove a checking continuation does not refute a quiet
        // mating move, so the forcing-only pass may only return UNKNOWN here.
        return MateVerification::UNKNOWN;
    }
    return attackerToMove ? MateVerification::REFUTED : MateVerification::PROVEN;
}

double Engine::monteCarloSimulation(int depth, bool *mateFound)
{
    int movesMade = 0;
    double result = 0.0;
    if (mateFound != nullptr)
    {
        *mateFound = false;
    }

    for (int ply = 0; ply < depth; ++ply)
    {
        Moves moves;
        moves = board.generateMoves(moves);

        if (moves.length == 0)
        {
            // A player with no legal move loses only when actually in check.
            // Stalemate is a draw.
            const bool checkmate = board.isMate();
            if (mateFound != nullptr)
            {
                *mateFound = checkmate;
            }
            result = checkmate ? -(1.0 - 0.001 * movesMade) : 0.0;
            if (movesMade % 2 != 0)
            {
                result = -result;
            }
            while (movesMade-- > 0)
            {
                board.undoMove();
            }
            return result;
        }

        board.move(selectTacticalMove(moves));
        movesMade++;
    }

    // Convert the leaf evaluation back to the rollout's starting player.
    result = evaluatePosition();
    if (movesMade % 2 != 0)
    {
        result = -result;
    }

    while (movesMade-- > 0)
    {
        board.undoMove();
    }
    return result;
}

uint64_t Engine::perft(int depth)
{
    uint64_t count = 0;
    Moves moves;

    moves = board.generateMoves(moves);
    if (depth == 1)
    {
        /*
        for (int i = 0; i < moves.length; i++)
        {
            for (auto m : board.matchMoves)
            {
                board.fenFile << board.to_string(m) << '|';
            }
            board.fenFile << board.to_string(moves.move[i]) << "\n";
        }
        */
        return moves.length;
    }
    for (int i = 0; i < moves.length; i++)
    {
        board.move(moves.move[i]);
        board.matchMoves.push_back(moves.move[i]);
        count += perft(depth - 1);
        board.matchMoves.pop_back();
        board.undoMove();
    }
    return count;
}

void Engine::parseMove(std::string move, Move *m)
{
    m->start = (Position)(((move[1] - '1') << 3) + (move[0] - 'a'));
    m->end = (Position)(((move[3] - '1') << 3) + (move[2] - 'a'));
    // with promotion?
    if (move.length() == 5)
    {
        Color color = WHITE;
        if (move[3] == '1')
        {
            color = BLACK;
        }
        if (move[4] == 'r')
        {
            m->promotion = color == WHITE ? R : r;
        }
        if (move[4] == 'n')
        {
            m->promotion = color == WHITE ? N : n;
        }
        if (move[4] == 'b')
        {
            m->promotion = color == WHITE ? B : b;
        }
        if (move[4] == 'q')
        {
            m->promotion = color == WHITE ? Q : q;
        }
    }
}
