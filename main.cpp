#include <SFML/Graphics.hpp>
#include <time.h>
#include <cmath>
#include <cstdlib>
#include <string>
#include <cstring>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
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

enum GameMode
{
    SINGLE_PLAYER, // 1P mode
    TWO_PLAYER     // 2P mode
};

enum GameLevel
{
    LEVEL_ONE,
    LEVEL_TWO,
    LEVEL_THREE
};

enum difficulty
{
    EASY,
    MEDIUM,
    HARD,
    continuous
};

enum movement_type
{
    LINEAR,
    ZIG_ZAG,
    CIRCULAR
};

GameState gameState = MENU;

difficulty gameDifficulty = EASY; // Default difficulty

GameLevel gameLevel = LEVEL_ONE; // Default level

GameMode gameMode = SINGLE_PLAYER;

float continuousModeTimer = 0; // Timer for adding enemies in Continuous Mode

float enemySpeed_timer = 0;

float enemy_movement_timer = 0;

size_t movementCounter_1p = 0; // Counter for movements
size_t movementCounter_2p = 0; // Counter for movements

size_t _1p_points = 0;
size_t _2p_points = 0;

sf::Clock powerUpClock;
bool isPaused = false;

bool disablePlayer2Controls = false;
bool disablePlayer1Controls = false;

// bool streak;

int elapsedTime = 0; // Elapsed time for the game

const int M = 35;
const int N = 55;

int grid[M][N] = {0};
int ts = 18;

size_t score_1p = 0;
size_t score_2p = 0;

struct Enemy
{
    int x, y, dx, dy;
    movement_type movement = LINEAR; // Default movement type

    Enemy()
    {
        x = y = 300;
        dx = 4 - rand() % 11;
        dy = 4 - rand() % 11;
    }

    // Default movement type

    void move()
    {
        if (isPaused) // If the game is paused, do not update the enemy's position
            return;

        if (movement == LINEAR)
        {
            x += dx;

            if (grid[y / ts][x / ts] == 1) // If the enemy hits the wall
            {
                dx = -dx;
                x += dx;
            }
            y += dy;
            if (grid[y / ts][x / ts] == 1) // If the enemy hits the wall
            {
                dy = -dy;
                y += dy;
            }
        }
        else if (movement == ZIG_ZAG)
        {
            zig_zag();
        }
        else if (movement == CIRCULAR)
        {
            circular();
        }
    }

    void zig_zag()
    {
        static int frameCount = 0;
        static bool goingRight = true;
        static int verticalDir = 1; // 1 for down, -1 for up

        const int zigzagInterval = 50;
        frameCount++;

        if (frameCount >= zigzagInterval)
        {
            goingRight = !goingRight;
            frameCount = 0;
        }

        int move_dx;

        if (goingRight)
        {
            move_dx = 5; // Move right
        }
        else
        {
            move_dx = -5; // Move left
        }

        int move_dy = verticalDir * 2;

        // Add jitter for randomness
        move_dx += rand() % 3 - 1; // Randomly adjust the x movement
        move_dy += rand() % 3 - 1; // Randomly adjust the y movement

        int new_x = x + move_dx;
        int new_y = y + move_dy;

        // Check if the new position is within bounds
        if (new_x >= 0 && new_x < N * ts && new_y >= 0 && new_y < M * ts)
        {
            // Move the enemy only if within bounds
            if (grid[new_y / ts][new_x / ts] != 1) // correct this for the 2p
            {
                x = new_x;
                y = new_y;
            }
            else
            {
                // If it hits a wall, reverse direction
                goingRight = !goingRight;
                verticalDir *= -1;
                frameCount = 0;
            }
        }
        else
        {
            // If out of bounds, reverse direction but don't move
            goingRight = !goingRight;
            verticalDir *= -1;
        }
    }

