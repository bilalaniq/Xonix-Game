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

enum GameState
{
    MENU,
    PLAY,
    GAMEOVER_MENU,
    MODES,
    highscore
};

enum GameMode
{
    SINGLE_PLAYER,
    TWO_PLAYER
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

bool lock_1p = false;
bool lock_2p = false;

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

bool New_highscore = false;

int power_up_inventory_1p = 0;
int power_up_inventory_2p = 0;

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
    movement_type movement = LINEAR;

    Enemy()
    {
        x = y = 300;
        dx = 4 - rand() % 11;
        dy = 4 - rand() % 11;
    }

    void move()
    {
        if (isPaused)
            return;

        if (movement == LINEAR)
        {
            x += dx;

            if (grid[y / ts][x / ts] == 1)
            {
                dx = -dx;
                x += dx;
            }
            y += dy;
            if (grid[y / ts][x / ts] == 1)
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
        static int verticalDir = 1;

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
            move_dx = 5;
        }
        else
        {
            move_dx = -5;
        }

        int move_dy = verticalDir * 2;

        move_dx += rand() % 3 - 1;
        move_dy += rand() % 3 - 1;

        int new_x = x + move_dx;
        int new_y = y + move_dy;

        if (new_x >= 0 && new_x < N * ts && new_y >= 0 && new_y < M * ts)
        {
            if (grid[new_y / ts][new_x / ts] != 1)
            {
                x = new_x;
                y = new_y;
            }
            else
            {
                goingRight = !goingRight;
                verticalDir *= -1;
                frameCount = 0;
            }
        }
        else
        {
            goingRight = !goingRight;
            verticalDir *= -1;
        }
    }

    void circular()
    {
        static float angle = 0.0f;
        angle += 0.05f;

        int radius = 60;
        int cx = 300, cy = 300;

        x = cx + static_cast<int>(radius * cos(angle)); //  x = h + r cos(t)
        y = cy + static_cast<int>(radius * sin(angle)); //  y = k + r sin(t)
    }
};

struct ScoreEntry
{
    int score;
    int timeTaken;
};

void ensureScoreFileExists(const std::string &filename = "scores.txt")
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "File not found. Creating a new file: " << filename << std::endl;
        std::ofstream newFile(filename); // Create the file
        if (!newFile)
        {
            std::cerr << "Error: Could not create file " << filename << "!" << std::endl;
            return;
        }
        newFile.close();
    }
    else
    {
        std::cout << "File " << filename << " already exists." << std::endl;
    }
}

void loadScoresFromFile(ScoreEntry scores[], int &count, const std::string &filename = "scores.txt")
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "File not found. Creating a new file: " << filename << std::endl;
        std::ofstream newFile(filename); // Create the file
        if (!newFile)
        {
            std::cerr << "Error: Could not create file " << filename << "!" << std::endl;
            count = 0;
            return;
        }
        newFile.close();
        count = 0;
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
        // Check if the new score is the highest score
        if (count == 0 || newScore > scores[0].score) // If no scores exist or newScore is higher
        {
            New_highscore = true; // Update the high_score flag
        }

        if (count < 5)
        {
            count++;
        }

        int i = count - 1;
        while (i > 0 && scores[i - 1].score < newScore)
        {
            scores[i] = scores[i - 1];
            i--;
        }

        scores[i].score = newScore;
        scores[i].timeTaken = newTime;
    }
    else
    {
        New_highscore = false; // No new high score
    }
}

void saveScoresToFile(ScoreEntry scores[], int count, const std::string &filename = "scores.txt")
{
    std::ofstream file(filename);
    if (!file)
    {
        std::cerr << "Error: Could not open file " << filename << " for writing!" << std::endl;
        return;
    }

    // Write only the top 5 scores to the file
    for (int i = 0; i < std::min(count, 5); i++)
    {
        file << scores[i].score << " " << scores[i].timeTaken << std::endl;
    }

    file.close();
}

