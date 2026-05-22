#include "game.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <queue>
#include <random>
#include <string>

namespace {
constexpr int kEnemiesPerWave = 12;
constexpr int kMaxActiveEnemies = 4;
constexpr int kPlayerFireCooldown = 6;
constexpr int kEnemyFireCooldown = 18;
constexpr int kEnemyMoveDelay = 4;
constexpr int kMaxWaves = 3;
constexpr int kPlayerInvulnerabilityTicks = 24;

std::mt19937& rng() {
    static std::mt19937 generator(std::random_device{}());
    return generator;
}

int randomInt(int min, int max) {
    return std::uniform_int_distribution<int>(min, max)(rng());
}
}  // namespace

Game::Game() {
    restart();
}

void Game::restart() {
    terrain_.assign(BOARD_HEIGHT, std::vector<Terrain>(BOARD_WIDTH, Terrain::Empty));
    enemies_.clear();
    bullets_.clear();
    enemySpawns_.clear();

    player_ = {BOARD_HEIGHT - 3, BOARD_WIDTH / 2, Facing::Up, true, 0, 0};
    playerSpawn_ = {player_.row, player_.col};
    score_ = 0;
    lives_ = 3;
    wave_ = 1;
    spawnedEnemies_ = 0;
    destroyedEnemies_ = 0;
    tickCount_ = 0;
    playerInvulnerability_ = kPlayerInvulnerabilityTicks;
    gameOver_ = false;
    victory_ = false;
    paused_ = false;

    if (!loadMapFromFile("maps/level1.txt")) {
        buildMap();
    }
    spawnEnemy();
    spawnEnemy();
}

void Game::buildMap() {
    enemySpawns_ = {{0, 2}, {0, BOARD_WIDTH / 2}, {0, BOARD_WIDTH - 3}};

    for (int row = 2; row < BOARD_HEIGHT - 3; row += 3) {
        for (int col = 3; col < BOARD_WIDTH - 3; col += 6) {
            terrain_[row][col] = Terrain::Brick;
            terrain_[row + 1][col] = Terrain::Brick;
            terrain_[row][col + 1] = Terrain::Brick;
        }
    }

    for (int col = 8; col < BOARD_WIDTH - 8; ++col) {
        if (col % 2 == 0) {
            terrain_[BOARD_HEIGHT / 2][col] = Terrain::Steel;
        }
    }

    const int baseRow = BOARD_HEIGHT - 1;
    const int baseCol = BOARD_WIDTH / 2;
    terrain_[baseRow][baseCol] = Terrain::Base;
    terrain_[baseRow][baseCol - 1] = Terrain::Brick;
    terrain_[baseRow][baseCol + 1] = Terrain::Brick;
    terrain_[baseRow - 1][baseCol - 1] = Terrain::Brick;
    terrain_[baseRow - 1][baseCol + 1] = Terrain::Brick;

    terrain_[player_.row][player_.col] = Terrain::Empty;
    terrain_[player_.row][player_.col - 1] = Terrain::Empty;
    terrain_[player_.row][player_.col + 1] = Terrain::Empty;
}

bool Game::loadMapFromFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) return false;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line) && static_cast<int>(lines.size()) < BOARD_HEIGHT) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }

    if (static_cast<int>(lines.size()) != BOARD_HEIGHT) return false;

    std::vector<std::vector<Terrain>> loadedTerrain(
        BOARD_HEIGHT, std::vector<Terrain>(BOARD_WIDTH, Terrain::Empty));
    std::vector<std::pair<int, int>> loadedEnemySpawns;
    std::pair<int, int> loadedPlayerSpawn = playerSpawn_;
    bool hasPlayer = false;
    bool hasBase = false;

    for (int row = 0; row < BOARD_HEIGHT; ++row) {
        if (static_cast<int>(lines[row].size()) != BOARD_WIDTH) return false;

        for (int col = 0; col < BOARD_WIDTH; ++col) {
            switch (lines[row][col]) {
                case '#':
                    loadedTerrain[row][col] = Terrain::Steel;
                    break;
                case 'B':
                    loadedTerrain[row][col] = Terrain::Brick;
                    break;
                case 'H':
                    loadedTerrain[row][col] = Terrain::Base;
                    hasBase = true;
                    break;
                case 'P':
                    loadedPlayerSpawn = {row, col};
                    loadedTerrain[row][col] = Terrain::Empty;
                    hasPlayer = true;
                    break;
                case 'E':
                    loadedEnemySpawns.push_back({row, col});
                    loadedTerrain[row][col] = Terrain::Empty;
                    break;
                case '.':
                case ' ':
                    loadedTerrain[row][col] = Terrain::Empty;
                    break;
                default:
                    return false;
            }
        }
    }

    if (!hasPlayer || !hasBase) return false;

    if (loadedEnemySpawns.empty()) {
        loadedEnemySpawns = {{0, 2}, {0, BOARD_WIDTH / 2}, {0, BOARD_WIDTH - 3}};
    }

    terrain_ = std::move(loadedTerrain);
    enemySpawns_ = std::move(loadedEnemySpawns);
    playerSpawn_ = loadedPlayerSpawn;
    resetPlayerToSpawn();
    return true;
}

