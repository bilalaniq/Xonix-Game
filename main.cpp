#include <SFML/Graphics.hpp>
#include <time.h>
#include <cmath>
#include <cstdlib>
#include <string>
#include <cstring>

const int M = 25;
const int N = 40;

int grid[M][N] = {0};
int ts = 18;

struct Enemy
{
    int x, y, dx, dy;

    Enemy()
    {
        x = y = 300;
        dx = 4 - rand() % 8;
        dy = 4 - rand() % 8;
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
                        grid[i][j] = 1;
        }

        for (int i = 0; i < enemyCount; i++) // if the enemy hits the players
            if (grid[a[i].y / ts][a[i].x / ts] == 2)
                Game = false;

        /////////draw//////////
        window.clear();

        for (int i = 0; i < M; i++)
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

        sTile.setTextureRect(sf::IntRect(36, 0, ts, ts));
        sTile.setPosition(x * ts, y * ts);
        window.draw(sTile);

        sEnemy.rotate(10);
        for (int i = 0; i < enemyCount; i++)
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
