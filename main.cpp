#include <iostream>
#include <chrono>
#include <random>
#include <cstdint>
#include <vector>

#include <cstdlib>
#include <memory>
#include <cmath>
#include <algorithm>
#include <array>


#include "XoshiroCpp.hpp"

using namespace std;

// add a terminal boolean so we dont have to evaluate every time ?
struct GameState {
    // doesnt require extra memory 
    static constexpr uint16_t OUT_OF_BOUNDS = 0b1111111000000000;

    static constexpr uint16_t FULL_BOARD = 0b0000000111111111;

    static constexpr uint16_t COL_1 = 0b0000000100100100;
    static constexpr uint16_t COL_2 = 0b0000000010010010;
    static constexpr uint16_t COL_3 = 0b0000000001001001;

    static constexpr uint16_t ROW_1 = 0b0000000111000000;
    static constexpr uint16_t ROW_2 = 0b0000000000111000;
    static constexpr uint16_t ROW_3 = 0b0000000000000111;

    static constexpr uint16_t DIAG_UP = 0b0000000001010100;
    static constexpr uint16_t DIAG_DOWN = 0b0000000100010001;

    static constexpr uint16_t WINNING_PATTERNS[] = {
        COL_1, COL_2, COL_3,
        ROW_1, ROW_2, ROW_3,
        DIAG_UP, DIAG_DOWN
    };

    uint16_t agent = 0;
    uint16_t opponent = 0;

    uint16_t move_idx = -1;
    bool agent_turn = true;

    uint16_t possible_moves = 9;
    bool is_terminal = false;
    int score = 0;

    GameState() {}


    void playMove(const uint16_t& move_idx) {
        this->move_idx = move_idx;
        this->possible_moves--;
        if (this->agent_turn) {
            this->agent |= 0b1 << move_idx;
            this->agent_turn = false;
            this->is_terminal = this->isDraw() || this->evaluateAgent();
        }
        else {
            this->opponent |= 0b1 << move_idx;
            this->agent_turn = true;
            this->is_terminal = this->isDraw() || this->evaluateOpponent();
        }
    }

    constexpr bool isDraw() const {
        return (this->agent | this->opponent) == FULL_BOARD;
    }

    // break into two functions 
    // might be useful later ? 
    constexpr int evaluateAgent() {
        for (const uint16_t pattern : WINNING_PATTERNS) {
            if ((this->agent & pattern) == pattern) {
                this->score = 1;
                return 1;
            }
        }
        return 0;
    }

    constexpr int evaluateOpponent() {
        for (const uint16_t pattern : WINNING_PATTERNS) {
            if ((this->opponent & pattern) == pattern){
                this->score = -1;
                return -1;
            }
        }
        return 0;
    }

    // gets all possible moves a player can put a marker 
    // can later make array and keep track of how many open spots there are 
    
    // gets the actual int (not index) for move
    // blsi needs march=native flag
    // uint16_t lsb = _blsi_u32(board);
    // uint16_t lsb = (board & -board);
    // board ^= lsb;

    array<GameState, 9> generateValidMoveStates() const {
        uint16_t board = (~(this->agent | this->opponent)) ^ OUT_OF_BOUNDS;
        array<GameState, 9> next_states;
        int i = 0;
        while (board) {
            uint16_t idx = __builtin_ctz(board);
            GameState next_state = *this;

            next_state.playMove(idx);

            next_states[i] = (next_state);
            i++;
            board ^= 0b1 << idx;
        }
        return next_states;
    }

    // need to change based on whether the agent goes first or not 
    void print_board() const {
        for (int i = 2; i >= 0; i--) {
            for (int j = 2; j>=0; j--) {
                uint16_t idx = 0b1 << (i * 3 + j);
                if (this->agent & idx)
                    cout << "X ";
                else if (this->opponent & idx) 
                    cout << "O ";
                else
                    cout << (i * 3 + j) << " ";
            }
            cout << endl;
        }
    }
};


// random_device r;
// default_random_engine generator{r()};
XoshiroCpp::Xoshiro256Plus rng(111);

// changing vectors to arrays 

struct Node {
    Node* parent;
    GameState state;
    array<unique_ptr<Node>, 9> children;
    array<GameState, 9> unexpanded_states;
    uint32_t states_left;

    int unexpanded_states_idx = 0;
    int children_idx = 0;

    int score = 0;  
    uint32_t visits = 0;

    Node() {}

    Node(Node* parent, const GameState &state) {
        this->parent = parent;
        this->state = state;

        this->unexpanded_states = state.generateValidMoveStates();
        this->states_left = state.possible_moves;
    }

