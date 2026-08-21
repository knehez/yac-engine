#pragma once

#include "engine.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>

class Node
{
public:
    enum class SolvedState
    {
        UNKNOWN,
        WIN,
        LOSS,
        DRAW
    };

    Node(Move move, Moves moves, Node *parent, Color playerColor);
    ~Node();
    Node *addChild(Move move, Moves moves, Color playerColor);
    bool has_children() const { return !children.empty(); }
    Node *selectChildWithMaxUCT(double explorationConstant = 1.5) const;
    void updateSolvedState();
    void updateImplicitMinimaxValue();
    Move getRandomMove();

    Node *const parent;
    std::vector<Node *> children;

    Moves moves;
    const Move currentMove;
    // Sum of simulation results from playerToMove's point of view.
    // A positive value is good for the player who is to move in this node.
    double valueSum;
    int visits;
    // Heuristic minimax value from playerToMove's point of view. Unlike the
    // Monte Carlo mean, this preserves the strongest reply found below the
    // node and prevents many mediocre samples from hiding a refutation.
    double implicitMinimaxValue;
    bool hasImplicitMinimaxValue;
    Color playerToMove;
    double utcScore;
    double prior;
    double policyPrior;
    SolvedState solvedState;
    bool rolloutMateFound;
    MateVerification mateVerification;
    std::string tree_to_string(int max_depth = 1000000, int indent = 0);
    std::string tree_to_dot(int max_depth = 1000000, int indent = 0);

private:
    Node(const Node &);
    Node &operator=(const Node &);
    std::string indent_string(int indent);
    std::string to_string();
    std::string to_string_node();
};

inline Node::Node(Move move, Moves moves, Node *parent, Color playerColor)
    : parent(parent),
      moves(moves),
      currentMove(move),
      valueSum(0),
      visits(0),
      implicitMinimaxValue(0),
      hasImplicitMinimaxValue(false),
      playerToMove(playerColor),
      utcScore(0),
      prior(1.0),
      policyPrior(0.0),
      solvedState(SolvedState::UNKNOWN),
      rolloutMateFound(false),
      mateVerification(MateVerification::NONE)
{
}

inline Node::~Node()
{
    for (auto child : children)
    {
        delete child;
    }
}

inline Move Node::getRandomMove()
{
    int randomIndex = rand() % moves.length;
    return moves.move[randomIndex];
}

inline Node *Node::addChild(Move move, Moves newMoves, Color playerColor)
{
    auto node = new Node(move, newMoves, this, playerColor);
    children.push_back(node);

    removeOneMove(moves, move);

    return node;
}

inline Node *Node::selectChildWithMaxUCT(double explorationConstant) const
{
    for (auto child : children)
    {
        if (child->solvedState == SolvedState::LOSS)
        {
            child->utcScore = std::numeric_limits<double>::infinity();
            continue;
        }
        if (child->solvedState == SolvedState::WIN)
        {
            child->utcScore = -std::numeric_limits<double>::infinity();
            continue;
        }
        if (child->solvedState == SolvedState::DRAW)
        {
            child->utcScore = 0.0;
            continue;
        }
        if (child->visits == 0)
        {
            child->utcScore = std::numeric_limits<double>::infinity();
            continue;
        }

        // A child's value is stored from the opponent's point of view, hence
        // the minus sign when its parent chooses a move.
        const double meanValue = -(child->valueSum / double(child->visits));
        // Implicit minimax backup: retain most of MCTS's robust sampled mean,
        // but also respect the best reply already found in the subtree. The
        // modest weight limits horizon errors in the static evaluator.
        constexpr double implicitMinimaxWeight = 0.50;
        const double minimaxValue = child->hasImplicitMinimaxValue
                                        ? -child->implicitMinimaxValue
                                        : meanValue;
        const double exploitation =
            (1.0 - implicitMinimaxWeight) * meanValue +
            implicitMinimaxWeight * minimaxValue;
        const double exploration = explorationConstant * child->prior *
                                   std::sqrt(double(std::max(1, this->visits))) /
                                   double(1 + child->visits);
        child->utcScore = exploitation + exploration;
    }

    return *std::max_element(children.begin(), children.end(), [](Node *a, Node *b) {
        return a->utcScore < b->utcScore;
    });
}

