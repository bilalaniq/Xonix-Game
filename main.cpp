#include <SFML/Graphics.hpp>
#include <time.h>
#include <cmath>
#include <cstdlib>
#include <string>
#include <cstring>
#include <SFML/Window.hpp>
#include <iostream>

// Define game states
enum GameState
{
    MENU,
    PLAY,
    GAMEOVER,
    LEVELS,
    MODES // Modes menu
};
GameState gameState = MENU;

enum GameMode
{
    SINGLE_PLAYER, // 1P mode
    TWO_PLAYER     // 2P mode
};
GameMode gameMode = SINGLE_PLAYER;

const int M = 35;
const int N = 55;

int grid[M][N] = {0};
int ts = 18;

struct Enemy
{
    int x, y, dx, dy;

    Enemy()
    {
        x = y = 300;
        dx = 4 - rand() % 11;
        dy = 4 - rand() % 11;
    }

    void move()
    {
        x += dx;
        if (grid[y / ts][x / ts] == 1) // if the enemy hits the wall
        {
            dx = -dx;
            x += dx;
        }
        y += dy;
        if (grid[y / ts][x / ts] == 1) // if the enemy hits the wall
        {
            dy = -dy;
            y += dy;
        }
    }
};

void drop(int y, int x)
{
    // this function is used to drop the enemy in the grid
    // if the enemy is in the grid, it will be dropped to the next cell
    if (grid[y][x] == 0)
        grid[y][x] = -1;
    if (grid[y - 1][x] == 0)
        drop(y - 1, x);
    if (grid[y + 1][x] == 0)
        drop(y + 1, x);
    if (grid[y][x - 1] == 0)
        drop(y, x - 1);
    if (grid[y][x + 1] == 0)
        drop(y, x + 1);
}

void construct_boundry()
{
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            if (i == 0 || j == 0 || i == M - 1 || j == N - 1)
                grid[i][j] = 1;
}