    Node* select() {
        std::unique_ptr<Node>* best_child = nullptr;  // Pointer to unique_ptr
        double best_value = -INFINITY;  // Initialize to a very small value

        // go through all the children and find the one with the highest UCB1 value
        for (int i = 0; i < this->state.possible_moves; i++) {  // Notice the auto& to get a reference to the unique_ptr
            auto& child = this->children[i];
            // UCB1 formula
            // double q_value = 1 - ((child->score / (double)child->visits) + 1) / 2;
            double ucb = (child->score / (double)child->visits) + 2 * sqrt(log(this->visits) / (double)child->visits);
            
            if (ucb > best_value) {
                best_value = ucb;
                best_child = &child;  // Notice that we're storing a pointer to the unique_ptr
            }
        }
        return best_child->get();  // De-referencing to return the actual unique_ptr
    }


    // goal is to get a new state from list of unexpanded states
    // returning new node with that state 

    // is better way just create all child nodes and then check to see if it has been visited ? 
    // runs into the issue of randomly picking a child node that has already been visited
    // unless going through sequentially (shuffle? )


    // if we need 2 vectors, could store list of indexes and then compute state? 
    // need to still pop and push 
    // save memory 

    // expand should be random ? 
    Node* expand() {
        GameState nextState = this->unexpanded_states[this->unexpanded_states_idx];
        this->unexpanded_states_idx++;

        this->states_left--;
        
        auto child = std::make_unique<Node>(this, nextState);
        Node* raw_child_ptr = child.get(); // Store the raw pointer before moving ownership to the vector
        this->children[this->children_idx] = (std::move(child)); // std::move transfers ownership
        this->children_idx++;

        return raw_child_ptr;
    }

    int simulate() {
        GameState current_state = this->state;
        while (!current_state.is_terminal) {
            // Generate valid moves for the current state
            array<GameState, 9> next_states = current_state.generateValidMoveStates();

            // Pick a random move
            uniform_int_distribution<int> dist(0, current_state.possible_moves - 1);
            current_state = next_states[dist(rng)];
        }

        // Evaluate the terminal state to find out who won
        // Return 1 if win, 0 if draw, -1 if lose
        return current_state.score;
    }

    void backpropagate(int result) {
        Node* current = this; // 'this' is already a raw pointer
        while (current != nullptr) {
            current->visits++;
            current->score += result;
            current = current->parent; // Get the raw pointer from unique_ptr
        }
    }

    // did we go through all of the possible moves? 
    bool fullyExpanded() const {
        return this->unexpanded_states_idx == this->state.possible_moves;
        // return this->unexpanded_states.empty();
        // return this->children.size() == this->states_left;
    }

    // is this node a terminal state? 
    bool isLeaf() const {
        return this->state.is_terminal;
        // return this->state.isTerminal();
    }
};


struct MCTS {

    std::unique_ptr<Node> root;

    MCTS(const GameState& state) {
        this->root = std::make_unique<Node>(nullptr, state);
    }

    uint16_t search() {
        auto time_start = chrono::steady_clock::now();
        int iterations = 0;
        while (true) {
            auto time_now = chrono::steady_clock::now();
            if (chrono::duration<double, milli>(time_now - time_start).count() > 500)
                break;

            Node* node = this->root.get();  // get the raw pointer from the unique_ptr

            while (node->fullyExpanded()) {
                node = node->select();  // select() returns a Node*
            }

            // if the node is not a terminal state, expand and simulate
            if (!node->isLeaf()) {
                node = node->expand();  // expand() returns a Node*
                int value = node->simulate();
                node->backpropagate(value);
            }
            iterations++;
        }
        cout << "Iterations: " << iterations << endl;

        double best_ratio = -INFINITY;
        uint16_t best_play;
        for (int i = 0; i < this->root->state.possible_moves; i++) {
            auto& child = this->root->children[i];
            double win_ratio = child->score / (double)child->visits;

            if (win_ratio > best_ratio) {
                best_ratio = win_ratio;
                best_play = child->state.move_idx;
            }
        }
        return best_play;
    }
};

int main() {

    GameState state;
    MCTS mcts(state);

    // uint16_t move = mcts.everything();
    // cout << "Best move idx " <<  move << endl;

    uint16_t move = mcts.search();
    cout << "MOVE: " <<  move << endl;

    // cout << "Parent visits " << mcts.root->visits << endl;

    for (int i = 0; i < mcts.root->state.possible_moves; i++) {
        auto& child = mcts.root->children[i];
        cout << "Score " << child->score << " " << "Visits " << child->visits << endl;
        cout << "Win ratio " << child->score / (double)child->visits << endl;
        child->state.print_board();
        cout << endl;
    }

    return 0;
}