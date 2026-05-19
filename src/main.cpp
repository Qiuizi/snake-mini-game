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

static Element renderBoard(const Game& game) {
    Elements rows;
    for (int row = 0; row < Game::BOARD_HEIGHT; ++row) {
        Elements cells;
        for (int col = 0; col < Game::BOARD_WIDTH; ++col) {
            if (game.isHeadCell(row, col)) {
                cells.push_back(text("  ") | bgcolor(Color::GreenLight));
            } else if (game.isSnakeCell(row, col)) {
                cells.push_back(text("  ") | bgcolor(Color::Green));
            } else if (game.isFoodCell(row, col)) {
                cells.push_back(text("()") | color(Color::RedLight) | bold);
            } else {
                cells.push_back(text(" .") | color(Color::GrayDark));
            }
        }
        rows.push_back(hbox(std::move(cells)));
    }
    return vbox(std::move(rows)) | border;
}

static Element renderStats(const Game& game) {
    return vbox({
        text(" SNAKE ") | bold | center,
        separator(),
        text(" SCORE ") | bold | center,
        text(" " + std::to_string(game.score())) | center,
        separator(),
        text(" LEVEL ") | bold | center,
        text(" " + std::to_string(game.level())) | center,
        separator(),
        text(" LENGTH ") | bold | center,
        text(" " + std::to_string(game.length())) | center,
    }) | border;
}

static Element renderControls() {
    return vbox({
        text(" CONTROLS ") | bold | center,
        separator(),
        text(" Arrows / WASD  Move"),
        text(" P              Pause"),
        text(" R              Restart"),
        text(" Q / Esc        Quit"),
    }) | border;
}

static Element renderMessage(const Game& game) {
    if (game.isGameOver()) {
        return vbox({
            text(" GAME OVER ") | bold | center | color(Color::RedLight),
            text(" Press R to restart ") | center | dim,
        }) | border;
    }

    if (game.isPaused()) {
        return text(" PAUSED ") | bold | center | color(Color::Yellow) | border;
    }

    return text(" Eat food, grow longer, stay alive. ") | center | dim;
}

static Element renderGame(const Game& game) {
    auto right = vbox({
        renderStats(game),
        text(""),
        renderControls(),
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

        if (game.isPaused() || game.isGameOver()) return false;

        if (event == Event::ArrowUp || event == Event::Character('w') ||
            event == Event::Character('W')) {
            game.turnUp();
            return true;
        }
        if (event == Event::ArrowDown || event == Event::Character('s') ||
            event == Event::Character('S')) {
            game.turnDown();
            return true;
        }
        if (event == Event::ArrowLeft || event == Event::Character('a') ||
            event == Event::Character('A')) {
            game.turnLeft();
            return true;
        }
        if (event == Event::ArrowRight || event == Event::Character('d') ||
            event == Event::Character('D')) {
            game.turnRight();
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
