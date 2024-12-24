#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <sstream>
using namespace sf;

int w = 800;
int h = 600;

class PowerUp
{
public:
    Sprite sprite;
    bool active = true;
    Clock lifetime;
    float LIFETIME_SECONDS = 5.0f; // 寶物存在時間

    PowerUp(const Texture &texture, Vector2f position)
    {
        sprite.setTexture(texture);
        float scale = 30.0f / texture.getSize().x; // 設定寶物大小
        sprite.setScale(scale, scale);
        sprite.setPosition(position);
        lifetime.restart();
    }

    bool shouldRemove() const
    {
        return lifetime.getElapsedTime().asSeconds() > LIFETIME_SECONDS;
    }
};

class SlowDownPowerUp
{
public:
    Sprite sprite;
    bool active = true;
    Clock lifetime;
    float LIFETIME_SECONDS = 5.0f; // 寶物存在時間

    SlowDownPowerUp(const Texture &texture, Vector2f position)
    {
        sprite.setTexture(texture);
        float scale = 30.0f / texture.getSize().x; // 設定寶物大小
        sprite.setScale(scale, scale);
        sprite.setPosition(position);
        lifetime.restart();
    }

    bool shouldRemove() const
    {
        return lifetime.getElapsedTime().asSeconds() > LIFETIME_SECONDS;
    }
};

// Player class to handle movement and jumping
class Player
{
public:
    Sprite sprite;
    float velocityY = 0;
    bool isJumping = false;
    bool isSpiking = false;
    const float gravity = 0.5f;
    const float jumpForce = -15.0f;
    const float groundY;
    int score = 0;
    float playerSpeed = 5.0f;
    float originalSpeed = 5.0f;

    // 力量
    bool hasPowerUp = false;
    Clock powerUpTimer;
    const float POWER_UP_DURATION = 5.0f; // 力量提升持續5秒

    // 減速
    bool isSlowedDown = false;
    Clock slowDownTimer;
    const float SLOW_DOWN_DURATION = 5.0f; // 減速效果持續5秒

    Player(const Texture &texture, float startX, float groundYPos) : groundY(groundYPos)
    {
        sprite.setTexture(texture);
        sprite.setPosition(startX, groundY);
    }

    void update()
    {
        // 檢查力量提升是否該結束
        if (hasPowerUp && powerUpTimer.getElapsedTime().asSeconds() > POWER_UP_DURATION)
        {
            hasPowerUp = false;
        }

        if (isSlowedDown && slowDownTimer.getElapsedTime().asSeconds() > SLOW_DOWN_DURATION)
        {
            isSlowedDown = false;
            playerSpeed = originalSpeed;
        }

        if (isJumping)
        {
            velocityY += gravity;
            sprite.move(0, velocityY);

            if (sprite.getPosition().y >= groundY)
            {
                sprite.setPosition(sprite.getPosition().x, groundY);
                isJumping = false;
                isSpiking = false;
                velocityY = 0;
            }
        }
    }

    void jump()
    {
        if (!isJumping)
        {
            velocityY = jumpForce;
            isJumping = true;
        }
    }

    void spike()
    {
        if (isJumping && !isSpiking)
        {
            isSpiking = true;
        }
    }

    void activatePowerUp()
    {
        hasPowerUp = true;
        powerUpTimer.restart();
    }

    void slowDown()
    {
        if (!isSlowedDown)
        {
            playerSpeed *= 0.5f;
            isSlowedDown = true;
            slowDownTimer.restart();
        }
    }
};

class Ball
{
public:
    Sprite sprite;
    Vector2f velocity;
    const float gravity = 0.3f;
    const float dampening = 0.8f;
    const float spikeForceY = 5.0f;
    const float spikeForceX = 20.0f;
    Texture texture;
    float speedMultiplier = 1.0f;
    Clock speedBoostTimer;
    const float SPEED_BOOST_DURATION = 5.0f; // 速度提升持續5秒

    Ball(const std::string &imagePath)
    {
        if (!texture.loadFromFile(imagePath))
        {
            printf("Error loading ball image!\n");
            return;
        }

        sprite.setTexture(texture);
        float scale = 50.0f / texture.getSize().x;
        sprite.setScale(scale, scale);
        sprite.setOrigin(texture.getSize().x / 2.f, texture.getSize().y / 2.f);
        reset();
    }