inline std::string Node::to_string()
{
    std::stringstream sout;
    auto curMove = ChessBoard::to_string(currentMove);

    sout << "["
         << (parent != nullptr ? ChessBoard::to_string(currentMove) : "") << " "
         << "Q/V: " << valueSum << "/" << visits << " "
         << "UTC: " << utcScore << "]\n";
    return sout.str();
}

inline std::string Node::to_string_node()
{
    std::stringstream sout;
    auto curMove = ChessBoard::to_string(currentMove);

    sout << (parent != nullptr ? ChessBoard::to_string(currentMove) : "") << "\\n"
         << "Q/V: " << valueSum << "/" << visits << "\\n"
         << "UTC: " << utcScore << "\\n";
    return sout.str();
}

inline std::vector<std::string> matchMoves;

// https://dreampuf.github.io/GraphvizOnline/
inline std::string Node::tree_to_dot(int max_depth, int indent)
{
    std::string s;

    sort(children.begin(), children.end(), [](Node *a, Node *b) {
        return a->visits > b->visits;
    });

    if (indent >= max_depth)
    {
        return "";
    }

    matchMoves.push_back(ChessBoard::to_string(currentMove));

    std::string parent;
    for (const auto piece : matchMoves)
    {
        parent += piece;
    }

    s = parent + "[label=\"" + to_string_node() + "\"" + (children.size() == 0 ? " color=\"red\"" : "") + "]\n";

    int index = 0;
    for (auto child : children)
    {
        if (index++ > 2)
        {
            //break;
        }
        s += child->tree_to_dot(max_depth, indent + 1);
        s += parent + " -> " + parent + ChessBoard::to_string(child->currentMove) + "[label = \"" + ChessBoard::to_string(child->currentMove) + "\"]" + "\n";
    }

    matchMoves.pop_back();

    return s;
}

inline std::string Node::tree_to_string(int max_depth, int indent)
{
    if (indent >= max_depth)
    {
        return "";
    }

    std::string s = indent_string(indent) + to_string();
    for (auto child : children)
    {
        s += child->tree_to_string(max_depth, indent + 1);
    }
    return s;
}

inline std::string Node::indent_string(int indent)
{
    std::string s = "";
    for (int i = 1; i <= indent; ++i)
    {
        s += "| ";
    }
    return s;
}

inline void Node::updateSolvedState()
{
    for (auto child : children)
    {
        if (child->solvedState == SolvedState::LOSS)
        {
            solvedState = SolvedState::WIN;
            return;
        }
    }

    // Unexpanded moves may still change the proof status.
    if (moves.length > 0)
    {
        solvedState = SolvedState::UNKNOWN;
        return;
    }

    // A terminal leaf is initialized by Engine when it is created.
    if (children.empty())
    {
        return;
    }

    bool allChildrenSolved = true;
    bool allChildrenWin = true;
    for (auto child : children)
    {
        allChildrenSolved &= child->solvedState != SolvedState::UNKNOWN;
        allChildrenWin &= child->solvedState == SolvedState::WIN;
    }

    if (allChildrenWin)
    {
        solvedState = SolvedState::LOSS;
    }
    else if (allChildrenSolved)
    {
        solvedState = SolvedState::DRAW;
    }
    else
    {
        solvedState = SolvedState::UNKNOWN;
    }
}

inline void Node::updateImplicitMinimaxValue()
{
    // Proven solver results are exact and take precedence over heuristics.
    if (solvedState == SolvedState::WIN)
    {
        implicitMinimaxValue = 1.0;
        hasImplicitMinimaxValue = true;
        return;
    }
    if (solvedState == SolvedState::LOSS)
    {
        implicitMinimaxValue = -1.0;
        hasImplicitMinimaxValue = true;
        return;
    }
    if (solvedState == SolvedState::DRAW)
    {
        implicitMinimaxValue = 0.0;
        hasImplicitMinimaxValue = true;
        return;
    }

    double bestValue = -std::numeric_limits<double>::infinity();
    bool foundEvaluatedChild = false;
    for (auto child : children)
    {
        if (!child->hasImplicitMinimaxValue)
        {
            continue;
        }
        bestValue = std::max(bestValue, -child->implicitMinimaxValue);
        foundEvaluatedChild = true;
    }

    // A frontier leaf keeps the static/rollout value assigned by Engine.
    if (foundEvaluatedChild)
    {
        implicitMinimaxValue = bestValue;
        hasImplicitMinimaxValue = true;
    }
}
