#include <SFML/Graphics.hpp>
#include <time.h>
#include <cmath>
#include <cstdlib>
#include <string>
#include <cstring>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <fstream>
#include <cmath>

// Define game states
enum GameState
{
    MENU,
    PLAY,
    GAMEOVER_MENU,
    LEVELS,
    MODES, // Modes menu
    highscore
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
difficulty gameDifficulty = EASY;
GameLevel gameLevel = LEVEL_ONE;
GameMode gameMode = SINGLE_PLAYER;

float continuousModeTimer = 0;
float enemySpeed_timer = 0;
float enemy_movement_timer = 0;

int tiles_covered_1p = 0;
int tiles_covered_2p = 0;

int movementCounter_1p = 0;
int movementCounter_2p = 0;

int _1p_points = 0;
int _2p_points = 0;

int reward_counter_1p = 1;
int reward_counter_2p = 1;

sf::Clock powerUpClock;

sf::Clock playModeClock;
bool playModeClock_clockRunning = false;

bool isPaused = false;
bool disablePlayer2Controls = false;
bool disablePlayer1Controls = false;

bool p1_dead = false;
bool p2_dead = false;

// bool streak;

int elapsedTime = 0;
float playElapsedTime = 0;

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

struct ScoreEntry
{
    int score;
    int timeTaken;
};

void loadScoresFromFile(ScoreEntry scores[], int &count, const std::string &filename = "scores.txt")
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "Error: Could not open file " << filename << " for reading!" << std::endl;
        count = 0; // No scores loaded
        return;
    }

    count = 0;
    while (file >> scores[count].score >> scores[count].timeTaken)
    {
        count++;
        if (count >= 5)
        {
            break;
        }
    }

    file.close();
}

void updateScoreboard(ScoreEntry scores[], int &count, int newScore, int newTime)
{
    // Check if the new score qualifies for the top 5
    if (count < 5 || newScore > scores[count - 1].score)
    {
        if (count < 5)
        {
            count++;
        }

        int i = count - 1;
        while (i > 0 && scores[i - 1].score < newScore)
        {
            scores[i] = scores[i - 1]; // Shift scores down
            i--;
        }

        // Insert the new score
        scores[i].score = newScore;
        scores[i].timeTaken = newTime;
    }
}

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

void dullSprite(sf::Sprite &sprite)
{
    sprite.setColor(sf::Color(128, 128, 128, 255));
}

void restoreSprite(sf::Sprite &sprite)
{
    sprite.setColor(sf::Color::White); // Restore by resetting to original color
}