void Game::moveUp() {
    turnAndMovePlayer(Facing::Up);
}

void Game::moveDown() {
    turnAndMovePlayer(Facing::Down);
}

void Game::moveLeft() {
    turnAndMovePlayer(Facing::Left);
}

void Game::moveRight() {
    turnAndMovePlayer(Facing::Right);
}

void Game::turnAndMovePlayer(Facing direction) {
    if (paused_ || gameOver_ || victory_) return;
    moveTank(player_, direction);
}

void Game::fire() {
    if (paused_ || gameOver_ || victory_) return;
    if (player_.fireCooldown == 0) {
        shootFrom(player_, BulletOwner::Player);
        player_.fireCooldown = kPlayerFireCooldown;
    }
}

void Game::tick() {
    if (paused_ || gameOver_ || victory_) return;

    ++tickCount_;
    if (player_.fireCooldown > 0) --player_.fireCooldown;
    if (playerInvulnerability_ > 0) --playerInvulnerability_;

    for (auto& enemy : enemies_) {
        if (enemy.fireCooldown > 0) --enemy.fireCooldown;
        if (enemy.moveCooldown > 0) --enemy.moveCooldown;
    }

    updateBullets();

    if (tickCount_ % kEnemyMoveDelay == 0) {
        updateEnemies();
    }

    if (tickCount_ % 24 == 0) {
        spawnEnemy();
    }

    updateVictory();
}

void Game::togglePause() {
    if (!gameOver_ && !victory_) {
        paused_ = !paused_;
    }
}

int Game::enemiesRemaining() const {
    return std::max(0, kEnemiesPerWave - destroyedEnemies_);
}

int Game::tickIntervalMs() const {
    return std::max(60, 105 - (wave_ - 1) * 12);
}

Terrain Game::terrainAt(int row, int col) const {
    if (!inBounds(row, col)) return Terrain::Steel;
    return terrain_[row][col];
}

bool Game::isPlayerAt(int row, int col) const {
    return player_.alive && player_.row == row && player_.col == col;
}

bool Game::isEnemyAt(int row, int col) const {
    return std::any_of(enemies_.begin(), enemies_.end(), [&](const Tank& enemy) {
        return enemy.alive && enemy.row == row && enemy.col == col;
    });
}

bool Game::isBulletAt(int row, int col) const {
    return std::any_of(bullets_.begin(), bullets_.end(), [&](const Bullet& bullet) {
        return bullet.active && bullet.row == row && bullet.col == col;
    });
}

Facing Game::playerDirection() const {
    return player_.direction;
}

Facing Game::enemyDirectionAt(int row, int col) const {
    const auto enemy = std::find_if(enemies_.begin(), enemies_.end(), [&](const Tank& tank) {
        return tank.alive && tank.row == row && tank.col == col;
    });
    return enemy == enemies_.end() ? Facing::Down : enemy->direction;
}

void Game::moveTank(Tank& tank, Facing direction) {
    tank.direction = direction;

    const auto [nextRow, nextCol] = stepFrom(tank.row, tank.col, direction);
    if (!isBlockedForTank(nextRow, nextCol, &tank)) {
        tank.row = nextRow;
        tank.col = nextCol;
    }
}

void Game::spawnEnemy() {
    if (spawnedEnemies_ >= kEnemiesPerWave) return;
    if (static_cast<int>(enemies_.size()) >= kMaxActiveEnemies) return;

    const int start = randomInt(0, static_cast<int>(enemySpawns_.size()) - 1);
    for (int i = 0; i < static_cast<int>(enemySpawns_.size()); ++i) {
        const auto [row, col] = enemySpawns_[(start + i) % enemySpawns_.size()];
        if (!isBlockedForTank(row, col, nullptr)) {
            enemies_.push_back({row, col, Facing::Down, true, randomInt(6, 14), 0});
            ++spawnedEnemies_;
            return;
        }
    }
}