    void circular()
    {
        static float angle = 0.0f;
        angle += 0.05f;

        int radius = 60;
        int cx = 300, cy = 300; // Circle center

        x = cx + static_cast<int>(radius * cos(angle)); //  x = h + r cos(t)
        y = cy + static_cast<int>(radius * sin(angle)); //  y = k + r sin(t)

        // Don't reverse dx/dy here — position is directly set
    }
};

void drop(int y, int x)
{

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

    sGameover.setPosition(300, 300); // The origin (0, 0) is at the top-left corner of the window

    sEnemy.setOrigin(20, 20); // Set the origin to the center of the sprite which is 20x20 pixels(assuming the sprite is 40x40 pixels)

    // The sprite rotates around its origin point.
    // If the origin is at the center of the sprite, the rotation looks natural.
    // If the origin is at the top-left corner (default), the sprite rotates around that corner, which can look strange.

    int enemyCount = 2;
    Enemy a[18];

    bool Game = true;
    int x = 0, y = 0, dx = 0, dy = 0;
    int x_2 = N - 1, y_2 = 0, dx_2 = 0, dy_2 = 0;

    bool power_up_1p = false;
    bool power_up_2p = false;

    float timer = 0, delay = 0.07;
    sf::Clock clock;

    construct_boundry(); // for making the boundry constructed

    sf::Texture menu_texture;
    menu_texture.loadFromFile("images/menu_xonix_pic.png");
    sf::Sprite menu_text(menu_texture);
    menu_text.setPosition(50, 20);
    menu_text.setScale(0.4f, 0.4f);

    sf::Texture menu_controller_texture;
    menu_controller_texture.loadFromFile("images/controller.jpg");
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
    mode_button.setPosition(50, 390);
    mode_button.setScale(1.4f, 1.4f);

    sf::Texture stop_buton_texture;
    stop_buton_texture.loadFromFile("images/stop_button.png");
    sf::Sprite stop_button(stop_buton_texture);
    stop_button.setPosition(50, 500);
    stop_button.setScale(0.5, 0.5f);

    sf::Texture _1p_buton_texture;
    _1p_buton_texture.loadFromFile("images/1p.png");
    sf::Sprite _1p_button(_1p_buton_texture);
    _1p_button.setPosition(50, 70);
    _1p_button.setScale(0.5, 0.5f);

    sf::Texture _2p_buton_texture;
    _2p_buton_texture.loadFromFile("images/2p.png");
    sf::Sprite _2p_button(_2p_buton_texture);
    _2p_button.setPosition(670, 70);
    _2p_button.setScale(0.5, 0.5f);

    sf::Texture easy_buton_texture;
    easy_buton_texture.loadFromFile("images/easy.png");
    sf::Sprite easy_button(easy_buton_texture);
    easy_button.setPosition(350, 150);
    easy_button.setScale(1.2, 1.2f);

    sf::Texture medium_buton_texture;
    medium_buton_texture.loadFromFile("images/medium.png");
    sf::Sprite medium_button(medium_buton_texture);
    medium_button.setPosition(350, 250);
    medium_button.setScale(1.2, 1.2f);

    sf::Texture hard_buton_texture;
    hard_buton_texture.loadFromFile("images/hard.png");
    sf::Sprite hard_button(hard_buton_texture);
    hard_button.setPosition(350, 350);
    hard_button.setScale(1.2, 1.2f);

    sf::Texture ContinuousMode;
    ContinuousMode.loadFromFile("images/ContinuousMode.png");
    sf::Sprite continuous_button(ContinuousMode);
    continuous_button.setPosition(350, 50);
    continuous_button.setScale(1.7, 1.7f);

    sf::Texture back_button_texture;
    back_button_texture.loadFromFile("images/back.png");
    sf::Sprite back_button_mode(back_button_texture);
    back_button_mode.setPosition(350, 450);
    back_button_mode.setScale(1.2, 1.2f);

    sf::Sprite back_button_level(back_button_texture);
    back_button_level.setPosition(350, 500);
    back_button_level.setScale(1.2, 1.2f);

    sf::Texture levle_one_texture;
    levle_one_texture.loadFromFile("images/1.png");
    sf::Sprite level_one(levle_one_texture);
    level_one.setPosition(50, 200);
    level_one.setScale(0.5, 0.5f);

    sf::Texture level_two_texture;
    level_two_texture.loadFromFile("images/2.png");
    sf::Sprite level_two(level_two_texture);
    level_two.setPosition(400, 200);
    level_two.setScale(0.5, 0.5f);

    sf::Texture level_three_texture;
    level_three_texture.loadFromFile("images/3.png");
    sf::Sprite level_three(level_three_texture);
    level_three.setPosition(700, 200);
    level_three.setScale(0.5, 0.5f);

    sf::Font font;
    if (!font.loadFromFile("images/Arial.ttf"))
    {
        std::cerr << "Error loading font!" << std::endl;
        return -1;
    }

    sf::Text text1("select level", font, 50);
    text1.setFillColor(sf::Color::Black);
    text1.setPosition(50, 50);
    text1.setStyle(sf::Text::Bold);

    sf::Text movement_CounterText_1p;
    movement_CounterText_1p.setFont(font);                      // Set the font
    movement_CounterText_1p.setCharacterSize(20);               // Set the text size
    movement_CounterText_1p.setFillColor(sf::Color(255, 0, 0)); // Set the text color
    movement_CounterText_1p.setPosition(10, 10);                // Set the position on the screen
    movement_CounterText_1p.setString("0");

    sf::Text point_1p;
    point_1p.setFont(font);                      // Set the font
    point_1p.setCharacterSize(30);               // Set the text size
    point_1p.setFillColor(sf::Color(255, 0, 0)); // Set the text color
    point_1p.setPosition(10, 30);                // Set the position on the screen
    point_1p.setString("0");

    sf::Text movement_CounterText_2p;
    movement_CounterText_2p.setFont(font);                      // Set the font
    movement_CounterText_2p.setCharacterSize(20);               // Set the text size
    movement_CounterText_2p.setFillColor(sf::Color(0, 128, 0)); // Set the text color
    movement_CounterText_2p.setPosition(780, 10);               // Set the position on the screen
    movement_CounterText_2p.setString("0");

    sf::Text point_2p;
    point_2p.setFont(font);                      // Set the font
    point_2p.setCharacterSize(30);               // Set the text size
    point_2p.setFillColor(sf::Color(0, 128, 0)); // Set the text color
    point_2p.setPosition(780, 30);               // Set the position on the screen
    point_2p.setString("0");

    sf::Clock playModeClock; // Clock to track elapsed time for PLAY mode
    sf::Text playElapsedTimeText;
    playElapsedTimeText.setFont(font);                  // Set the font
    playElapsedTimeText.setCharacterSize(20);           // Set the text size
    playElapsedTimeText.setFillColor(sf::Color::White); // Set the text color
    playElapsedTimeText.setPosition(480, 10);           // Set the position on the screen
    playElapsedTimeText.setString("0");

    sf::SoundBuffer buffer_button;
    if (!buffer_button.loadFromFile("sounds/button_click.wav"))
    {
        std::cerr << "Error loading button_sound!" << std::endl;
        return -1;
    }

    sf::Sound button_sound;
    button_sound.setBuffer(buffer_button);
    button_sound.setVolume(100);
    button_sound.setPitch(1.0f);

    sf::SoundBuffer buffer_gameover;
    if (!buffer_gameover.loadFromFile("sounds/gameover.wav"))
    {
        std::cerr << "Error loading gameover_sound!" << std::endl;
        return -1;
    }

    sf::Sound gameover_sound;
    gameover_sound.setBuffer(buffer_gameover);
    gameover_sound.setVolume(100);
    gameover_sound.setPitch(1.0f);

    while (window.isOpen())
    {
        float time = clock.getElapsedTime().asSeconds();
        clock.restart();
        timer += time;

        sf::Event e;

        while (window.pollEvent(e))
        {
            if (e.type == sf::Event::Closed)
                window.close();

            if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape)
            {
                if (gameState == MENU)
                    window.close();
                else
                {
                    for (int i = 1; i < M - 1; i++)
                        for (int j = 1; j < N - 1; j++)
                            grid[i][j] = 0;

                    x = y = 0;
                    x_2 = N - 1;
                    y_2 = 0;
                    movementCounter_1p = movementCounter_2p = 0;
                    Game = true;
                }
            }

            if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left)
            {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                if (gameState == MENU)
                {
                    if (start_button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        gameState = PLAY;
                    }
                    else if (level_button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        gameState = LEVELS;
                    }
                    else if (mode_button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        gameState = MODES;
                    }
                    else if (stop_button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        window.close();
                    }

                    if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left)
                    {
                        sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                        if (start_button.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
                        {
                            button_sound.play();
                            gameState = PLAY;
                        }
                        else if (level_button.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
                        {
                            button_sound.play();
                            gameState = LEVELS;
                        }
                        else if (mode_button.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
                        {
                            std::cout << "Mode button clicked!" << std::endl;
                            button_sound.play();
                            gameState = MODES;
                        }
                    }
                }
                else if (gameState == MODES)
                {
                    if (_1p_button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        gameMode = SINGLE_PLAYER;
                        button_sound.play();
                        std::cout << "1P mode selected!" << std::endl;
                    }
                    else if (_2p_button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        gameMode = TWO_PLAYER;
                        button_sound.play();
                        std::cout << "2P mode selected!" << std::endl;
                    }
                    else if (back_button_mode.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        gameState = MENU;
                    }
                    else if (easy_button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        gameDifficulty = EASY;
                        enemyCount = 2; // Set enemy count for easy mode
                        button_sound.play();
                        std::cout << "Easy mode selected!" << std::endl;
                    }
                    else if (medium_button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        gameDifficulty = MEDIUM;
                        enemyCount = 4; // Set enemy count for medium mode
                        button_sound.play();
                        std::cout << "Medium mode selected!" << std::endl;
                    }
                    else if (hard_button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        gameDifficulty = HARD;
                        enemyCount = 6; // Set enemy count for hard mode
                        button_sound.play();
                        std::cout << "Hard mode selected!" << std::endl;
                    }
                    else if (continuous_button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        gameDifficulty = continuous;
                        button_sound.play();
                        std::cout << "Continuous mode selected!" << std::endl;
                    }
                }
                else if (gameState == LEVELS)
                {
                    if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left)
                    {
                        sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                        if (level_one.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
                        {
                            gameLevel = LEVEL_ONE;
                            gameState = MENU;
                            button_sound.play();
                            std::cout << "Level 1 selected!" << std::endl;
                        }
                        else if (level_two.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
                        {
                            gameLevel = LEVEL_TWO;
                            gameState = MENU;
                            button_sound.play();
                            std::cout << "Level 2 selected!" << std::endl;
                        }
                        else if (level_three.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
                        {
                            gameLevel = LEVEL_THREE;
                            gameState = MENU;
                            button_sound.play();
                            std::cout << "Level 3 selected!" << std::endl;
                        }
                        else if (back_button_level.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
                        {
                            button_sound.play();
                            gameState = MENU;
                        }
                    }
                }
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

            window.draw(_1p_button);
            window.draw(_2p_button);

            window.draw(continuous_button);

            window.draw(easy_button);
            window.draw(medium_button);
            window.draw(hard_button);

            window.draw(back_button_mode);

            window.display();
            continue;
        }

        if (gameState == LEVELS)
        {
            window.clear(sf::Color(50, 158, 168));

            window.draw(level_one);
            window.draw(level_two);
            window.draw(level_three);
            window.draw(text1);

            window.draw(back_button_level);

            window.display();
            continue;
        }

        if (gameState == PLAY)
        {

            // window.clear();

            if (gameDifficulty == continuous)
            {
                continuousModeTimer += time; // Increment the timer for Continuous Mode

                if (continuousModeTimer >= 20.0f) // Check if 20 seconds have passed
                {
                    if (enemyCount + 2 <= 18) // Limit the maximum number of enemies to 10
                    {
                        enemyCount += 2; // Add 2 more enemies
                        std::cout << "Added 2 more enemies! Total enemies: " << enemyCount << std::endl;
                    }
                    continuousModeTimer = 0; // Reset the timer
                }
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && !disablePlayer1Controls)
            {
                dx = -1;
                dy = 0;
            };
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && !disablePlayer1Controls)
            {
                dx = 1;
                dy = 0;
            };
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && !disablePlayer1Controls)
            {
                dx = 0;
                dy = -1;
            };
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && !disablePlayer1Controls)
            {
                dx = 0;
                dy = 1;
            };

            int temp_dx_2, temp_dy_2;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) && !isPaused && !disablePlayer1Controls)
            {
                if (power_up_1p == false)
                {
                    power_up_1p = true;
                    isPaused = true;
                    disablePlayer2Controls = true; // Disable Player 2 controls
                    std::cout << "Power-up activated! Enemies and Player 2 stopped for 3 seconds." << std::endl;

                    // Stop all enemies and Player 2
                    for (int i = 0; i < enemyCount; i++)
                    {
                        a[i].dx = 0;
                        a[i].dy = 0;
                    }
                    for (int i = 0; i < M - 1; i++)
                    {
                        for (int j = 0; j < N - 1; j++)
                        {
                            if (grid[i][j] == 3)
                            {
                                grid[i][j] = 1;
                            }
                        }
                    }
                    temp_dx_2 = dx_2; // Store Player 2's current movement
                    temp_dy_2 = dy_2; // Store Player 2's current movement
                    dx_2 = 0;
                    dy_2 = 0;

                    powerUpClock.restart(); // Start the timer
                }
            }

            // Check if the 3-second pause is over
            if (isPaused && powerUpClock.getElapsedTime().asSeconds() >= 3.0f && power_up_1p)
            {
                // Resume enemy and Player 2 movement
                for (int i = 0; i < enemyCount; i++)
                {
                    a[i].dx = 4 - rand() % 11; // Restore random movement for enemies
                    a[i].dy = 4 - rand() % 11;
                }
                dx_2 = temp_dx_2; // Reset Player 2's movement to stationary
                dy_2 = temp_dy_2; // Reset Player 2's movement to stationary

                isPaused = false;               // End the pause
                power_up_1p = false;            // Reset the power-up state
                disablePlayer2Controls = false; // Enable Player 2 controls
            }

            if (gameMode == TWO_PLAYER)
            {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && !disablePlayer2Controls)
                {
                    dx_2 = -1;
                    dy_2 = 0;
                };
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && !disablePlayer2Controls)
                {
                    dx_2 = 1;
                    dy_2 = 0;
                };
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && !disablePlayer2Controls)
                {
                    dx_2 = 0;
                    dy_2 = -1;
                };
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && !disablePlayer2Controls)
                {
                    dx_2 = 0;
                    dy_2 = 1;
                };
                int temp_dx, temp_dy;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && !isPaused && !disablePlayer2Controls)
                {
                    if (power_up_2p == false)
                    {
                        power_up_2p = true;
                        isPaused = true;
                        disablePlayer1Controls = true; // Disable Player 1 controls
                        std::cout << "Power-up activated! Enemies and Player 1 stopped for 3 seconds." << std::endl;

                        // Stop all enemies and Player 1
                        for (int i = 0; i < enemyCount; i++)
                        {
                            a[i].dx = 0;
                            a[i].dy = 0;
                        }
                        for (int i = 0; i < M - 1; i++)
                        {
                            for (int j = 0; j < N - 1; j++)
                            {
                                if (grid[i][j] == 2)
                                {
                                    grid[i][j] = 1;
                                }
                            }
                        }
                        temp_dx = dx; // Store Player 1's current movement
                        temp_dy = dy; // Store Player 1's current movement
                        dx = 0;
                        dy = 0;

                        powerUpClock.restart(); // Start the timer
                    }
                }

                if (isPaused && powerUpClock.getElapsedTime().asSeconds() >= 3.0f && power_up_2p)
                {
                    // Resume enemy and Player 1 movement
                    for (int i = 0; i < enemyCount; i++)
                    {
                        a[i].dx = 4 - rand() % 11; // Restore random movement for enemies
                        a[i].dy = 4 - rand() % 11;
                    }
                    dx = temp_dx; // Reset Player 1's movement to stationary
                    dy = temp_dy; // Reset Player 1's movement to stationary

                    isPaused = false;               // End the pause
                    power_up_2p = false;            // Reset the power-up state
                    disablePlayer1Controls = false; // Enable Player 1 controls
                }
            }

            if (!Game)
                continue;

            if (timer > delay) // Check if enough time has passed to update the player's movement
            {
                int prevX = x;
                int prevY = y;
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

                if ((x != prevX || y != prevY) && grid[y][x] != 1)
                {
                    movementCounter_1p++;
                    std::cout << "Movement Counter: " << movementCounter_1p << std::endl;
                }

                // check if the player has hit a wall or an enemy
                if (grid[y][x] == 2 || grid[y][x] == 3) // Collision with wall or Player 2's trail
                    Game = false;
                // if the player hits a wall, set the position back to the last valid position
                if (grid[y][x] == 0)
                    grid[y][x] = 2;

                timer = 0;

                if (gameMode == TWO_PLAYER)
                {
                    int prevX_2 = x_2;
                    int prevY_2 = y_2;
                    // move the player
                    x_2 += dx_2;
                    y_2 += dy_2;

                    // detect collision with the grid boundaries
                    if (x_2 < 0)
                        x_2 = 0;
                    if (x_2 > N - 1)
                        x_2 = N - 1;
                    if (y_2 < 0)
                        y_2 = 0;
                    if (y_2 > M - 1)
                        y_2 = M - 1;

                    if ((x_2 != prevX_2 || y_2 != prevY_2) && grid[y_2][x_2] != 1)
                    {
                        movementCounter_2p++;
                        std::cout << "Movement Counter 2p: " << movementCounter_2p << std::endl;
                    }

                    if (grid[y_2][x_2] == 3 || grid[y_2][x_2] == 2) // Collision with wall or Player 1's trail
                    {
                        Game = false;
                    }

                    if (grid[y_2][x_2] == 0) // Only mark empty cells
                    {
                        std::cout << "Player 2's movement: " << x_2 << ", " << y_2 << std::endl;
                        grid[y_2][x_2] = 3; // Mark Player 2's trail
                    }
                }
            }

            for (int i = 0; i < enemyCount; i++)
            {
                a[i].move();
            }

            enemySpeed_timer += time;

            if (enemySpeed_timer >= 20.0f)
            {
                for (int i = 0; i < enemyCount; i++)
                {
                    if (a[i].dx < 0)
                        a[i].dx -= 1; // Increase negative speed
                    else
                        a[i].dx += 1; // Increase positive speed

                    if (a[i].dy < 0)
                        a[i].dy -= 1; // Increase negative speed
                    else
                        a[i].dy += 1; // Increase positive speed
                }

                std::cout << "Enemy speed increased!" << std::endl;
                enemySpeed_timer = 0;
            }

            enemy_movement_timer += time;

            if (enemy_movement_timer >= 5.0f) // After 30 seconds
            {
                for (int i = 0; i < enemyCount / 2; i++) // Switch movement for half the enemies
                {
                    if (i % 2 == 0)
                        a[i].movement = ZIG_ZAG; // Assign zig-zag movement
                    else
                        a[i].movement = CIRCULAR; // Assign circular movement
                }

                std::cout << "Enemies switched to geometric patterns!" << std::endl;
                enemy_movement_timer = 0; // Reset the timer
            }

            if (grid[y][x] == 1)
            {
                dx = dy = 0;

                for (int i = 0; i < enemyCount; i++)
                    drop(a[i].y / ts, a[i].x / ts);

                for (int i = 0; i < M; i++)
                    for (int j = 0; j < N; j++)
                        if (grid[i][j] == -1)
                        {
                            grid[i][j] = 0;
                        }
                        else if (grid[i][j] == 2 || grid[i][j] == 0)
                        {
                            grid[i][j] = 1; // this will replace the 2 with the 1 in the grid
                        }
            }

            if (gameMode == TWO_PLAYER)
            {

                if (grid[y_2][x_2] == 1)
                {
                    dx_2 = dy_2 = 0;

                    for (int i = 0; i < enemyCount; i++)
                        drop(a[i].y / ts, a[i].x / ts);

                    for (int i = 0; i < M; i++)
                        for (int j = 0; j < N; j++)
                            if (grid[i][j] == -1)
                            {
                                grid[i][j] = 0;
                            }
                            else if (grid[i][j] == 3 || grid[i][j] == 0)
                            {
                                grid[i][j] = 1;
                            }
                }
            }

            for (int i = 0; i < enemyCount; i++)
            {
                if (grid[a[i].y / ts][a[i].x / ts] == 2 || grid[a[i].y / ts][a[i].x / ts] == 3)
                {
                    Game = false;
                }
            }

            /////////draw//////////
            window.clear(); /// clear the window with black color so that the previous frame is not visible

            for (int i = 0; i < M; i++)
            {
                for (int j = 0; j < N; j++)
                {
                    if (grid[i][j] == 0)
                        continue;
                    if (grid[i][j] == 1)
                    {
                        sTile.setTextureRect(sf::IntRect(0, 0, ts, ts));
                    }
                    if (grid[i][j] == 2)
                    {
                        sTile.setTextureRect(sf::IntRect(54, 0, ts, ts));
                    }
                    if (grid[i][j] == 3)
                    {
                        sTile.setTextureRect(sf::IntRect(54, 0, ts, ts)); // Use a different texture for Player 2's trail
                    }
                    sTile.setPosition(j * ts, i * ts);
                    window.draw(sTile);
                }
            }

            sTile.setTextureRect(sf::IntRect(36, 0, ts, ts)); // this is the sprite for the player
            sTile.setPosition(x * ts, y * ts);
            window.draw(sTile);

            if (gameMode == TWO_PLAYER)
            {

                sTile.setTextureRect(sf::IntRect(109, 0, ts, ts)); // this is the sprite for the player 2
                sTile.setPosition(x_2 * ts, y_2 * ts);
                window.draw(sTile);
            }

            sEnemy.rotate(20); // rotate the enemy sprite on its own axis set by  sEnemy.setOrigin(20, 20);

            for (int i = 0; i < enemyCount; i++) // draw the enemies
            {
                sEnemy.setPosition(a[i].x, a[i].y);
                window.draw(sEnemy);
            }

            if (!Game)
            {
                gameover_sound.play();
                window.draw(sGameover);
            }

            movement_CounterText_1p.setString("movement counter:" + std::to_string(movementCounter_1p)); // Update the text with the current movement counter
            point_1p.setString("X:" + std::to_string(movementCounter_1p));
            window.draw(movement_CounterText_1p);
            window.draw(point_1p);

            if (gameMode == TWO_PLAYER)
            {
                movement_CounterText_2p.setString("movement counter:" + std::to_string(movementCounter_2p));
                point_2p.setString("X" + std::to_string(movementCounter_2p));
                window.draw(movement_CounterText_2p);
                window.draw(point_2p);
            }

            if (playModeClock.getElapsedTime().asSeconds() == 0) // Restart clock only once
            {
                playModeClock.restart();
            }

            float playElapsedTime = playModeClock.getElapsedTime().asSeconds();
            playElapsedTimeText.setString("Time: " + std::to_string(static_cast<int>(playElapsedTime)) + "s");

            window.draw(playElapsedTimeText);

            window.display();
        }
    }

    return 0;
}