int main()
{
    srand(time(0));

    sf::RenderWindow window(sf::VideoMode(N * ts, M * ts), "Xonix Game!");
    window.setFramerateLimit(60);

    sf::Texture t1, t2, t3;
    t1.loadFromFile("images/tiles.png");
    t2.loadFromFile("images/gameover.png");
    t3.loadFromFile("images/enemy.png");

    sf::Sprite sTile(t1), sGameover(t2), sEnemy(t3);

    sGameover.setPosition(100, 100); // The origin (0, 0) is at the top-left corner of the window

    sEnemy.setOrigin(20, 20); // Set the origin to the center of the sprite which is 20x20 pixels(assuming the sprite is 40x40 pixels)

    // The sprite rotates around its origin point.
    // If the origin is at the center of the sprite, the rotation looks natural.
    // If the origin is at the top-left corner (default), the sprite rotates around that corner, which can look strange.

    int enemyCount = 4;
    Enemy a[10];

    bool Game = true;
    int x = 0, y = 0, dx = 0, dy = 0;
    float timer = 0, delay = 0.07;
    sf::Clock clock;

    construct_boundry(); // for making the boundry constructed

    sf::Texture menu_texture;
    menu_texture.loadFromFile("images/menu_xonix_pic.png");
    sf::Sprite menu_text(menu_texture);
    menu_text.setPosition(50, 20);
    menu_text.setScale(0.4f, 0.4f);

    sf::Texture menu_controller_texture;
    menu_controller_texture.loadFromFile("images/controller_pic.jpg");
    sf::Sprite menu_controller(menu_controller_texture);
    menu_controller.setPosition(550, 170);
    menu_controller.setScale(0.6f, 0.6f);

    sf::Texture start_buton_texture;
    start_buton_texture.loadFromFile("images/start_button.png");
    sf::Sprite start_button(start_buton_texture);
    start_button.setPosition(50, 150);
    start_button.setScale(0.5f, 0.5f);

    sf::Texture level_buton_texture;
    level_buton_texture.loadFromFile("images/levels.png");
    sf::Sprite level_button(level_buton_texture);
    level_button.setPosition(50, 280);
    level_button.setScale(1.4f, 1.4f);

    sf::Texture mode_buton_texture;
    mode_buton_texture.loadFromFile("images/modes.png");
    sf::Sprite mode_button(mode_buton_texture);
    mode_button.setPosition(50, 390); // Ensure this is correct
    mode_button.setScale(1.4f, 1.4f); // Ensure this is correct

    sf::Texture stop_buton_texture;
    stop_buton_texture.loadFromFile("images/stop_button.png");
    sf::Sprite stop_button(stop_buton_texture);
    stop_button.setPosition(50, 500);
    stop_button.setScale(0.5, 0.5f);

    sf::Texture _1p_buton_texture;
    _1p_buton_texture.loadFromFile("images/1p.png");
    sf::Sprite _1p_button(_1p_buton_texture);
    _1p_button.setPosition(50, 200);
    _1p_button.setScale(0.5, 0.5f);

    sf::Texture _2p_buton_texture;
    _2p_buton_texture.loadFromFile("images/2p.png");
    sf::Sprite _2p_button(_2p_buton_texture);
    _2p_button.setPosition(670, 200);
    _2p_button.setScale(0.5, 0.5f);

    sf::Texture back_button_texture;
    back_button_texture.loadFromFile("images/back.png");
    sf::Sprite back_button(back_button_texture);
    back_button.setPosition(50, 50);
    back_button.setScale(0.1, 0.1f);

    while (window.isOpen())
    {
        float time = clock.getElapsedTime().asSeconds();
        clock.restart();
        timer += time;

        // ensures that the game updates are tied to real time, not the frame rate. It makes the game run consistently on systems with
        // different performance levels.

        sf::Event e;

        while (window.pollEvent(e))
        {
            if (e.type == sf::Event::Closed) // when exiting the game
                window.close();

            if (gameState == MENU)
            {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter))
                {
                    gameState = PLAY; // Start the game
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                {
                    window.close(); // Exit the game
                }
            }

            if (e.type == sf::Event::KeyPressed)
                if (e.key.code == sf::Keyboard::Escape) // press escape to restart the game
                {
                    for (int i = 1; i < M - 1; i++)
                        for (int j = 1; j < N - 1; j++)
                            grid[i][j] = 0;

                    x = 0;
                    y = 0;
                    Game = true;
                }
        }

        if (gameState == MENU)
        {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);

            const float hoverScale = 0.52f;
            const float normalScale = 0.5f;

            sf::Vector2f currentScale_start = start_button.getScale();

            // "Start" button
            if (start_button.getGlobalBounds().contains((float)(mousePos.x), (float)(mousePos.y)))
            {
                if (currentScale_start.x < hoverScale)
                    start_button.setScale(currentScale_start.x + 0.01f, currentScale_start.y + 0.01f);
            }
            else
            {
                if (currentScale_start.x > normalScale)
                    start_button.setScale(currentScale_start.x - 0.01f, currentScale_start.y - 0.01f);
            }

            // sf::Vector2f currentScale = mode_button.getScale();

            if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left)
            {
                // Get the mouse position relative to the window
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                // Handle "Start" button click
                if (start_button.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
                {
                    gameState = PLAY; // Transition to PLAY state
                }
            }

            if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left)
            {
                // Get the mouse position relative to the window
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                // Handle "Levels" button click
                if (level_button.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
                {
                    gameState = LEVELS; // Transition to LEVELS state
                }
            }

            if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left)
            {
                // Get the mouse position relative to the window
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                if (mode_button.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
                {
                    std::cout << "Mode button clicked!" << std::endl;
                    gameState = MODES;
                }
            }

            // Draw the menu elements
            window.clear(sf::Color::White);
            window.draw(menu_text);
            window.draw(menu_controller);
            window.draw(start_button);
            window.draw(level_button);
            window.draw(mode_button);
            window.draw(stop_button);
            window.display();
            continue;
        }

        if (gameState == MODES)
        {

            window.clear(sf::Color(50, 158, 168));
            sf::Event e2;

            if (e2.type == sf::Event::MouseButtonPressed && e2.mouseButton.button == sf::Mouse::Left)
            {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                if (_1p_button.getGlobalBounds().contains((float)(mousePos.x), (float)(mousePos.y)))
                {   
                    gameMode = SINGLE_PLAYER;
                    gameState = PLAY;
                }
                if (_2p_button.getGlobalBounds().contains((float)(mousePos.x), (float)(mousePos.y)))
                {
                    gameMode = TWO_PLAYER;
                    gameState = PLAY;
                }
                if (back_button.getGlobalBounds().contains((float)(mousePos.x), (float)(mousePos.y)))
                {
                    gameState = MENU;
                }
            }

            window.draw(_1p_button);
            window.draw(_2p_button);
            window.draw(back_button);

            window.display();
            continue;
        }

        if (gameState == LEVELS)
        {
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        {
            dx = -1;
            dy = 0;
        };
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        {
            dx = 1;
            dy = 0;
        };
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
        {
            dx = 0;
            dy = -1;
        };
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
        {
            dx = 0;
            dy = 1;
        };

        if (!Game)
            continue;

        if (timer > delay) // Check if enough time has passed to update the player's movement
        {
            // move the player
            x += dx;
            y += dy;

            // detect collision with the grid boundaries
            if (x < 0)
                x = 0;
            if (x > N - 1)
                x = N - 1;
            if (y < 0)
                y = 0;
            if (y > M - 1)
                y = M - 1;

            // check if the player has hit a wall or an enemy
            if (grid[y][x] == 2)
                Game = false;
            // if the player hits a wall, set the position back to the last valid position
            if (grid[y][x] == 0)
                grid[y][x] = 2;

            timer = 0;
        }

        for (int i = 0; i < enemyCount; i++)
            a[i].move();

        if (grid[y][x] == 1)
        {
            dx = dy = 0;

            for (int i = 0; i < enemyCount; i++)
                drop(a[i].y / ts, a[i].x / ts);

            for (int i = 0; i < M; i++)
                for (int j = 0; j < N; j++)
                    if (grid[i][j] == -1)
                        grid[i][j] = 0;
                    else
                        grid[i][j] = 1; // this will replace the 2 with the 1 in the grid
        }

        for (int i = 0; i < enemyCount; i++) // if the enemy hits the players trail
            if (grid[a[i].y / ts][a[i].x / ts] == 2)
                Game = false;

        /////////draw//////////
        window.clear(); /// clear the window with black color so that the previous frame is not visible

        for (int i = 0; i < M; i++)
        {
            for (int j = 0; j < N; j++)
            {
                if (grid[i][j] == 0)
                    continue;
                if (grid[i][j] == 1)
                    sTile.setTextureRect(sf::IntRect(0, 0, ts, ts));
                if (grid[i][j] == 2)
                    sTile.setTextureRect(sf::IntRect(54, 0, ts, ts));
                sTile.setPosition(j * ts, i * ts);
                window.draw(sTile);
            }
        }

        sTile.setTextureRect(sf::IntRect(36, 0, ts, ts)); // this is the sprite for the player
        sTile.setPosition(x * ts, y * ts);
        window.draw(sTile);

        sEnemy.rotate(20); // rotate the enemy sprite on its own axis set by  sEnemy.setOrigin(20, 20);

        for (int i = 0; i < enemyCount; i++) // draw the enemies
        {
            sEnemy.setPosition(a[i].x, a[i].y);
            window.draw(sEnemy);
        }

        if (!Game)
            window.draw(sGameover);

        window.display();
    }

    return 0;
}
