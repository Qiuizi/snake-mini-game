#pragma once

#include <deque>
#include <utility>

enum class Direction { Up, Down, Left, Right };

class Game {
public:
    static constexpr int BOARD_WIDTH = 24;
    static constexpr int BOARD_HEIGHT = 18;

    Game();

    void turnUp();
    void turnDown();
    void turnLeft();
    void turnRight();
    void tick();
    void togglePause();
    void restart();

    bool isGameOver() const { return gameOver_; }
    bool isPaused() const { return paused_; }
    int score() const { return score_; }
    int level() const { return level_; }
    int length() const { return static_cast<int>(snake_.size()); }
    int tickIntervalMs() const;

    bool isSnakeCell(int row, int col) const;
    bool isHeadCell(int row, int col) const;
    bool isFoodCell(int row, int col) const;

private:
    using Cell = std::pair<int, int>;

    void setDirection(Direction direction);
    void placeFood();
    bool occupies(const Cell& cell) const;
    bool hitsWall(const Cell& cell) const;
    bool hitsSelf(const Cell& cell, bool tailWillMove) const;
    void updateLevel();

    std::deque<Cell> snake_;
    Cell food_{0, 0};
    Direction direction_ = Direction::Right;
    Direction nextDirection_ = Direction::Right;
    int score_ = 0;
    int level_ = 1;
    bool gameOver_ = false;
    bool paused_ = false;
};
