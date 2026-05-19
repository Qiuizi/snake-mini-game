#include "game.hpp"

#include <algorithm>
#include <random>

static std::mt19937& rng() {
    static std::mt19937 generator(std::random_device{}());
    return generator;
}

Game::Game() {
    restart();
}

void Game::restart() {
    snake_.clear();
    const int row = BOARD_HEIGHT / 2;
    const int col = BOARD_WIDTH / 2;
    snake_.push_back({row, col});
    snake_.push_back({row, col - 1});
    snake_.push_back({row, col - 2});

    direction_ = Direction::Right;
    nextDirection_ = Direction::Right;
    score_ = 0;
    level_ = 1;
    gameOver_ = false;
    paused_ = false;
    placeFood();
}

void Game::setDirection(Direction direction) {
    if (gameOver_) return;

    const bool opposite =
        (direction_ == Direction::Up && direction == Direction::Down) ||
        (direction_ == Direction::Down && direction == Direction::Up) ||
        (direction_ == Direction::Left && direction == Direction::Right) ||
        (direction_ == Direction::Right && direction == Direction::Left);

    if (!opposite) {
        nextDirection_ = direction;
    }
}

void Game::turnUp() {
    setDirection(Direction::Up);
}

void Game::turnDown() {
    setDirection(Direction::Down);
}

void Game::turnLeft() {
    setDirection(Direction::Left);
}

void Game::turnRight() {
    setDirection(Direction::Right);
}

void Game::tick() {
    if (paused_ || gameOver_) return;

    direction_ = nextDirection_;
    Cell head = snake_.front();
    switch (direction_) {
        case Direction::Up:    --head.first; break;
        case Direction::Down:  ++head.first; break;
        case Direction::Left:  --head.second; break;
        case Direction::Right: ++head.second; break;
    }

    const bool eating = head == food_;
    if (hitsWall(head) || hitsSelf(head, !eating)) {
        gameOver_ = true;
        return;
    }

    snake_.push_front(head);
    if (eating) {
        score_ += 10 * level_;
        updateLevel();
        placeFood();
    } else {
        snake_.pop_back();
    }
}

void Game::togglePause() {
    if (!gameOver_) {
        paused_ = !paused_;
    }
}

int Game::tickIntervalMs() const {
    return std::max(70, 220 - (level_ - 1) * 18);
}

bool Game::isSnakeCell(int row, int col) const {
    return occupies({row, col});
}

bool Game::isHeadCell(int row, int col) const {
    return !snake_.empty() && snake_.front() == Cell{row, col};
}

bool Game::isFoodCell(int row, int col) const {
    return food_ == Cell{row, col};
}

void Game::placeFood() {
    if (static_cast<int>(snake_.size()) >= BOARD_WIDTH * BOARD_HEIGHT) {
        gameOver_ = true;
        return;
    }

    std::uniform_int_distribution<int> rowDist(0, BOARD_HEIGHT - 1);
    std::uniform_int_distribution<int> colDist(0, BOARD_WIDTH - 1);

    do {
        food_ = {rowDist(rng()), colDist(rng())};
    } while (occupies(food_));
}

bool Game::occupies(const Cell& cell) const {
    return std::find(snake_.begin(), snake_.end(), cell) != snake_.end();
}

bool Game::hitsWall(const Cell& cell) const {
    return cell.first < 0 || cell.first >= BOARD_HEIGHT ||
           cell.second < 0 || cell.second >= BOARD_WIDTH;
}

bool Game::hitsSelf(const Cell& cell, bool tailWillMove) const {
    auto end = snake_.end();
    if (tailWillMove && !snake_.empty()) {
        --end;
    }
    return std::find(snake_.begin(), end, cell) != end;
}

void Game::updateLevel() {
    level_ = 1 + score_ / 50;
}
