#include <iostream>
#include <chrono>
#include <random>
#include <cstdint>
#include <vector>

#include <cstdlib>

#include <cmath>
#include <algorithm>

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

    GameState() {}


    void playMove(const uint16_t& move_idx) {
        this->move_idx = move_idx;
        if (this->agent_turn) {
            this->agent |= 0b1 << move_idx;
            this->agent_turn = false;
        }
        else {
            this->opponent |= 0b1 << move_idx;
            this->agent_turn = true;
        }
    }

    constexpr bool isDraw() const {
        return (this->agent | this->opponent) == FULL_BOARD;
    }

    // can evaluate just the player that made turn instead of both ?
    // have to evaluate both for snake
    constexpr int evaluateTerminalState() const {
        for (uint16_t pattern : WINNING_PATTERNS) {
            if ((this->agent & pattern) == pattern) 
                return 1;
            else if ((this->opponent & pattern) == pattern) 
                return -1;
        }
        return 0;
    }

    // break into two functions 
    // might be useful later ? 
    constexpr int evaluateAgent() const {
        for (uint16_t pattern : WINNING_PATTERNS) {
            if ((this->agent & pattern) == pattern) 
                return 1;
        }
        return 0;
    }

    constexpr int evaluateOpponent() const {
        for (uint16_t pattern : WINNING_PATTERNS) {
            if ((this->opponent & pattern) == pattern) 
                return -1;
        }
        return 0;
    }

    constexpr bool isTerminal() const {
        if (this->isDraw() || this->evaluateTerminalState())
            return true;
        return false;
    }

    // gets all possible moves a player can put a marker 
    // can later make array and keep track of how many open spots there are 
    
    // gets the actual int (not index) for move
    // blsi needs march=native flag
    // uint16_t lsb = _blsi_u32(board);
    // uint16_t lsb = (board & -board);
    // board ^= lsb;

    vector<GameState> generateValidMoveStates() const {
        uint16_t board = (~(this->agent | this->opponent)) ^ OUT_OF_BOUNDS;
        vector<GameState> next_states;

        while (board) {
            uint16_t idx = __builtin_ctz(board);
            GameState next_state = *this;

            next_state.playMove(idx);

            next_states.push_back(next_state);
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



random_device r;
default_random_engine generator{r()};

struct Node {
    Node* parent;
    GameState state;  
    vector<Node*> children;
    vector<GameState> unexpanded_states;
    uint32_t states_left;

    int score = 0;  
    uint32_t visits = 0;

    Node() {}

    Node(Node *parent, const GameState &state) {
        this->parent = parent;
        this->state = state;

        this->unexpanded_states = state.generateValidMoveStates();
        this->states_left = unexpanded_states.size();
    }

    ~Node() {
        for (Node* child : children) {
            delete child;
        }
    }

    Node* select() {
        Node* best_child = nullptr;
        double best_value = -INFINITY;  // Initialize to a very small value

        // go through all the children and find the one with the highest UCB1 value
        for (Node* child : this->children) {
            // UCB1 formula
            // double q_value = 1 - ((child->score / (double)child->visits) + 1) / 2;
            double ucb = (child->score / (double)child->visits) + 2 * sqrt(log(this->visits) / (double)child->visits);
            
            if (ucb > best_value) {
                best_value = ucb;
                best_child = child;
            }
        }
        return best_child;
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

        GameState nextState = this->unexpanded_states.back();
        this->unexpanded_states.pop_back();
        Node* child = new Node(this, nextState);
        this->children.push_back(child);
        return child;
    }

    int simulate() {
        GameState current_state = this->state;
        while (!current_state.isTerminal()) {
            // Generate valid moves for the current state
            vector<GameState> next_states = current_state.generateValidMoveStates();

            // Pick a random move
            uniform_int_distribution<int> distribution(0, next_states.size() - 1);
            current_state = next_states[distribution(generator)];
        }

        // Evaluate the terminal state to find out who won
        // Return 1 if win, 0 if draw, -1 if lose
        return current_state.evaluateTerminalState();
    }

    void backpropagate(int result) {
        Node* current = this;
        while (current != nullptr) {
            current->visits++;
            current->score += result; 
            current = current->parent;
        }
    }

    // did we go through all of the possible moves? 
    bool fullyExpanded() const {
        return this->unexpanded_states.empty();
        // return this->children.size() == this->states_left;
    }

    // is this node a terminal state? 
    bool isLeaf() const {
        return this->state.isTerminal();
    }
};


struct MCTS {

    Node* root;

    MCTS(GameState state) {
        this->root = new Node(nullptr, state);
    }

    ~MCTS() {
        delete this->root;
    }

    uint16_t search() {
        auto time_start = chrono::steady_clock::now();
        int iterations = 0;
        while (true) {
            auto time_now = chrono::steady_clock::now();
            if (chrono::duration<double, milli>(time_now - time_start).count() > 500)
                break;

            Node *node = this->root;
            
            while (node->fullyExpanded()) {
                node = node->select();
            }

            // if the node is not a terminal state, expand and simulate
            if (!node->isLeaf()) {
                node = node->expand();
                int value = node->simulate();
                node->backpropagate(value);
            }
            iterations++;
        }
        cout << "Iterations: " << iterations << endl;

        double best_ratio = -INFINITY;
        uint16_t best_play;
        for (Node* child : this->root->children) {
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

    // for (const Node* child : mcts.root->children) {
    //     cout << "Score " << child->score << " " << "Visits " << child->visits << endl;
    //     cout << "Win ratio " << child->score / (double)child->visits << endl;
    //     child->state.print_board();
    //     cout << endl;
    // }

    return 0;
}