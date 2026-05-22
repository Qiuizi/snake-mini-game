#include "game.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

using namespace ftxui;

static std::string tankGlyph(Facing direction, bool player) {
    if (player) {
        switch (direction) {
            case Facing::Up: return "/\\";
            case Facing::Down: return "\\/";
            case Facing::Left: return "<]";
            case Facing::Right: return "[]";
        }
    }
    switch (direction) {
        case Facing::Up: return "^^";
        case Facing::Down: return "vv";
        case Facing::Left: return "<<";
        case Facing::Right: return ">>";
    }
    return "[]";
}

static Element renderBoard(const Game& game) {
    Elements rows;
    for (int row = 0; row < Game::BOARD_HEIGHT; ++row) {
        Elements cells;
        for (int col = 0; col < Game::BOARD_WIDTH; ++col) {
            if (game.isPlayerAt(row, col)) {
                auto cell = text(tankGlyph(game.playerDirection(), true)) |
                            color(Color::GreenLight) | bold;
                if (game.isPlayerShielded()) {
                    cell = cell | bgcolor(Color::Blue);
                }
                cells.push_back(cell);
            } else if (game.isEnemyAt(row, col)) {
                cells.push_back(text(tankGlyph(game.enemyDirectionAt(row, col), false)) |
                                color(Color::RedLight) | bold);
            } else if (game.isBulletAt(row, col)) {
                cells.push_back(text("**") | color(Color::YellowLight) | bold);
            } else {
                switch (game.terrainAt(row, col)) {
                    case Terrain::Empty:
                        cells.push_back(text(" .") | color(Color::GrayDark));
                        break;
                    case Terrain::Brick:
                        cells.push_back(text("[]") | color(Color(210, 105, 30)));
                        break;
                    case Terrain::Steel:
                        cells.push_back(text("##") | color(Color::GrayLight));
                        break;
                    case Terrain::Base:
                        cells.push_back(text("HQ") | color(Color::CyanLight) | bold);
                        break;
                }
            }
        }
        rows.push_back(hbox(std::move(cells)));
    }
    return vbox(std::move(rows)) | border;
}

static Element renderStats(const Game& game) {
    return vbox({
        text(" TANK BATTLE ") | bold | center,
        separator(),
        text(" SCORE ") | bold | center,
        text(" " + std::to_string(game.score())) | center,
        separator(),
        text(" LIVES ") | bold | center,
        text(" " + std::to_string(game.lives())) | center,
        separator(),
        text(" WAVE ") | bold | center,
        text(" " + std::to_string(game.wave())) | center,
        separator(),
        text(" ENEMIES ") | bold | center,
        text(" " + std::to_string(game.enemiesRemaining())) | center,
    }) | border;
}

static Element renderControls() {
    return vbox({
        text(" CONTROLS ") | bold | center,
        separator(),
        text(" Arrows / WASD  Move"),
        text(" Space / J      Fire"),
        text(" P              Pause"),
        text(" R              Restart"),
        text(" Q / Esc        Quit"),
    }) | border;
}

static Element renderLegend() {
    return vbox({
        text(" LEGEND ") | bold | center,
        separator(),
        text(" /\\ \\/ <> []  Player"),
        text(" ##          Enemy"),
        text(" []          Brick"),
        text(" ##          Steel"),
        text(" HQ          Base"),
        text(" **          Shell"),
    }) | border;
}

static Element renderMessage(const Game& game) {
    if (game.isVictory()) {
        return vbox({
            text(" VICTORY ") | bold | center | color(Color::GreenLight),
            text(" Press R to play again ") | center | dim,
        }) | border;
    }

    if (game.isGameOver()) {
        return vbox({
            text(" GAME OVER ") | bold | center | color(Color::RedLight),
            text(" Press R to restart ") | center | dim,
        }) | border;
    }

    if (game.isPaused()) {
        return text(" PAUSED ") | bold | center | color(Color::Yellow) | border;
    }

    return text(" Clear 3 waves, defend HQ, stay alive. ") | center | dim;
}

static Element renderGame(const Game& game) {
    auto right = vbox({
        renderStats(game),
        text(""),
        renderControls(),
        text(""),
        renderLegend(),
        text(""),
        renderMessage(game),
    });

    return hbox({
        renderBoard(game),
        text("  "),
        right,
    }) | center;
}

int main() {
    Game game;
    auto screen = ScreenInteractive::Fullscreen();

    std::atomic<bool> running{true};
    std::thread timer([&] {
        using namespace std::chrono;
        while (running.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(milliseconds(game.tickIntervalMs()));
            if (running.load(std::memory_order_relaxed)) {
                screen.PostEvent(Event::Custom);
            }
        }
    });

    auto component = Renderer([&] { return renderGame(game); });

    component |= CatchEvent([&](Event event) {
        if (event == Event::Custom) {
            game.tick();
            return true;
        }

        if (event == Event::Character('q') || event == Event::Character('Q') ||
            event == Event::Escape) {
            running.store(false, std::memory_order_relaxed);
            screen.Exit();
            return true;
        }

        if (event == Event::Character('p') || event == Event::Character('P')) {
            game.togglePause();
            return true;
        }

        if (event == Event::Character('r') || event == Event::Character('R')) {
            game.restart();
            return true;
        }

        if (event == Event::Character(' ') || event == Event::Character('j') ||
            event == Event::Character('J')) {
            game.fire();
            return true;
        }

        if (game.isPaused() || game.isGameOver() || game.isVictory()) return false;

        if (event == Event::ArrowUp || event == Event::Character('w') ||
            event == Event::Character('W')) {
            game.moveUp();
            return true;
        }
        if (event == Event::ArrowDown || event == Event::Character('s') ||
            event == Event::Character('S')) {
            game.moveDown();
            return true;
        }
        if (event == Event::ArrowLeft || event == Event::Character('a') ||
            event == Event::Character('A')) {
            game.moveLeft();
            return true;
        }
        if (event == Event::ArrowRight || event == Event::Character('d') ||
            event == Event::Character('D')) {
            game.moveRight();
            return true;
        }

        return false;
    });

    screen.Loop(component);

    running.store(false, std::memory_order_relaxed);
    if (timer.joinable()) {
        timer.join();
    }

    return 0;
}
