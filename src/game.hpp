#pragma once

#include <string>
#include <utility>
#include <vector>

enum class Facing { Up, Down, Left, Right };
enum class Terrain { Empty, Brick, Steel, Base };
enum class BulletOwner { Player, Enemy };

struct Bullet {
    int row = 0;
    int col = 0;
    Facing direction = Facing::Up;
    BulletOwner owner = BulletOwner::Player;
    bool active = true;
};

class Game {
public:
    static constexpr int BOARD_WIDTH = 30;
    static constexpr int BOARD_HEIGHT = 20;

    Game();

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void fire();
    void tick();
    void togglePause();
    void restart();

    bool isGameOver() const { return gameOver_; }
    bool isPaused() const { return paused_; }
    bool isVictory() const { return victory_; }
    int score() const { return score_; }
    int lives() const { return lives_; }
    int wave() const { return wave_; }
    int enemiesRemaining() const;
    int tickIntervalMs() const;

    Terrain terrainAt(int row, int col) const;
    bool isPlayerAt(int row, int col) const;
    bool isEnemyAt(int row, int col) const;
    bool isBulletAt(int row, int col) const;
    Facing playerDirection() const;
    Facing enemyDirectionAt(int row, int col) const;
    bool isPlayerShielded() const { return playerInvulnerability_ > 0; }

private:
    struct Tank {
        int row = 0;
        int col = 0;
        Facing direction = Facing::Up;
        bool alive = true;
        int fireCooldown = 0;
        int moveCooldown = 0;
    };

    void buildMap();
    bool loadMapFromFile(const std::string& path);
    void turnAndMovePlayer(Facing direction);
    void moveTank(Tank& tank, Facing direction);
    void spawnEnemy();
    void updateEnemies();
    void updateBullets();
    void shootFrom(const Tank& tank, BulletOwner owner);
    void hitPlayer();
    void hitBase();
    void updateVictory();
    void startNextWave();
    void resetPlayerToSpawn();
    int removeEnemiesNearPlayerSpawn();

    bool inBounds(int row, int col) const;
    bool isBlockedForTank(int row, int col, const Tank* moving) const;
    bool tankAt(int row, int col, const Tank* ignored) const;
    bool isObstacle(Terrain terrain) const;
    bool canPathThrough(int row, int col, const Tank* moving, int targetRow, int targetCol) const;
    Facing chooseEnemyDirection(const Tank& enemy) const;
    bool hasLineOfSight(const Tank& enemy, Facing& direction) const;
    std::pair<int, int> stepFrom(int row, int col, Facing direction) const;
    Facing directionFromDelta(int dRow, int dCol) const;

    std::vector<std::vector<Terrain>> terrain_;
    Tank player_;
    std::vector<Tank> enemies_;
    std::vector<Bullet> bullets_;
    std::vector<std::pair<int, int>> enemySpawns_;
    std::pair<int, int> playerSpawn_;

    int score_ = 0;
    int lives_ = 3;
    int wave_ = 1;
    int spawnedEnemies_ = 0;
    int destroyedEnemies_ = 0;
    int tickCount_ = 0;
    int playerInvulnerability_ = 0;
    bool gameOver_ = false;
    bool victory_ = false;
    bool paused_ = false;
};