void handleGameOver()
{

    ScoreEntry scores[5];
    int count = 0;

    loadScoresFromFile(scores, count);

    int newScore;
    if (_1p_points > _2p_points)
    {
        newScore = _1p_points;
    }
    else if (_2p_points > _1p_points)
    {
        newScore = _2p_points;
    }
    else
    {
        newScore = _1p_points;
    }

    int newTime = static_cast<int>(playElapsedTime);
    updateScoreboard(scores, count, newScore, newTime);

    saveScoresToFile(scores, count);

    std::cout << "Scoreboard updated!" << std::endl;
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
    sprite.setColor(sf::Color(128, 128, 128, 255)); // Dull the sprite by changing its color to gray
}

void restoreSprite(sf::Sprite &sprite)
{
    sprite.setColor(sf::Color::White); // Restore by resetting to original color
}

void reward_p1()
{
    if (tiles_covered_1p >= 10 && reward_counter_1p <= 3)
    {
        _1p_points += tiles_covered_1p * 2;
        reward_counter_1p++;
        std::cout << "Reward 1: " << reward_counter_1p << " (×2 points)" << std::endl;
    }
    else if (tiles_covered_1p >= 5 && reward_counter_1p > 3 && reward_counter_1p <= 5)
    {
        _1p_points += tiles_covered_1p * 2;
        reward_counter_1p++;
        std::cout << "Reward Counter2: " << reward_counter_1p << " (×2 points, reduced threshold)" << std::endl;
    }
    else if (tiles_covered_1p >= 5 && reward_counter_1p > 5)
    {
        _1p_points += tiles_covered_1p * 4;
        reward_counter_1p++;
        std::cout << "Reward Counter3: " << reward_counter_1p << " (×4 points)" << std::endl;
    }

    static int nextPowerUpThreshold = 50;
    if (_1p_points >= nextPowerUpThreshold)
    {
        power_up_inventory_1p++;
        std::cout << "Player 1 earned a power-up! Total: " << power_up_inventory_1p << std::endl;
        nextPowerUpThreshold += 30;
    }

    tiles_covered_1p = 0;
}

void reward_p2()
{
    if (tiles_covered_2p >= 10 && reward_counter_2p <= 3)
    {
        _2p_points += tiles_covered_2p * 2;
        reward_counter_2p++;
        std::cout << "Reward 1 (P2): " << reward_counter_2p << " (×2 points)" << std::endl;
    }
    else if (tiles_covered_2p >= 5 && reward_counter_2p > 3 && reward_counter_2p <= 5)
    {
        _2p_points += tiles_covered_2p * 2;
        reward_counter_2p++;
        std::cout << "Reward Counter2 (P2): " << reward_counter_2p << " (×2 points, reduced threshold)" << std::endl;
    }
    else if (tiles_covered_2p >= 5 && reward_counter_2p > 5)
    {
        _2p_points += tiles_covered_2p * 4;
        reward_counter_2p++;
        std::cout << "Reward Counter3 (P2): " << reward_counter_2p << " (×4 points)" << std::endl;
    }

    static int nextPowerUpThreshold = 50;
    if (_2p_points >= nextPowerUpThreshold)
    {
        power_up_inventory_2p++;
        std::cout << "Player 2 earned a power-up! Total: " << power_up_inventory_2p << std::endl;
        nextPowerUpThreshold += 30;
    }

    tiles_covered_2p = 0;
}

void floodFill(int i, int j, int playerTrail)
{
    // Check boundaries to avoid out-of-bounds access
    if (i < 0 || i >= M || j < 0 || j >= N)
        return;

    // If the cell is not empty (0) or is a wall (1), stop processing this cell
    if (grid[i][j] != 0)
        return;

    // Mark the cell as part of the player's trail
    grid[i][j] = playerTrail;

    // Recursively flood-fill all neighbors (4 sides)
    floodFill(i - 1, j, playerTrail); // Top
    floodFill(i + 1, j, playerTrail); // Bottom
    floodFill(i, j - 1, playerTrail); // Left
    floodFill(i, j + 1, playerTrail); // Right
}