    void reset()
    {
        sprite.setPosition(w / 2, h / 2);
        velocity = Vector2f(rand() % 2 ? 5.0f : -5.0f, -5.0f);
    }

    Vector2f calculateSpikeVelocity(const Player &player, const Vector2f &ballPos)
    {
        Vector2f spikeVel;
        bool isLeftSide = ballPos.x < w / 2;

        // 根據球在網子左側還是右側決定水平方向
        if (isLeftSide)
        {
            spikeVel.x = spikeForceX;
        }
        else
        {
            spikeVel.x = -spikeForceX;
        }

        spikeVel.y = spikeForceY;

        return spikeVel;
    }

    void applySpeedBoost()
    {
        speedMultiplier = 2.f; // 速度提升50%
        speedBoostTimer.restart();
    }

    void update(Player &player1, Player &player2, RectangleShape &net)
    {

        velocity.y += gravity;
        sprite.move(velocity);

        FloatRect ballBounds = sprite.getGlobalBounds();

        // 球與牆壁的碰撞
        if (sprite.getPosition().x <= ballBounds.width / 2 ||
            sprite.getPosition().x >= w - ballBounds.width / 2)
        {
            velocity.x = -velocity.x * dampening;
            if (sprite.getPosition().x <= ballBounds.width / 2)
                sprite.setPosition(ballBounds.width / 2, sprite.getPosition().y);
            if (sprite.getPosition().x >= w - ballBounds.width / 2)
                sprite.setPosition(w - ballBounds.width / 2, sprite.getPosition().y);
        }

        // 球與地板的碰撞（計分）
        if (sprite.getPosition().y >= h - ballBounds.height / 2)
        {
            if (sprite.getPosition().x < w / 2)
            {
                player2.score++;
            }
            else
            {
                player1.score++;
            }
            reset();
        }

        // 球與天花板的碰撞
        if (sprite.getPosition().y <= ballBounds.height / 2)
        {
            velocity.y = -velocity.y * dampening;
            sprite.setPosition(sprite.getPosition().x, ballBounds.height / 2);
        }

        // 球與網子的碰撞
        if (ballBounds.intersects(net.getGlobalBounds()))
        {
            if (velocity.x > 0)
            {
                sprite.setPosition(net.getPosition().x - ballBounds.width / 2, sprite.getPosition().y);
            }
            else
            {
                sprite.setPosition(net.getPosition().x + net.getGlobalBounds().width + ballBounds.width / 2, sprite.getPosition().y);
            }
            velocity.x = -velocity.x * dampening;
        }

        // 球與玩家的碰撞
        Vector2f ballPos = sprite.getPosition();
        if (ballBounds.intersects(player1.sprite.getGlobalBounds()))
        {
            if (player1.isSpiking)
            {
                // 如果有力量提升，增加球的速度
                Vector2f spikeVel = calculateSpikeVelocity(player1, ballPos);
                if (player1.hasPowerUp)
                {
                    spikeVel *= 1.5f; // 力量提升時速度增加50%
                }
                velocity = spikeVel;
            }
            else
            {
                velocity.y = -12.0f;
                velocity.x = 8.0f;
                if (player1.hasPowerUp)
                {
                    velocity *= 1.3f; // 普通碰撞也稍微增加速度
                }
            }
        }
        else if (ballBounds.intersects(player2.sprite.getGlobalBounds()))
        {
            if (player2.isSpiking)
            {
                Vector2f spikeVel = calculateSpikeVelocity(player2, ballPos);
                if (player2.hasPowerUp)
                {
                    spikeVel *= 1.5f;
                }
                velocity = spikeVel;
            }
            else
            {
                velocity.y = -12.0f;
                velocity.x = -8.0f;
                if (player2.hasPowerUp)
                {
                    velocity *= 1.3f;
                }
            }
        }

        sprite.rotate(velocity.x * 2);
    }

    void draw(RenderWindow &window)
    {
        window.draw(sprite);
    }
};

void resetGame(Player &player1, Player &player2, Ball &ball, std::vector<PowerUp> &powerUps)
{
    player1.score = 0;
    player2.score = 0;
    player1.sprite.setPosition(100, h - 100);
    player2.sprite.setPosition(w - 150, h - 100);
    ball.reset();
    powerUps.clear();
}