void Game::updateEnemies() {
    for (auto& enemy : enemies_) {
        if (!enemy.alive) continue;

        Facing aimDirection = enemy.direction;
        if (hasLineOfSight(enemy, aimDirection) &&
            enemy.fireCooldown == 0 && randomInt(0, 99) < 55) {
            enemy.direction = aimDirection;
            shootFrom(enemy, BulletOwner::Enemy);
            enemy.fireCooldown = kEnemyFireCooldown;
        }

        Facing direction = chooseEnemyDirection(enemy);
        if (randomInt(0, 99) < 12) {
            direction = static_cast<Facing>(randomInt(0, 3));
        }

        const auto [nextRow, nextCol] = stepFrom(enemy.row, enemy.col, direction);
        if (isBlockedForTank(nextRow, nextCol, &enemy)) {
            enemy.direction = static_cast<Facing>(randomInt(0, 3));
        } else {
            moveTank(enemy, direction);
        }

        if (enemy.fireCooldown == 0 && randomInt(0, 99) < 18) {
            shootFrom(enemy, BulletOwner::Enemy);
            enemy.fireCooldown = kEnemyFireCooldown;
        }
    }
}

void Game::updateBullets() {
    for (auto& bullet : bullets_) {
        if (!bullet.active) continue;

        const auto [nextRow, nextCol] = stepFrom(bullet.row, bullet.col, bullet.direction);
        bullet.row = nextRow;
        bullet.col = nextCol;

        if (!inBounds(bullet.row, bullet.col)) {
            bullet.active = false;
            continue;
        }

        Terrain& terrain = terrain_[bullet.row][bullet.col];
        if (terrain == Terrain::Brick) {
            terrain = Terrain::Empty;
            bullet.active = false;
            continue;
        }
        if (terrain == Terrain::Steel) {
            bullet.active = false;
            continue;
        }
        if (terrain == Terrain::Base) {
            bullet.active = false;
            hitBase();
            continue;
        }

        if (bullet.owner == BulletOwner::Player) {
            for (auto& enemy : enemies_) {
                if (enemy.alive && enemy.row == bullet.row && enemy.col == bullet.col) {
                    enemy.alive = false;
                    bullet.active = false;
                    score_ += 100;
                    ++destroyedEnemies_;
                    break;
                }
            }
        } else if (playerInvulnerability_ == 0 && player_.alive &&
                   player_.row == bullet.row && player_.col == bullet.col) {
            bullet.active = false;
            hitPlayer();
        }
    }

    for (auto& bullet : bullets_) {
        if (!bullet.active) continue;
        const auto matching = std::find_if(bullets_.begin(), bullets_.end(), [&](const Bullet& other) {
            return &bullet != &other && other.active && other.row == bullet.row &&
                   other.col == bullet.col && other.owner != bullet.owner;
        });
        if (matching != bullets_.end()) {
            bullet.active = false;
            matching->active = false;
        }
    }

    bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(), [](const Bullet& bullet) {
        return !bullet.active;
    }), bullets_.end());

    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Tank& enemy) {
        return !enemy.alive;
    }), enemies_.end());
}

void Game::shootFrom(const Tank& tank, BulletOwner owner) {
    bullets_.push_back({tank.row, tank.col, tank.direction, owner, true});
}

void Game::hitPlayer() {
    --lives_;
    if (lives_ <= 0) {
        gameOver_ = true;
        return;
    }

    resetPlayerToSpawn();
    player_.fireCooldown = 8;
    playerInvulnerability_ = kPlayerInvulnerabilityTicks;

    for (auto& bullet : bullets_) {
        if (bullet.owner == BulletOwner::Enemy &&
            std::abs(bullet.row - player_.row) <= 1 &&
            std::abs(bullet.col - player_.col) <= 1) {
            bullet.active = false;
        }
    }
}

void Game::hitBase() {
    gameOver_ = true;
}

void Game::updateVictory() {
    if (destroyedEnemies_ >= kEnemiesPerWave) {
        if (wave_ >= kMaxWaves) {
            victory_ = true;
        } else {
            startNextWave();
        }
    }
}

void Game::startNextWave() {
    ++wave_;
    spawnedEnemies_ = 0;
    destroyedEnemies_ = 0;
    bullets_.clear();
    enemies_.clear();
    resetPlayerToSpawn();
    player_.fireCooldown = 0;
    playerInvulnerability_ = kPlayerInvulnerabilityTicks;

    spawnEnemy();
    spawnEnemy();
}

void Game::resetPlayerToSpawn() {
    player_.row = playerSpawn_.first;
    player_.col = playerSpawn_.second;
    player_.direction = Facing::Up;
    player_.alive = true;

    const int removedEnemies = removeEnemiesNearPlayerSpawn();
    spawnedEnemies_ = std::max(0, spawnedEnemies_ - removedEnemies);
}