void processFloodFill(int playerTrail)
{
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {
            if (grid[i][j] == playerTrail)
            {
                if (i > 0)
                    floodFill(i - 1, j, playerTrail);
                if (i < M - 1)
                    floodFill(i + 1, j, playerTrail);
                if (j > 0)
                    floodFill(i, j - 1, playerTrail);
                if (j < N - 1)
                    floodFill(i, j + 1, playerTrail);
            }
        }
    }
}

int main()
{
    ensureScoreFileExists("scores.txt");
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

    construct_boundry(); // for making the boundry constructed by 1

    sf::Font font;
    if (!font.loadFromFile("images/Arial.ttf"))
    {
        std::cerr << "Error loading font!" << std::endl;
        return -1;
    }

    sf::Texture background_texture;
    if (!background_texture.loadFromFile("images/menu_bg.png"))
    {
        std::cerr << "Error loading menu_bg.png!" << std::endl;
        return -1;
    }

    sf::Sprite menu_background(background_texture);

    menu_background.setScale(
        static_cast<float>(N * ts) / background_texture.getSize().x,
        static_cast<float>(M * ts) / background_texture.getSize().y);

    sf::Texture menu_texture;
    menu_texture.loadFromFile("images/menu_xonix_pic.png");
    sf::Sprite menu_text(menu_texture);
    menu_text.setPosition(50, 20);
    menu_text.setScale(0.4f, 0.4f);

    sf::Text start_button_text;
    start_button_text.setFont(font);
    start_button_text.setString("Start");
    start_button_text.setCharacterSize(50);
    start_button_text.setFillColor(sf::Color::White);
    start_button_text.setPosition(50, 150);
    start_button_text.setStyle(sf::Text::Bold);

    sf::Text mode_button_text;
    mode_button_text.setFont(font);
    mode_button_text.setString("Modes");
    mode_button_text.setCharacterSize(50);
    mode_button_text.setFillColor(sf::Color::White);
    mode_button_text.setPosition(50, 250);
    mode_button_text.setStyle(sf::Text::Bold);

    sf::Text stop_button_text;
    stop_button_text.setFont(font);
    stop_button_text.setString("Stop");
    stop_button_text.setCharacterSize(50);
    stop_button_text.setFillColor(sf::Color::White);
    stop_button_text.setPosition(50, 350);
    stop_button_text.setStyle(sf::Text::Bold);

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

    sf::Text easy_button_text;
    easy_button_text.setFont(font);
    easy_button_text.setString("Easy");
    easy_button_text.setCharacterSize(50);
    easy_button_text.setFillColor(sf::Color::White);
    easy_button_text.setPosition(430, 150);
    easy_button_text.setStyle(sf::Text::Bold);

    sf::Text medium_button_text;
    medium_button_text.setFont(font);
    medium_button_text.setString("Medium");
    medium_button_text.setCharacterSize(50);
    medium_button_text.setFillColor(sf::Color::White);
    medium_button_text.setPosition(400, 250);
    medium_button_text.setStyle(sf::Text::Bold);

    sf::Text hard_button_text;
    hard_button_text.setFont(font);
    hard_button_text.setString("Hard");
    hard_button_text.setCharacterSize(50);
    hard_button_text.setFillColor(sf::Color::White);
    hard_button_text.setPosition(430, 350);
    hard_button_text.setStyle(sf::Text::Bold);

    sf::Texture ContinuousMode;
    ContinuousMode.loadFromFile("images/ContinuousMode.png");
    sf::Sprite continuous_button(ContinuousMode);
    continuous_button.setPosition(350, 50);
    continuous_button.setScale(1.7, 1.7f);

    sf::Text back_button_mode_text;
    back_button_mode_text.setFont(font);
    back_button_mode_text.setString("Back");
    back_button_mode_text.setCharacterSize(50);
    back_button_mode_text.setFillColor(sf::Color::White);
    back_button_mode_text.setPosition(430, 450);
    back_button_mode_text.setStyle(sf::Text::Bold);

    sf::Text back_button_score_text;
    back_button_score_text.setFont(font);
    back_button_score_text.setString("Back");
    back_button_score_text.setCharacterSize(50);
    back_button_score_text.setFillColor(sf::Color::White);
    back_button_score_text.setPosition(350, 500);
    back_button_score_text.setStyle(sf::Text::Bold);

    sf::Text point_1p;
    point_1p.setFont(font);
    point_1p.setCharacterSize(25);
    point_1p.setFillColor(sf::Color(255, 0, 0));
    point_1p.setPosition(10, 30);
    point_1p.setString("0");
    point_1p.setStyle(sf::Text::Bold);

    sf::Text score_x1p;
    score_x1p.setFont(font);
    score_x1p.setCharacterSize(25);
    score_x1p.setFillColor(sf::Color(255, 0, 0));
    score_x1p.setPosition(10, 50);
    score_x1p.setString("0");
    score_x1p.setStyle(sf::Text::Bold);

    sf::Text movement_counter_1p_text;
    movement_counter_1p_text.setFont(font);
    movement_counter_1p_text.setCharacterSize(25);
    movement_counter_1p_text.setFillColor(sf::Color(255, 0, 0));
    movement_counter_1p_text.setPosition(10, 70);
    movement_counter_1p_text.setString("0");
    movement_counter_1p_text.setStyle(sf::Text::Bold);

    sf::Text score_x2p;
    score_x2p.setFont(font);
    score_x2p.setCharacterSize(25);
    score_x2p.setFillColor(sf::Color(255, 165, 0));
    score_x2p.setPosition(815, 50);
    score_x2p.setString("0");
    score_x2p.setStyle(sf::Text::Bold);

    sf::Text point_2p;
    point_2p.setFont(font);
    point_2p.setCharacterSize(25);
    point_2p.setFillColor(sf::Color(255, 165, 0));
    point_2p.setPosition(815, 30);
    point_2p.setString("0");
    point_2p.setStyle(sf::Text::Bold);

    sf::Text movement_counter_2p_text;
    movement_counter_2p_text.setFont(font);
    movement_counter_2p_text.setCharacterSize(25);
    movement_counter_2p_text.setFillColor(sf::Color(255, 165, 0));
    movement_counter_2p_text.setPosition(815, 70);
    movement_counter_2p_text.setString("0");
    movement_counter_2p_text.setStyle(sf::Text::Bold);

    sf::Text playElapsedTimeText;
    playElapsedTimeText.setFont(font);
    playElapsedTimeText.setCharacterSize(20);
    playElapsedTimeText.setFillColor(sf::Color::White);
    playElapsedTimeText.setPosition(480, 10);
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

    sf::Text gameover_Menu_restart_text;
    gameover_Menu_restart_text.setFont(font);
    gameover_Menu_restart_text.setString("Restart");
    gameover_Menu_restart_text.setCharacterSize(50);
    gameover_Menu_restart_text.setFillColor(sf::Color::White);
    gameover_Menu_restart_text.setPosition(50, 60);
    gameover_Menu_restart_text.setStyle(sf::Text::Bold);

    sf::Text gameover_Menu_resume_text;
    gameover_Menu_resume_text.setFont(font);
    gameover_Menu_resume_text.setString("Resume");
    gameover_Menu_resume_text.setCharacterSize(50);
    gameover_Menu_resume_text.setFillColor(sf::Color::White);
    gameover_Menu_resume_text.setPosition(55, 170);
    gameover_Menu_resume_text.setStyle(sf::Text::Bold);

    sf::Text gameover_menu_stop_text;
    gameover_menu_stop_text.setFont(font);
    gameover_menu_stop_text.setString("Stop");
    gameover_menu_stop_text.setCharacterSize(50);
    gameover_menu_stop_text.setFillColor(sf::Color::White);
    gameover_menu_stop_text.setPosition(50, 420);
    gameover_menu_stop_text.setStyle(sf::Text::Bold);

    sf::Text score_show_gameover_menu;
    score_show_gameover_menu.setFont(font);
    score_show_gameover_menu.setString("Score: 0");
    score_show_gameover_menu.setCharacterSize(50);
    score_show_gameover_menu.setFillColor(sf::Color::White);
    score_show_gameover_menu.setPosition(600, 170);
    score_show_gameover_menu.setStyle(sf::Text::Bold);

    sf::Text score_show_gameover_menu_1p;
    score_show_gameover_menu_1p.setFont(font);
    score_show_gameover_menu_1p.setString("p1 Score: 0");
    score_show_gameover_menu_1p.setCharacterSize(50);
    score_show_gameover_menu_1p.setFillColor(sf::Color::White);
    score_show_gameover_menu_1p.setPosition(550, 170);
    score_show_gameover_menu_1p.setStyle(sf::Text::Bold);

    sf::Text score_show_gameover_menu_2p;
    score_show_gameover_menu_2p.setFont(font);
    score_show_gameover_menu_2p.setString("P2 Score: 0");
    score_show_gameover_menu_2p.setCharacterSize(50);
    score_show_gameover_menu_2p.setFillColor(sf::Color::White);
    score_show_gameover_menu_2p.setPosition(550, 250);
    score_show_gameover_menu_2p.setStyle(sf::Text::Bold);

    sf::Text who_wins_text;
    who_wins_text.setFont(font);
    who_wins_text.setString("Still Playing...");
    who_wins_text.setCharacterSize(50);
    who_wins_text.setFillColor(sf::Color::White);
    who_wins_text.setPosition(550, 110);
    who_wins_text.setStyle(sf::Text::Bold);

    sf::Text New_highscore_text;
    New_highscore_text.setFont(font);
    New_highscore_text.setString("New Highscore!");
    New_highscore_text.setCharacterSize(50);
    New_highscore_text.setFillColor(sf::Color::Red);
    New_highscore_text.setPosition(600, 250);
    New_highscore_text.setStyle(sf::Text::Bold);

    sf::Text main_menu_text;
    main_menu_text.setFont(font);
    main_menu_text.setString("Main Menu");
    main_menu_text.setCharacterSize(50);
    main_menu_text.setFillColor(sf::Color::White);
    main_menu_text.setPosition(50, 300);
    main_menu_text.setStyle(sf::Text::Bold);

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
                    if (start_button_text.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        gameState = PLAY;
                    }

                    else if (mode_button_text.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        gameState = MODES;
                    }
                    else if (stop_button_text.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        window.close();
                    }

                    if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left)
                    {
                        sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                        if (start_button_text.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
                        {
                            button_sound.play();
                            gameState = PLAY;
                        }
                        else if (mode_button_text.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)))
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
                    else if (back_button_mode_text.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        gameState = MENU;
                    }
                    else if (easy_button_text.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        gameDifficulty = EASY;
                        enemyCount = 2; // Set enemy count for easy mode
                        button_sound.play();
                        std::cout << "Easy mode selected!" << std::endl;
                    }
                    else if (medium_button_text.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        gameDifficulty = MEDIUM;
                        enemyCount = 4; // Set enemy count for medium mode
                        button_sound.play();
                        std::cout << "Medium mode selected!" << std::endl;
                    }
                    else if (hard_button_text.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
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

                else if (gameState == highscore)
                {
                    if (back_button_score_text.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        gameState = MENU;
                    }
                }
                else if (gameState == GAMEOVER_MENU)
                {
                    if (gameover_Menu_restart_text.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();

                        // Reset timers
                        continuousModeTimer = 0;
                        enemySpeed_timer = 0;
                        enemy_movement_timer = 0;

                        // Reset scores
                        _1p_points = 0;
                        _2p_points = 0;

                        // Reset game states
                        isPaused = false;
                        disablePlayer2Controls = false;
                        disablePlayer1Controls = false;

                        p1_dead = false;
                        p2_dead = false;

                        // Reset elapsed time
                        playElapsedTime = 0;
                        elapsedTime = 0;

                        // Reset power-ups
                        power_up_1p = false;
                        power_up_2p = false;

                        // Reset reward counters
                        reward_counter_1p = 1;
                        reward_counter_2p = 1;

                        // Reset movement counters
                        movementCounter_1p = 0;
                        movementCounter_2p = 0;

                        power_up_inventory_1p = 0;
                        power_up_inventory_2p = 0;

                        // Restart the play mode clock
                        playModeClock.restart();
                        playModeClock_clockRunning = false;

                        // Reset the grid
                        for (int i = 1; i < M - 1; i++)
                        {
                            for (int j = 1; j < N - 1; j++)
                            {
                                grid[i][j] = 0; // Clear the grid
                            }
                        }

                        // Reset enemies
                        for (int i = 0; i < enemyCount; i++)
                        {
                            x = y = 300;
                            a[i].dx = 4 - rand() % 11;
                            a[i].dy = 4 - rand() % 11;
                            a[i].movement = LINEAR; // Reset enemy movement type to default
                        }

                        // Reset player positions
                        x = 0, y = 0, dx = 0, dy = 0;
                        x_2 = N - 1, y_2 = 0, dx_2 = 0, dy_2 = 0;

                        // Reset game state
                        Game = true;
                        gameState = PLAY;
                    }
                    else if (gameover_menu_stop_text.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        window.close(); // Close the game
                    }
                    else if (gameover_Menu_resume_text.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();
                        Game = true;
                        gameState = PLAY; // Resume the game
                    }
                    else if (main_menu_text.getGlobalBounds().contains((float)mousePos.x, (float)mousePos.y))
                    {
                        button_sound.play();

                        // Reset timers
                        continuousModeTimer = 0;
                        enemySpeed_timer = 0;
                        enemy_movement_timer = 0;

                        // Reset scores
                        _1p_points = 0;
                        _2p_points = 0;

                        // Reset game states
                        isPaused = false;
                        disablePlayer2Controls = false;
                        disablePlayer1Controls = false;

                        p1_dead = false;
                        p2_dead = false;

                        // Reset elapsed time
                        playElapsedTime = 0;
                        elapsedTime = 0;

                        // Reset power-ups
                        power_up_1p = false;
                        power_up_2p = false;

                        // Reset reward counters
                        reward_counter_1p = 1;
                        reward_counter_2p = 1;

                        // Reset movement counters
                        movementCounter_1p = 0;
                        movementCounter_2p = 0;

                        // Stop the play mode clock
                        playModeClock_clockRunning = false;

                        power_up_inventory_1p = 0;
                        power_up_inventory_2p = 0;

                        // Reset the grid
                        for (int i = 1; i < M - 1; i++)
                        {
                            for (int j = 1; j < N - 1; j++)
                            {
                                grid[i][j] = 0; // Clear the grid
                            }
                        }

                        // Reset enemies
                        for (int i = 0; i < enemyCount; i++)
                        {
                            x = y = 300;
                            a[i].dx = 4 - rand() % 11;
                            a[i].dy = 4 - rand() % 11;
                            a[i].movement = LINEAR; // Reset enemy movement type to default
                        }

                        // Reset player positions
                        x = 0, y = 0, dx = 0, dy = 0;
                        x_2 = N - 1, y_2 = 0, dx_2 = 0, dy_2 = 0;

                        // Reset game state
                        Game = true;
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

            // Draw the menu elements
            window.clear(sf::Color::White);
            window.draw(menu_background);

            window.draw(menu_text);
            window.draw(start_button_text);
            window.draw(mode_button_text);
            window.draw(stop_button_text);
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
            window.draw(back_button_score_text);

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

            window.draw(easy_button_text);
            window.draw(medium_button_text);
            window.draw(hard_button_text);

            window.draw(back_button_mode_text);

            window.display();
            continue;
        }

        if (gameState == GAMEOVER_MENU)
        {

            window.clear();
            window.draw(menu_background);
            window.draw(gameover_Menu_restart_text);
            window.draw(gameover_menu_stop_text);
            window.draw(gameover_Menu_resume_text);
            window.draw(main_menu_text);

            if (gameMode == SINGLE_PLAYER)
            {
                score_show_gameover_menu.setString("Score: " + std::to_string(_1p_points));
                window.draw(score_show_gameover_menu);

                New_highscore_text.setPosition(600, 250); // Set the position

                if (New_highscore)
                {
                    window.draw(New_highscore_text);
                }
            }
            else if (gameMode == TWO_PLAYER)
            {

                score_show_gameover_menu_1p.setString("p1 Score: " + std::to_string(_1p_points));

                score_show_gameover_menu_2p.setString("p2 Score: " + std::to_string(_2p_points));

                if (p1_dead && p2_dead && _2p_points > _1p_points)
                {
                    who_wins_text.setString("Player 2 wins!");
                }
                else if (p1_dead && p2_dead && _1p_points > _2p_points)
                {
                    who_wins_text.setString("Player 1 wins!");
                }
                else if (p2_dead && p1_dead && _2p_points == _1p_points)
                {
                    who_wins_text.setString("It's a tie!");
                }
                else if (!p2_dead && !p1_dead)
                {
                    who_wins_text.setString("Still Playing...");
                }

                window.draw(score_show_gameover_menu_1p);
                window.draw(score_show_gameover_menu_2p);
                window.draw(who_wins_text);

                New_highscore_text.setPosition(550, 320);

                if (New_highscore)
                {
                    window.draw(New_highscore_text);
                }
            }

            window.display();
            continue;
        }

        if (gameState == PLAY)
        {

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
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) && power_up_inventory_1p > 0 && !isPaused && !disablePlayer1Controls && !p1_dead)
            {
                if (power_up_1p == false)
                {
                    power_up_sound.play();

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
                    power_up_inventory_1p--;
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
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && power_up_inventory_2p > 0 && !isPaused && !disablePlayer2Controls && !p2_dead)
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
                        power_up_inventory_2p--;
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
                        tiles_covered_2p++;
                        movement_sound.play();
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
                {
                    drop(a[i].y / ts, a[i].x / ts);
                }

                processFloodFill(2);

                for (int i = 0; i < M; i++)
                    for (int j = 0; j < N; j++)
                        if (grid[i][j] == -1)
                        {
                            grid[i][j] = 0;
                        }
                        else if (grid[i][j] == 2)
                        {
                            grid[i][j] = 1;
                            tiles_covered_1p++;
                        }

                movementCounter_1p = 0;

                reward_p1();
            }

            if (gameMode == TWO_PLAYER && grid[y_2][x_2] == 1)
            {

                dx_2 = dy_2 = 0;

                tiles_covered_2p = 0;

                for (int i = 0; i < enemyCount; i++)
                    drop(a[i].y / ts, a[i].x / ts);

                processFloodFill(3);

                for (int i = 0; i < M; i++)
                    for (int j = 0; j < N; j++)
                        if (grid[i][j] == -1)
                        {
                            grid[i][j] = 0;
                        }
                        else if (grid[i][j] == 3)
                        {
                            grid[i][j] = 1;
                            tiles_covered_2p++;
                        }

                movementCounter_2p = 0;
                reward_p2();
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
                if (gameState == PLAY)
                {
                    handleGameOver();
                }

                gameover_sound.play();
                window.draw(sGameover);

                gameState = GAMEOVER_MENU;
            }

            point_1p.setString("POINTS: " + std::to_string(_1p_points));
            score_x1p.setString("X" + std::to_string(reward_counter_1p));
            movement_counter_1p_text.setString("Movement: " + std::to_string(movementCounter_1p));
            window.draw(point_1p);
            window.draw(score_x1p);
            window.draw(movement_counter_1p_text);

            if (gameMode == TWO_PLAYER)
            {
                point_2p.setString("POINTS: " + std::to_string(_2p_points));
                score_x2p.setString("X" + std::to_string(reward_counter_2p));
                movement_counter_2p_text.setString("Movement: " + std::to_string(movementCounter_2p));

                window.draw(point_2p);
                window.draw(score_x2p);
                window.draw(movement_counter_2p_text);
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
