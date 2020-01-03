#pragma once

#include "engine.h"
#include <vector>
#include <algorithm>
#include <cmath>

class Node
{
public:
    Node(Move move, Moves moves, Node *parent, Color playerColor);
    ~Node();
    Node *addChild(Move move, Moves moves, Color playerColor);
    bool has_children() const { return !children.empty(); }
    Node *selectChildWithMaxUCT() const;
    Move getRandomMove();

    Node *const parent;
    std::vector<Node *> children;

    Moves moves;
    const Move currentMove;
    double wins;
    int visits;
    Color playerToMove;
    double utcScore;
    std::string tree_to_string(int max_depth = 1000000, int indent = 0);
    std::string tree_to_dot(int max_depth = 1000000, int indent = 0);

private:
    Node(const Node &);
    Node &operator=(const Node &);
    std::string indent_string(int indent);
    std::string to_string();
    std::string to_string_node();
};

Node::Node(Move move, Moves moves, Node *parent, Color playerColor)
    : currentMove(move),
      parent(parent),
      playerToMove(playerColor),
      wins(0),
      visits(0),
      moves(moves),
      utcScore(0) {}

Node::~Node()
{
    for (auto child : children)
    {
        delete child;
    }
}

Move Node::getRandomMove()
{
    int randomIndex = rand() % moves.length;
    return moves.move[randomIndex];
}

Node *Node::addChild(Move move, Moves newMoves, Color playerColor)
{
    auto node = new Node(move, newMoves, this, playerColor);
    children.push_back(node);

    removeOneMove(moves, move);

    return node;
}

Node *Node::selectChildWithMaxUCT() const
{
    for (auto child : children)
    {
        child->utcScore = (double(child->wins) / double(child->visits)) + std::sqrt(2.0 * std::log(double(this->visits)) / child->visits);
    }

    return *std::max_element(children.begin(), children.end(), [](Node *a, Node *b) {
        return a->utcScore < b->utcScore;
    });
}

std::string Node::to_string()
{
    std::stringstream sout;
    auto curMove = ChessBoard::to_string(currentMove);

    sout << "["
         << (parent != nullptr ? ChessBoard::to_string(currentMove) : "") << " "
         << "W/V: " << wins << "/" << visits << " "
         << "UTC: " << utcScore << "]\n";
    return sout.str();
}

std::string Node::to_string_node()
{
    std::stringstream sout;
    auto curMove = ChessBoard::to_string(currentMove);

    sout << (parent != nullptr ? ChessBoard::to_string(currentMove) : "") << "\\n"
         << "W/V: " << wins << "/" << visits << "\\n"
         << "UTC: " << utcScore << "\\n"
         << "log: " << (parent != nullptr ? std::to_string(std::sqrt(2.0 * std::log(double(parent->visits)) / visits)) : "");
    return sout.str();
}

std::vector<std::string> matchMoves;

// https://dreampuf.github.io/GraphvizOnline/
std::string Node::tree_to_dot(int max_depth, int indent)
{
    std::string s;

    sort(children.begin(), children.end(), [](Node *a, Node *b) {
        return a->wins > b->wins;
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
        if (index++ > 4)
        {
            break;
        }
        s += child->tree_to_dot(max_depth, indent + 1);
        s += parent + " -> " + parent + ChessBoard::to_string(child->currentMove) + "\n";
    }

    matchMoves.pop_back();

    return s;
}

std::string Node::tree_to_string(int max_depth, int indent)
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

std::string Node::indent_string(int indent)
{
    std::string s = "";
    for (int i = 1; i <= indent; ++i)
    {
        s += "| ";
    }
    return s;
}