int main()
{
    RenderWindow window(VideoMode(w, h), "Volleyball Game!");
    window.setFramerateLimit(60);

    // Load textures
    Texture t1, t2, powerUpTexture, slowDownPowerUpTexture;
    t1.loadFromFile("images/bg.jpg");
    t2.loadFromFile("images/character5.png");
    powerUpTexture.loadFromFile("images/power.png");
    slowDownPowerUpTexture.loadFromFile("images/lower.png");

    Sprite bg(t1);
    bg.setScale(float(w) / bg.getTexture()->getSize().x,
                float(h) / bg.getTexture()->getSize().y);

    // Create players
    Player player1(t2, 100, h - 100);
    Player player2(t2, w - 150, h - 100);

    float playerScaleX = 100.0f / player1.sprite.getTexture()->getSize().x;
    float playerScaleY = 100.0f / player1.sprite.getTexture()->getSize().y;
    player1.sprite.setScale(playerScaleX, playerScaleY);
    player2.sprite.setScale(playerScaleX, playerScaleY);

    Ball ball("images/ball.png");

    std::vector<PowerUp> powerUps;
    std::vector<SlowDownPowerUp> slowDownPowerUps;
    Clock powerUpSpawnTimer;
    const float SPAWN_INTERVAL = 5.0f;

    RectangleShape net(Vector2f(10, 200));
    net.setPosition(w / 2 - 5, h - 200);
    net.setFillColor(Color::White);

    Font font;
    font.loadFromFile("DotGothic16-Regular.ttf");
    Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(30);
    scoreText.setPosition(320, 10);
    scoreText.setFillColor(Color::White);

    Text winText;
    winText.setFont(font);
    winText.setCharacterSize(50);
    winText.setPosition(240, 200);
    winText.setFillColor(Color::Red);

    // Buttons
    Text restartButton;
    restartButton.setFont(font);
    restartButton.setCharacterSize(30);
    restartButton.setPosition(340, 300);
    restartButton.setFillColor(Color::Black);
    restartButton.setString("Restart");

    Text exitButton;
    exitButton.setFont(font);
    exitButton.setCharacterSize(30);
    exitButton.setPosition(360, 350);
    exitButton.setFillColor(Color::Black);
    exitButton.setString("Exit");

    // Intro text
    Text introText;
    introText.setFont(font);
    introText.setCharacterSize(20);
    introText.setPosition(100, 250);
    introText.setFillColor(Color::Black);
    introText.setString("Welcome to Volleyball Game!\n\n"
                        "Rules:\n"
                        "1. Use W and Up arrow keys to jump.\n"
                        "2. Use A/D and Left/Right arrow keys to move.\n"
                        "3. Use Z and Enter keys to spike.\n"
                        "4. First to 5 points wins.\n"
                        "5. There is some special feature in the game.\n\n"
                        "Press any key to start...");

    bool showIntro = true;

    float playerSpeed = 5.0f;
    bool gameOver = false;

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();

            if (showIntro)
            {
                if (event.type == Event::KeyPressed)
                {
                    showIntro = false;
                }
            }
            else if (event.type == Event::KeyPressed)
            {
                if (!gameOver)
                {
                    if (event.key.code == Keyboard::W)
                        player1.jump();
                    if (event.key.code == Keyboard::Up)
                        player2.jump();
                    if (event.key.code == Keyboard::Z)
                        player1.spike();
                    if (event.key.code == Keyboard::Enter)
                        player2.spike();
                }
            }

            if (event.type == Event::MouseButtonPressed)
            {
                if (gameOver)
                {
                    Vector2i mousePos = Mouse::getPosition(window);
                    if (restartButton.getGlobalBounds().contains(mousePos.x, mousePos.y))
                    {
                        resetGame(player1, player2, ball, powerUps);
                        gameOver = false;
                    }
                    else if (exitButton.getGlobalBounds().contains(mousePos.x, mousePos.y))
                    {
                        window.close();
                    }
                }
            }
        }

        if (!showIntro && !gameOver)
        {
            // Player horizontal movement
            if (Keyboard::isKeyPressed(Keyboard::A))
            {
                if (player1.sprite.getPosition().x > 0)
                    player1.sprite.move(-player1.playerSpeed, 0);
            }
            if (Keyboard::isKeyPressed(Keyboard::D))
            {
                if (player1.sprite.getPosition().x + player1.sprite.getGlobalBounds().width < w / 2 - 10)
                    player1.sprite.move(player1.playerSpeed, 0);
            }
            if (Keyboard::isKeyPressed(Keyboard::Left))
            {
                if (player2.sprite.getPosition().x > w / 2 + 10)
                    player2.sprite.move(-player2.playerSpeed, 0);
            }
            if (Keyboard::isKeyPressed(Keyboard::Right))
            {
                if (player2.sprite.getPosition().x + player2.sprite.getGlobalBounds().width < w)
                    player2.sprite.move(player2.playerSpeed, 0);
            }

            // Update game objects
            player1.update();
            player2.update();
            ball.update(player1, player2, net);

            // Update score text
            std::stringstream ss;
            ss << "score:" << player1.score << " - " << player2.score;
            scoreText.setString(ss.str());

            // Check for win condition
            if (player1.score >= 5)
            {
                winText.setString("Player 1 Wins!");
                gameOver = true;
            }
            else if (player2.score >= 5)
            {
                winText.setString("Player 2 Wins!");
                gameOver = true;
            }

            // 產生新的寶物
            if (powerUpSpawnTimer.getElapsedTime().asSeconds() > SPAWN_INTERVAL)
            {
                Vector2f randomPos(
                    50 + rand() % (w - 100),
                    (h / 2) + 50 + rand() % ((h / 2) - 100));
                if (rand() % 2 == 0)
                {
                    powerUps.push_back(PowerUp(powerUpTexture, randomPos));
                }
                else
                {
                    slowDownPowerUps.push_back(SlowDownPowerUp(slowDownPowerUpTexture, randomPos));
                }
                powerUpSpawnTimer.restart();
            }

            // 更新寶物狀態並檢查與玩家的碰撞
            for (std::vector<PowerUp>::iterator it = powerUps.begin(); it != powerUps.end();)
            {
                if (it->shouldRemove())
                {
                    it = powerUps.erase(it);
                }
                else
                {
                    // 檢查玩家是否碰到寶物
                    if (it->active)
                    {
                        if (player1.sprite.getGlobalBounds().intersects(it->sprite.getGlobalBounds()))
                        {
                            player1.activatePowerUp();
                            it = powerUps.erase(it);
                            continue;
                        }
                        if (player2.sprite.getGlobalBounds().intersects(it->sprite.getGlobalBounds()))
                        {
                            player2.activatePowerUp();
                            it = powerUps.erase(it);
                            continue;
                        }
                    }
                    ++it;
                }
            }

            // 更新減速寶物狀態並檢查與玩家的碰撞
            for (std::vector<SlowDownPowerUp>::iterator it = slowDownPowerUps.begin(); it != slowDownPowerUps.end();)
            {
                if (it->shouldRemove())
                {
                    it = slowDownPowerUps.erase(it);
                }
                else
                {
                    // 檢查玩家是否碰到寶物
                    if (it->active)
                    {
                        if (player1.sprite.getGlobalBounds().intersects(it->sprite.getGlobalBounds()))
                        {
                            player2.slowDown();
                            it = slowDownPowerUps.erase(it);
                            continue;
                        }
                        if (player2.sprite.getGlobalBounds().intersects(it->sprite.getGlobalBounds()))
                        {
                            player1.slowDown();
                            it = slowDownPowerUps.erase(it);
                            continue;
                        }
                    }
                    ++it;
                }
            }
        }

        // Draw everything
        window.clear();

        if (showIntro)
        {
            window.draw(bg);        // 繪製背景
            window.draw(introText); // 繪製介紹文字
        }
        else
        {
            window.draw(bg);
            for (const auto &powerUp : powerUps)
            {
                window.draw(powerUp.sprite);
            }
            for (const auto &slowDownPowerUp : slowDownPowerUps)
            {
                window.draw(slowDownPowerUp.sprite);
            }
            window.draw(player1.sprite);
            window.draw(player2.sprite);
            window.draw(net);
            ball.draw(window);
            window.draw(scoreText);
            if (gameOver)
            {
                window.draw(winText);
                window.draw(restartButton);
                window.draw(exitButton);
            }
        }
        window.display();
    }

    return 0;
}