int Game::removeEnemiesNearPlayerSpawn() {
    const auto oldSize = enemies_.size();
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [&](const Tank& enemy) {
        return enemy.alive && std::abs(enemy.row - player_.row) <= 1 &&
               std::abs(enemy.col - player_.col) <= 1;
    }), enemies_.end());
    return static_cast<int>(oldSize - enemies_.size());
}

bool Game::inBounds(int row, int col) const {
    return row >= 0 && row < BOARD_HEIGHT && col >= 0 && col < BOARD_WIDTH;
}

bool Game::isBlockedForTank(int row, int col, const Tank* moving) const {
    if (!inBounds(row, col)) return true;
    if (isObstacle(terrain_[row][col])) return true;
    return tankAt(row, col, moving);
}

bool Game::tankAt(int row, int col, const Tank* ignored) const {
    if (&player_ != ignored && player_.alive && player_.row == row && player_.col == col) {
        return true;
    }

    return std::any_of(enemies_.begin(), enemies_.end(), [&](const Tank& enemy) {
        return &enemy != ignored && enemy.alive && enemy.row == row && enemy.col == col;
    });
}

bool Game::isObstacle(Terrain terrain) const {
    return terrain == Terrain::Brick || terrain == Terrain::Steel || terrain == Terrain::Base;
}

bool Game::canPathThrough(int row, int col, const Tank* moving, int targetRow, int targetCol) const {
    if (!inBounds(row, col)) return false;
    if (row == targetRow && col == targetCol) return true;
    if (isObstacle(terrain_[row][col])) return false;
    return !tankAt(row, col, moving);
}

Facing Game::chooseEnemyDirection(const Tank& enemy) const {
    std::array<std::array<bool, BOARD_WIDTH>, BOARD_HEIGHT> visited{};
    std::array<std::array<std::pair<int, int>, BOARD_WIDTH>, BOARD_HEIGHT> parent{};
    std::queue<std::pair<int, int>> pending;

    visited[enemy.row][enemy.col] = true;
    parent[enemy.row][enemy.col] = {-1, -1};
    pending.push({enemy.row, enemy.col});

    const std::array<std::pair<int, int>, 4> deltas{{
        {-1, 0},
        {1, 0},
        {0, -1},
        {0, 1},
    }};

    while (!pending.empty()) {
        const auto [row, col] = pending.front();
        pending.pop();

        if (row == player_.row && col == player_.col) {
            std::pair<int, int> current{row, col};
            while (parent[current.first][current.second] != std::pair<int, int>{enemy.row, enemy.col}) {
                current = parent[current.first][current.second];
            }
            return directionFromDelta(current.first - enemy.row, current.second - enemy.col);
        }

        for (const auto [dRow, dCol] : deltas) {
            const int nextRow = row + dRow;
            const int nextCol = col + dCol;
            if (!inBounds(nextRow, nextCol) || visited[nextRow][nextCol]) continue;
            if (!canPathThrough(nextRow, nextCol, &enemy, player_.row, player_.col)) continue;

            visited[nextRow][nextCol] = true;
            parent[nextRow][nextCol] = {row, col};
            pending.push({nextRow, nextCol});
        }
    }

    if (std::abs(enemy.row - player_.row) > std::abs(enemy.col - player_.col)) {
        return enemy.row < player_.row ? Facing::Down : Facing::Up;
    }
    return enemy.col < player_.col ? Facing::Right : Facing::Left;
}

bool Game::hasLineOfSight(const Tank& enemy, Facing& direction) const {
    if (enemy.row != player_.row && enemy.col != player_.col) {
        return false;
    }

    if (enemy.row == player_.row) {
        direction = enemy.col < player_.col ? Facing::Right : Facing::Left;
    } else {
        direction = enemy.row < player_.row ? Facing::Down : Facing::Up;
    }

    auto [row, col] = stepFrom(enemy.row, enemy.col, direction);
    while (inBounds(row, col)) {
        if (row == player_.row && col == player_.col) return true;
        if (isObstacle(terrain_[row][col])) return false;
        if (tankAt(row, col, &enemy)) return false;
        const auto next = stepFrom(row, col, direction);
        row = next.first;
        col = next.second;
    }
    return false;
}

std::pair<int, int> Game::stepFrom(int row, int col, Facing direction) const {
    switch (direction) {
        case Facing::Up:    return {row - 1, col};
        case Facing::Down:  return {row + 1, col};
        case Facing::Left:  return {row, col - 1};
        case Facing::Right: return {row, col + 1};
    }
    return {row, col};
}

Facing Game::directionFromDelta(int dRow, int dCol) const {
    if (dRow < 0) return Facing::Up;
    if (dRow > 0) return Facing::Down;
    if (dCol < 0) return Facing::Left;
    return Facing::Right;
}