void reward_p1()
{
    if (tiles_covered_1p >= 10 && reward_counter_1p <= 3)
    {
        // Double points for capturing more than 10 tiles
        _1p_points += tiles_covered_1p * 2;
        reward_counter_1p++;
        std::cout << "Reward 1: " << reward_counter_1p << " (×2 points)" << std::endl;
        tiles_covered_1p = 0; // Reset tiles covered
    }
    else if (tiles_covered_1p >= 5 && reward_counter_1p > 3 && reward_counter_1p <= 5)
    {
        // Double points for capturing more than 5 tiles after 3 rewards
        _1p_points += tiles_covered_1p * 2;
        reward_counter_1p++;
        std::cout << "Reward Counter2: " << reward_counter_1p << " (×2 points, reduced threshold)" << std::endl;
        tiles_covered_1p = 0; // Reset tiles covered
    }
    else if (tiles_covered_1p >= 5 && reward_counter_1p > 5)
    {
        // Quadruple points for capturing more than 5 tiles after 5 rewards
        _1p_points += tiles_covered_1p * 4;
        reward_counter_1p++;
        std::cout << "Reward Counter3: " << reward_counter_1p << " (×4 points)" << std::endl;
        tiles_covered_1p = 0; // Reset tiles covered
    }
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

    sf::Texture background_texture;
    if (!background_texture.loadFromFile("images/menu_bg.png"))
    {
        std::cerr << "Error loading menu_bg.png!" << std::endl;
        return -1;
    }

    sf::Sprite menu_background(background_texture);

    menu_background.setScale(
        static_cast<float>(N * ts) / background_texture.getSize().x, // Scale to fit the window width
        static_cast<float>(M * ts) / background_texture.getSize().y  // Scale to fit the window height
    );

    sf::Texture menu_texture;
    menu_texture.loadFromFile("images/menu_xonix_pic.png");
    sf::Sprite menu_text(menu_texture);
    menu_text.setPosition(50, 20);
    menu_text.setScale(0.4f, 0.4f);

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

    sf::Sprite back_button_score(back_button_texture);
    back_button_score.setPosition(350, 500);
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
    movement_CounterText_1p.setStyle(sf::Text::Bold);

    sf::Text point_1p;
    point_1p.setFont(font);                      // Set the font
    point_1p.setCharacterSize(25);               // Set the text size
    point_1p.setFillColor(sf::Color(255, 0, 0)); // Set the text color
    point_1p.setPosition(10, 30);                // Set the position on the screen
    point_1p.setString("0");
    point_1p.setStyle(sf::Text::Bold);

    sf::Text score_x1p;
    score_x1p.setFont(font);                      // Set the font
    score_x1p.setCharacterSize(25);               // Set the text size
    score_x1p.setFillColor(sf::Color(255, 0, 0)); // Set the text color
    score_x1p.setPosition(10, 50);
    score_x1p.setString("0");
    score_x1p.setStyle(sf::Text::Bold);

    sf::Text movement_CounterText_2p;
    movement_CounterText_2p.setFont(font);                        // Set the font
    movement_CounterText_2p.setCharacterSize(20);                 // Set the text size
    movement_CounterText_2p.setFillColor(sf::Color(255, 165, 0)); // Set the text color
    movement_CounterText_2p.setPosition(760, 10);                 // Set the position on the screen
    movement_CounterText_2p.setString("0");
    movement_CounterText_2p.setStyle(sf::Text::Bold);

    sf::Text point_2p;
    point_2p.setFont(font);                        // Set the font
    point_2p.setCharacterSize(25);                 // Set the text size
    point_2p.setFillColor(sf::Color(255, 165, 0)); // Set the text color
    point_2p.setPosition(780, 30);                 // Set the position on the screen
    point_2p.setString("0");
    point_2p.setStyle(sf::Text::Bold);

    // Clock to track elapsed time for PLAY mode
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

    sf::Texture highscore_texture;
    highscore_texture.loadFromFile("images/high_score.png");
    sf::Sprite highscore_button(highscore_texture);
    highscore_button.setPosition(800, 500);
    highscore_button.setScale(0.7, 0.7f);

    sf::SoundBuffer buffer_power_up;
    if (!buffer_power_up.loadFromFile("sounds/power_up.wav"))
    {
        std::cerr << "Error loading power_up_sound!" << std::endl;
        return -1;
    }

    sf::Sound power_up_sound;
    power_up_sound.setBuffer(buffer_power_up);
    power_up_sound.setVolume(100);
    power_up_sound.setPitch(1.0f);

    sf::Texture power_up_texture;
    if (!power_up_texture.loadFromFile("images/power_up_effect.png"))
    {
        std::cerr << "Error loading power_up_effect.png!" << std::endl;
        return -1;
    }
    sf::Sprite power_up(power_up_texture);

    sf::SoundBuffer buffer_p1_terminated;
    if (!buffer_p1_terminated.loadFromFile("sounds/p1_terminated.wav"))
    {
        std::cerr << "Error loading p1_terminated_sound!" << std::endl;
        return -1;
    }

    sf::Sound p1_terminated_sound;
    p1_terminated_sound.setBuffer(buffer_p1_terminated);
    p1_terminated_sound.setVolume(100);
    p1_terminated_sound.setPitch(1.0f);

    sf::SoundBuffer buffer_p2_terminated;
    if (!buffer_p2_terminated.loadFromFile("sounds/p2_terminated.wav"))
    {
        std::cerr << "Error loading p2_terminated_sound!" << std::endl;
        return -1;
    }
    sf::Sound p2_terminated_sound;
    p2_terminated_sound.setBuffer(buffer_p2_terminated);
    p2_terminated_sound.setVolume(100);
    p2_terminated_sound.setPitch(1.0f);

    sf::Texture gameover_Menu_restart_texture;
    gameover_Menu_restart_texture.loadFromFile("images/restart.png");
    sf::Sprite gameover_Menu_restart_button(gameover_Menu_restart_texture);
    gameover_Menu_restart_button.setPosition(50, 60);
    gameover_Menu_restart_button.setScale(0.36f, 0.36f);

    sf::Texture gameover_Menu_resume_texture;
    gameover_Menu_resume_texture.loadFromFile("images/resume.png");
    sf::Sprite gameover_Menu_resume_button(gameover_Menu_resume_texture);
    gameover_Menu_resume_button.setPosition(55, 200);
    gameover_Menu_resume_button.setScale(0.85f, 0.85f);

    sf::Sprite gameover_menu_stop(stop_buton_texture);
    gameover_menu_stop.setPosition(50, 450);
    gameover_menu_stop.setScale(0.5, 0.5f);

    sf::Texture main_menu_texture;
    main_menu_texture.loadFromFile("images/main_menu.png");
    sf::Sprite main_menu(main_menu_texture);
    main_menu.setPosition(30, 210);
    main_menu.setScale(0.8f, 0.8f);

    sf::SoundBuffer buffer_movement;
    if (!buffer_movement.loadFromFile("sounds/movement_effect.wav"))
    {
        std::cerr << "Error loading movement_sound!" << std::endl;
        return -1;
    }

    sf::Sound movement_sound;
    movement_sound.setBuffer(buffer_movement);
    movement_sound.setVolume(75);
    movement_sound.setPitch(3.0f);

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
                if (gameState == PLAY && !power_up_1p && !power_up_2p)
                {
                    gameState = GAMEOVER_MENU;
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
                            button_sound.play();
                            gameState = MODES;
                        }
                        else if (highscore_button.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
                        {
                            button_sound.play();
                            gameState = highscore;
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
                else if (gameState == highscore)
                {
                    if (back_button_score.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        gameState = MENU;
                    }
                }
                else if (gameState == GAMEOVER_MENU)
                {
                    if (gameover_Menu_restart_button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();

                        continuousModeTimer = 0;
                        enemySpeed_timer = 0;
                        enemy_movement_timer = 0;

                        _1p_points = 0;
                        _2p_points = 0;

                        isPaused = false;
                        disablePlayer2Controls = false;
                        disablePlayer1Controls = false;

                        p1_dead = false;
                        p2_dead = false;

                        playElapsedTime = 0;
                        playModeClock.restart();

                        Game = true;

                        for (int i = 1; i < M - 1; i++)
                        {
                            for (int j = 1; j < N - 1; j++)
                            {
                                grid[i][j] = 0; // Clear the grid
                            }
                        }

                        for (int i = 0; i < enemyCount; i++)
                        {
                            x = y = 300;
                            a[i].dx = 4 - rand() % 11;
                            a[i].dy = 4 - rand() % 11;
                        }

                        x = 0, y = 0, dx = 0, dy = 0;
                        x_2 = N - 1, y_2 = 0, dx_2 = 0, dy_2 = 0;

                        // bool streak;

                        elapsedTime = 0; // Elapsed time for the game
                        gameState = PLAY;
                    }
                    else if (gameover_menu_stop.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        window.close();
                    }
                    else if (gameover_Menu_resume_button.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        Game = true;
                        gameState = PLAY;
                    }
                    else if (main_menu.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();

                        continuousModeTimer = 0;
                        enemySpeed_timer = 0;
                        enemy_movement_timer = 0;

                        _1p_points = 0;
                        _2p_points = 0;

                        isPaused = false;
                        disablePlayer2Controls = false;
                        disablePlayer1Controls = false;

                        p1_dead = false;
                        p2_dead = false;

                        Game = true;

                        playElapsedTime = 0;

                        // playModeClock.restart();
                        playModeClock_clockRunning = false;

                        for (int i = 1; i < M - 1; i++)
                        {
                            for (int j = 1; j < N - 1; j++)
                            {
                                grid[i][j] = 0;
                            }
                        }

                        for (int i = 0; i < enemyCount; i++)
                        {
                            x = y = 300;
                            a[i].dx = 4 - rand() % 11;
                            a[i].dy = 4 - rand() % 11;
                        }

                        x = 0, y = 0, dx = 0, dy = 0;
                        x_2 = N - 1, y_2 = 0, dx_2 = 0, dy_2 = 0;

                        // bool streak;

                        elapsedTime = 0; // Elapsed time for the game
                        gameState = MENU;
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
            window.draw(menu_background);

            window.draw(menu_text);
            window.draw(start_button);
            window.draw(level_button);
            window.draw(mode_button);
            window.draw(stop_button);
            window.draw(highscore_button);

            window.display();
            continue;
        }

        if (gameState == highscore)
        {
            ScoreEntry high_scores[5];
            int count = 0;

            // Load high scores from the file
            loadScoresFromFile(high_scores, count, "scores.txt");

            sf::Text highscore_texts[5]; // Array to hold 5 highscore texts

            for (int i = 0; i < 5; i++)
            {
                highscore_texts[i].setFont(font);                    // Set the font
                highscore_texts[i].setCharacterSize(40);             // Set the text size
                highscore_texts[i].setFillColor(sf::Color(0, 0, 0)); // Set the text color
                highscore_texts[i].setPosition(330, 100 + i * 40);   // Set the position for each text (vertically spaced)

                if (i < count)
                {
                    // Use the actual score and time from the high_scores array
                    highscore_texts[i].setString("Highscore " + std::to_string(i + 1) + ": " + std::to_string(high_scores[i].score) + " (" + std::to_string(high_scores[i].timeTaken) + "s)");
                }
                else
                {
                    highscore_texts[i].setString("Highscore " + std::to_string(i + 1) + ": ---");
                }
            }

            window.clear();
            window.draw(menu_background);
            window.draw(back_button_score);

            for (int i = 0; i < 5; i++)
            {
                window.draw(highscore_texts[i]);
            }

            window.display();
            continue;
        }

        if (gameState == MODES)
        {

            window.clear();
            window.draw(menu_background);

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
            window.clear();
            window.draw(menu_background);

            window.draw(level_one);
            window.draw(level_two);
            window.draw(level_three);
            window.draw(text1);

            window.draw(back_button_level);

            window.display();
            continue;
        }

        if (gameState == GAMEOVER_MENU)
        {
            // help

            window.clear();
            window.draw(menu_background);
            window.draw(gameover_Menu_restart_button);
            window.draw(gameover_menu_stop);
            window.draw(gameover_Menu_resume_button);
            window.draw(main_menu);
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
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && !disablePlayer1Controls && !p1_dead)
            {
                dx = -1;
                dy = 0;
            };
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && !disablePlayer1Controls && !p1_dead)
            {
                dx = 1;
                dy = 0;
            };
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && !disablePlayer1Controls && !p1_dead)
            {
                dx = 0;
                dy = -1;
            };
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && !disablePlayer1Controls && !p1_dead)
            {
                dx = 0;
                dy = 1;
            };

            int temp_dx_2, temp_dy_2;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) && !isPaused && !disablePlayer1Controls && !p1_dead)
            {
                if (power_up_1p == false)
                {
                    power_up_sound.play();
                    // Set the scale of the power-up sprite

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

                    dullSprite(sTile);

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

                restoreSprite(sTile);

                dx_2 = temp_dx_2; // Reset Player 2's movement to stationary
                dy_2 = temp_dy_2; // Reset Player 2's movement to stationary

                isPaused = false;               // End the pause
                power_up_1p = false;            // Reset the power-up state
                disablePlayer2Controls = false; // Enable Player 2 controls
            }

            if (gameMode == TWO_PLAYER)
            {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && !disablePlayer2Controls && !p2_dead)
                {
                    dx_2 = -1;
                    dy_2 = 0;
                };
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && !disablePlayer2Controls && !p2_dead)
                {
                    dx_2 = 1;
                    dy_2 = 0;
                };
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && !disablePlayer2Controls && !p2_dead)
                {
                    dx_2 = 0;
                    dy_2 = -1;
                };
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && !disablePlayer2Controls && !p2_dead)
                {
                    dx_2 = 0;
                    dy_2 = 1;
                };
                int temp_dx, temp_dy;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && !isPaused && !disablePlayer2Controls && !p2_dead)
                {
                    if (power_up_2p == false)
                    {
                        power_up_sound.play();

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

                        dullSprite(sTile);

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

                    restoreSprite(sTile);

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

                    tiles_covered_1p++;
                    movement_sound.play();
                    std::cout << tiles_covered_1p << std::endl;
                    reward_p1();
                }

                if (grid[y][x] == 2 || grid[y][x] == 3)
                {
                    if (gameMode == TWO_PLAYER)
                    {
                        p1_dead = true;
                        std::cout << "Player 1 is dead!" << std::endl;
                        p1_terminated_sound.play();

                        dx = dy = 0;
                        for (int i = 0; i < M - 1; i++)
                        {
                            for (int j = 0; j < N - 1; j++)
                            {
                                if (grid[i][j] == 2 || grid[i][j] == 3)
                                {
                                    grid[i][j] = 1;
                                }
                            }
                        }
                        if (p2_dead == true)
                        {
                            Game = false;
                        }
                    }

                    if (gameMode == SINGLE_PLAYER)
                    {
                        Game = false;
                    }
                }
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

                    if (grid[y_2][x_2] == 3 || grid[y_2][x_2] == 2)
                    {
                        p2_dead = true;
                        std::cout << "Player 2 is dead!" << std::endl;
                        p2_terminated_sound.play();

                        for (int i = 0; i < M - 1; i++)
                        {
                            for (int j = 0; j < N - 1; j++)
                            {
                                if (grid[i][j] == 3 || grid[i][j] == 2)
                                {
                                    grid[i][j] = 1;
                                }
                            }
                        }
                        if (p1_dead)
                        {
                            Game = false;
                        }
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

            // help

            if (grid[y][x] == 1)
            {
                dx = dy = 0;

                tiles_covered_1p = 0;

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
                            grid[i][j] = 1;
                            tiles_covered_1p++;
                        }

                tiles_covered_1p = std::abs(movementCounter_1p - tiles_covered_1p);
                movementCounter_1p = 0;

                reward_p1();
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
                if (grid[a[i].y / ts][a[i].x / ts] == 2)
                {
                    p1_dead = true;
                    p1_terminated_sound.play();
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
                    if (p2_dead == true && gameMode == TWO_PLAYER)
                    {
                        Game = false;
                    }

                    if (gameMode == SINGLE_PLAYER)
                    {
                        Game = false;
                    }
                }
                if (grid[a[i].y / ts][a[i].x / ts] == 3)
                {
                    p2_dead = true;
                    p2_terminated_sound.play();
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
                    if (p1_dead == true && gameMode == TWO_PLAYER)
                    {
                        Game = false;
                    }
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

            movement_CounterText_1p.setString("movement counter:" + std::to_string(tiles_covered_1p));
            point_1p.setString("POINTS: " + std::to_string(_1p_points));
            score_x1p.setString("X" + std::to_string(reward_counter_1p));
            window.draw(movement_CounterText_1p);
            window.draw(point_1p);
            window.draw(score_x1p);

            if (gameMode == TWO_PLAYER)
            {
                movement_CounterText_2p.setString("movement counter:" + std::to_string(movementCounter_2p));
                point_2p.setString("POINTS: " + std::to_string(movementCounter_2p));
                window.draw(movement_CounterText_2p);
                window.draw(point_2p);
            }

            if (playModeClock.getElapsedTime().asSeconds() == 0 || !playModeClock_clockRunning) // Restart clock only once
            {
                playModeClock_clockRunning = true;

                playModeClock.restart();
            }

            playElapsedTime = playModeClock.getElapsedTime().asSeconds();
            playElapsedTimeText.setString("Time: " + std::to_string(static_cast<int>(playElapsedTime)) + "s");

            window.draw(playElapsedTimeText);

            if (power_up_1p)
            {
                power_up.setScale(4.0f, 4.0f);
                power_up.setPosition(x * ts + ts / 2 - 55, y * ts + ts / 2 - 55);
                window.draw(power_up);
            }

            if (power_up_2p)
            {
                power_up.setScale(4.0f, 4.0f);
                power_up.setPosition(x_2 * ts + ts / 2 - 55, y_2 * ts + ts / 2 - 55);
                window.draw(power_up);
            }

            window.display();
        }
    }

    return 0;
}
