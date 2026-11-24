#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>

using namespace sf;
using namespace std;

// Window and game states
const int windowWidth = 900;
const int windowHeight = 600;

const int menuState = 0;
const int playingState = 1;
const int gameOverState = 2;

// Menu options
const int optionStart = 0;
const int optionControls = 1;
const int optionExit = 2;
const int optionCount = 3;

// Gameplay constants
const float enforcerRadius = 30;
const float enforcerSpeed = 6;
const float puckRadius = 15;
const float hitSpeed = 10;        // base speed given to the puck on a hit
const float maxPuckSpeed = 18;    // puck can never go faster than this
const float serveSpeed = 7;       // speed of the puck when the round starts
const float friction = 0.985f;
const float stopSpeed = 0.1f;
const int winningScore = 5;

const float goalHeight = 200;                             // size of the goal opening
const float goalTop = (windowHeight - goalHeight) / 2;    // y where the goal starts
const float goalBottom = goalTop + goalHeight;            // y where the goal ends
const int goalPauseFrames = 60;                           // 1 second freeze after a goal
const int countdownFrames = 120;                          // 3 2 1 countdown before the serve
const int framesPerSecond = 60;
const int matchSeconds = 180;
const int matchTimeFrames = matchSeconds * framesPerSecond;


// Helper functions
void resetPuck(CircleShape& puck, float& puckSpeedX, float& puckSpeedY)
{
    puck.setPosition(Vector2f(windowWidth / 2, windowHeight / 2));
    puckSpeedX = 0;
    puckSpeedY = 0;
}

void resetEnforcers(CircleShape& player1, CircleShape& player2)
{
    player1.setPosition(Vector2f(100, windowHeight / 2));
    player2.setPosition(Vector2f(windowWidth - 100, windowHeight / 2));
}


// function to launch puck randomly at each serve
void servePuck(float& puckSpeedX, float& puckSpeedY, int direction)
{
    if (direction == 0)
    {
        if (rand() % 2 == 0)
        {
            direction = -1;
        }
        else
        {
            direction = 1;
        }
    }

    float angle = (rand() % 61 - 30) * 3.14159f / 180;   // gives angle between -30 and +30 degrees randomly

    puckSpeedX = direction * serveSpeed * cos(angle);
    puckSpeedY = serveSpeed * sin(angle);
}

void centerText(Text& text, float x, float y)
{
    FloatRect bounds = text.getLocalBounds();
    text.setOrigin(Vector2f(bounds.position.x + bounds.size.x / 2, bounds.position.y + bounds.size.y / 2));
    text.setPosition(Vector2f(x, y));
}

void updateScoreText(Text& scoreText, int score1, int score2)
{
    scoreText.setString(to_string(score1) + " : " + to_string(score2));
    centerText(scoreText, windowWidth / 2, 40);
}

void updateTimerText(Text& timerText, int framesLeft)
{
    int totalSeconds = (framesLeft + framesPerSecond - 1) / framesPerSecond;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    string secondsString = to_string(seconds);
    if (seconds < 10)
    {
        secondsString = "0" + secondsString;
    }

    timerText.setString(to_string(minutes) + ":" + secondsString);
    centerText(timerText, windowWidth / 2, 90);
}

void setWinnerText(Text& winnerText, int score1, int score2)
{
    if (score1 > score2)
    {
        winnerText.setString("PLAYER 1 WINS!");
        winnerText.setFillColor(Color(30, 90, 220));
    }
    else if (score2 > score1)
    {
        winnerText.setString("PLAYER 2 WINS!");
        winnerText.setFillColor(Color(220, 50, 50));
    }
    else
    {
        winnerText.setString("IT'S A DRAW!");
        winnerText.setFillColor(Color(90, 90, 90));
    }
}

void drawButton(RenderWindow& window, RectangleShape& button, Text& text, float y, bool selected)
{
    button.setPosition(Vector2f(windowWidth / 2 - 180, y - 28));

    if (selected)
    {
        button.setFillColor(Color(40, 110, 240));
        button.setOutlineColor(Color(150, 205, 255));
        text.setFillColor(Color::White);
    }
    else
    {
        button.setFillColor(Color(25, 40, 75));
        button.setOutlineColor(Color(60, 85, 140));
        text.setFillColor(Color(150, 170, 210));
    }

    centerText(text, windowWidth / 2, y);
    window.draw(button);
    window.draw(text);
}

// function to control movement
void moveEnforcer(CircleShape& enforcer, bool up, bool down, bool left, bool right,
                  float minX, float maxX, float& movedX, float& movedY)
{
    float moveX = 0;
    float moveY = 0;

    if (up)    moveY -= enforcerSpeed;
    if (down)  moveY += enforcerSpeed;
    if (left)  moveX -= enforcerSpeed;
    if (right) moveX += enforcerSpeed;

    // Diagonal movement should not be faster than straight movement.
    if (moveX != 0 && moveY != 0)
    {
        moveX = moveX * 0.7071f;
        moveY = moveY * 0.7071f;
    }

    float newX = enforcer.getPosition().x + moveX;
    float newY = enforcer.getPosition().y + moveY;

    if (newX < minX) newX = minX;
    if (newX > maxX) newX = maxX;
    if (newY < enforcerRadius) newY = enforcerRadius;
    if (newY > windowHeight - enforcerRadius) newY = windowHeight - enforcerRadius;

    movedX = newX - enforcer.getPosition().x;
    movedY = newY - enforcer.getPosition().y;
    enforcer.setPosition(Vector2f(newX, newY));
}

void hitPuckWithEnforcer(CircleShape& puck, CircleShape& enforcer, float& puckSpeedX, float& puckSpeedY,
                         float enforcerMoveX, float enforcerMoveY)
{
    float differenceX = puck.getPosition().x - enforcer.getPosition().x;
    float differenceY = puck.getPosition().y - enforcer.getPosition().y;
    float distance = sqrt(differenceX * differenceX + differenceY * differenceY);
    float touchDistance = enforcerRadius + puckRadius;

    if (distance < touchDistance && distance > 0)
    {
        float directionX = differenceX / distance;
        float directionY = differenceY / distance;

        // pushes the puck out so it does not stay inside the enforcer
        puck.setPosition(Vector2f(enforcer.getPosition().x + directionX * touchDistance,
                                  enforcer.getPosition().y + directionY * touchDistance));

        puckSpeedX = directionX * hitSpeed + enforcerMoveX * 0.8f;
        puckSpeedY = directionY * hitSpeed + enforcerMoveY * 0.8f;

        // limit the top speed
        float speed = sqrt(puckSpeedX * puckSpeedX + puckSpeedY * puckSpeedY);
        if (speed > maxPuckSpeed)
        {
            puckSpeedX = puckSpeedX / speed * maxPuckSpeed;
            puckSpeedY = puckSpeedY / speed * maxPuckSpeed;
        }
    }
}


int main()
{
    srand(time(0));   // different random serve every time the program runs

    RenderWindow window(VideoMode({windowWidth, windowHeight}), "Mini Ice Hockey Arcade");
    window.setFramerateLimit(60);

    // Try several font locations so the game does not instantly quit if one is missing.
    const int fontPathCount = 3;
    string fontPaths[fontPathCount] = {
        "C:/Windows/Fonts/arial.ttf",
        "arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    };

    Font font;
    bool fontLoaded = false;
    for (int i = 0; i < fontPathCount && !fontLoaded; i++)
    {
        fontLoaded = font.openFromFile(fontPaths[i]);
    }
    if (!fontLoaded)
    {
        return -1;
    }

    // drawing rink
    RectangleShape iceSurface(Vector2f(windowWidth, windowHeight));
    iceSurface.setFillColor(Color(200, 230, 255));

    RectangleShape centerLine(Vector2f(4, windowHeight));
    centerLine.setFillColor(Color(220, 40, 40));
    centerLine.setPosition(Vector2f(windowWidth / 2 - 2, 0));

    CircleShape centerCircle(70);
    centerCircle.setFillColor(Color::Transparent);
    centerCircle.setOutlineThickness(4);
    centerCircle.setOutlineColor(Color(40, 80, 220));
    centerCircle.setOrigin(Vector2f(70, 70));
    centerCircle.setPosition(Vector2f(windowWidth / 2, windowHeight / 2));

    // player 1 goal
    RectangleShape goal1(Vector2f(14, goalHeight));
    goal1.setFillColor(Color(30, 90, 220));
    goal1.setPosition(Vector2f(0, goalTop));

    // player 2 goal
    RectangleShape goal2(Vector2f(14, goalHeight));
    goal2.setFillColor(Color(220, 50, 50));
    goal2.setPosition(Vector2f(windowWidth - 14, goalTop));

    // creating player 1
    CircleShape player1(enforcerRadius);
    player1.setFillColor(Color(30, 90, 220));
    player1.setOutlineThickness(4);
    player1.setOutlineColor(Color(10, 40, 130));
    player1.setOrigin(Vector2f(enforcerRadius, enforcerRadius));

    // creating player 2
    CircleShape player2(enforcerRadius);
    player2.setFillColor(Color(220, 50, 50));
    player2.setOutlineThickness(4);
    player2.setOutlineColor(Color(130, 20, 20));
    player2.setOrigin(Vector2f(enforcerRadius, enforcerRadius));

    // creating puck
    CircleShape puck(puckRadius);
    puck.setFillColor(Color::Black);
    puck.setOrigin(Vector2f(puckRadius, puckRadius));

    // creating menu
    RectangleShape menuBackground(Vector2f(windowWidth, windowHeight));
    menuBackground.setFillColor(Color(12, 20, 45));

    RectangleShape menuBorder(Vector2f(windowWidth - 30, windowHeight - 30));
    menuBorder.setPosition(Vector2f(15, 15));
    menuBorder.setFillColor(Color::Transparent);
    menuBorder.setOutlineThickness(3);
    menuBorder.setOutlineColor(Color(40, 70, 130));

    CircleShape menuCircle(150);
    menuCircle.setOrigin(Vector2f(150, 150));
    menuCircle.setPosition(Vector2f(windowWidth / 2, windowHeight / 2));
    menuCircle.setFillColor(Color::Transparent);
    menuCircle.setOutlineThickness(3);
    menuCircle.setOutlineColor(Color(25, 45, 90));

    RectangleShape accentLine(Vector2f(300, 4));
    accentLine.setFillColor(Color(220, 40, 40));
    accentLine.setPosition(Vector2f(windowWidth / 2 - 150, 212));

    RectangleShape button(Vector2f(360, 56));
    button.setOutlineThickness(3);

    RectangleShape panel(Vector2f(660, 330));
    panel.setPosition(Vector2f(windowWidth / 2 - 330, 150));
    panel.setFillColor(Color(18, 32, 68));
    panel.setOutlineThickness(3);
    panel.setOutlineColor(Color(150, 205, 255));

    RectangleShape resultPanel(Vector2f(600, 300));
    resultPanel.setPosition(Vector2f(windowWidth / 2 - 300, 150));
    resultPanel.setFillColor(Color(250, 250, 255));
    resultPanel.setOutlineThickness(5);
    resultPanel.setOutlineColor(Color(40, 80, 220));

    // Decorative puck that bounces around behind the menu
    CircleShape menuPuck(puckRadius);
    menuPuck.setOrigin(Vector2f(puckRadius, puckRadius));
    menuPuck.setFillColor(Color(230, 240, 255));
    menuPuck.setPosition(Vector2f(150, 480));
    float menuPuckSpeedX = 4;
    float menuPuckSpeedY = 3;

    // Texts
    Text titleText(font, "MINI ICE HOCKEY", 66);
    titleText.setFillColor(Color::White);
    titleText.setOutlineThickness(3);
    titleText.setOutlineColor(Color(30, 90, 220));
    titleText.setLetterSpacing(1.5f);
    centerText(titleText, windowWidth / 2, 105);

    Text subtitleText(font, "A R C A D E", 28);
    subtitleText.setFillColor(Color(150, 205, 255));
    centerText(subtitleText, windowWidth / 2, 172);

    Text startText(font, "START GAME", 26);
    Text controlsButtonText(font, "CONTROLS", 26);
    Text exitText(font, "EXIT", 26);

    Text hintText(font, "W / S or UP / DOWN to move     ENTER to select", 16);
    hintText.setFillColor(Color(110, 135, 190));
    centerText(hintText, windowWidth / 2, 525);

    Text footerText(font, "Zubair Ali  |  Muhammad Hassan  |  Muhammad Wasif Ali   -   BCS-1E", 15);
    footerText.setFillColor(Color(85, 105, 155));
    centerText(footerText, windowWidth / 2, 565);

    Text controlsTitle(font, "CONTROLS", 36);
    controlsTitle.setFillColor(Color::White);
    centerText(controlsTitle, windowWidth / 2, 190);

    Text controlsLine1(font, "PLAYER 1 (BLUE)        W  A  S  D", 24);
    controlsLine1.setFillColor(Color(90, 150, 255));
    centerText(controlsLine1, windowWidth / 2, 255);

    Text controlsLine2(font, "PLAYER 2 (RED)        ARROW KEYS", 24);
    controlsLine2.setFillColor(Color(255, 100, 100));
    centerText(controlsLine2, windowWidth / 2, 305);

    Text controlsLine3(font, "Hit the puck into the opponent's goal.", 20);
    controlsLine3.setFillColor(Color(200, 215, 245));
    centerText(controlsLine3, windowWidth / 2, 360);

    Text controlsLine4(font, "First to " + to_string(winningScore) + " goals wins the match.   ESC returns to the menu.", 20);
    controlsLine4.setFillColor(Color(200, 215, 245));
    centerText(controlsLine4, windowWidth / 2, 395);

    Text controlsBack(font, "Press ENTER or ESC to go back", 18);
    controlsBack.setFillColor(Color(110, 135, 190));
    centerText(controlsBack, windowWidth / 2, 440);

    Text scoreText(font, "0 : 0", 50);
    scoreText.setFillColor(Color::Black);
    updateScoreText(scoreText, 0, 0);

    Text timerText(font, "3:00", 30);
    timerText.setFillColor(Color::Black);
    updateTimerText(timerText, matchTimeFrames);

    Text reasonText(font, "", 36);
    reasonText.setFillColor(Color::Black);

    Text goalText(font, "GOAL!", 80);
    goalText.setFillColor(Color(220, 140, 0));
    centerText(goalText, windowWidth / 2, windowHeight / 2 - 120);

    Text countdownText(font, "3", 100);
    countdownText.setFillColor(Color(40, 80, 220));

    Text winnerText(font, "", 60);

    Text backText(font, "Press ENTER for Main Menu", 28);
    backText.setFillColor(Color::Black);
    centerText(backText, windowWidth / 2, 400);

    // Game variables
    int gameState = menuState;
    int menuSelection = optionStart;
    bool showControls = false;
    int player1Score = 0;
    int player2Score = 0;
    int goalPause = 0;        // counts down after a goal, 0 means normal play
    int countdown = 0;        // counts down before the puck is served, 0 means puck is live
    int nextServe = 0;        // direction of the next serve
    int timeLeftFrames = matchTimeFrames;
    float puckSpeedX = 0;
    float puckSpeedY = 0;

    while (window.isOpen())
    {
        //  Events
        while (const auto event = window.pollEvent())
        {
            if (event->is<Event::Closed>())
            {
                window.close();
            }

            if (const auto* keyPressed = event->getIf<Event::KeyPressed>())
            {
                if (gameState == menuState)
                {
                    if (showControls)
                    {
                        // Controls screen,go back to the menu
                        if (keyPressed->code == Keyboard::Key::Enter || keyPressed->code == Keyboard::Key::Escape)
                        {
                            showControls = false;
                        }
                    }
                    else
                    {
                        if (keyPressed->code == Keyboard::Key::Up || keyPressed->code == Keyboard::Key::W)
                        {
                            menuSelection = (menuSelection + optionCount - 1) % optionCount;
                        }
                        if (keyPressed->code == Keyboard::Key::Down || keyPressed->code == Keyboard::Key::S)
                        {
                            menuSelection = (menuSelection + 1) % optionCount;
                        }
                        if (keyPressed->code == Keyboard::Key::Escape)
                        {
                            window.close();
                        }
                        if (keyPressed->code == Keyboard::Key::Enter)
                        {
                            if (menuSelection == optionStart)
                            {
                                player1Score = 0;
                                player2Score = 0;
                                goalPause = 0;
                                nextServe = 0;               // random side for the first serve
                                countdown = countdownFrames;
                                timeLeftFrames = matchTimeFrames;
                                resetEnforcers(player1, player2);
                                resetPuck(puck, puckSpeedX, puckSpeedY);
                                updateScoreText(scoreText, player1Score, player2Score);
                                updateTimerText(timerText, timeLeftFrames);
                                gameState = playingState;
                            }
                            else if (menuSelection == optionControls)
                            {
                                showControls = true;
                            }
                            else if (menuSelection == optionExit)
                            {
                                window.close();
                            }
                        }
                    }
                }
                else if (gameState == playingState)
                {
                    if (keyPressed->code == Keyboard::Key::Escape)
                    {
                        gameState = menuState;
                    }
                }
                else if (gameState == gameOverState)
                {
                    if (keyPressed->code == Keyboard::Key::Enter)
                    {
                        gameState = menuState;
                    }
                }
            }
        }

        //  Update
        if (gameState == menuState)
        {
            // Animate the decorative puck
            menuPuck.move(Vector2f(menuPuckSpeedX, menuPuckSpeedY));

            if (menuPuck.getPosition().x < 40 || menuPuck.getPosition().x > windowWidth - 40)
            {
                menuPuckSpeedX = -menuPuckSpeedX;
            }
            if (menuPuck.getPosition().y < 40 || menuPuck.getPosition().y > windowHeight - 40)
            {
                menuPuckSpeedY = -menuPuckSpeedY;
            }
        }
        else if (gameState == playingState)
        {
            if (goalPause > 0)
            {
                // Freeze the game for a moment after a goal.
                goalPause--;

                if (goalPause == 0)
                {
                   if (player1Score == winningScore || player2Score == winningScore)
                    {
                        reasonText.setString("MATCH OVER!");
                        centerText(reasonText, windowWidth / 2, 200);
                        setWinnerText(winnerText, player1Score, player2Score);
                        gameState = gameOverState;
                    }
                    else
                    {
                        resetPuck(puck, puckSpeedX, puckSpeedY);
                        resetEnforcers(player1, player2);
                        countdown = countdownFrames;   // start a new 3-2-1 countdown
                    }
                }
            }
            else
            {
                float player1MovedX, player1MovedY, player2MovedX, player2MovedY;

                moveEnforcer(player1,
                             Keyboard::isKeyPressed(Keyboard::Key::W),
                             Keyboard::isKeyPressed(Keyboard::Key::S),
                             Keyboard::isKeyPressed(Keyboard::Key::A),
                             Keyboard::isKeyPressed(Keyboard::Key::D),
                             enforcerRadius, windowWidth / 2 - enforcerRadius,
                             player1MovedX, player1MovedY);

                moveEnforcer(player2,
                             Keyboard::isKeyPressed(Keyboard::Key::Up),
                             Keyboard::isKeyPressed(Keyboard::Key::Down),
                             Keyboard::isKeyPressed(Keyboard::Key::Left),
                             Keyboard::isKeyPressed(Keyboard::Key::Right),
                             windowWidth / 2 + enforcerRadius, windowWidth - enforcerRadius,
                             player2MovedX, player2MovedY);

                if (countdown > 0)
                {
                    // Players may move, but the puck waits for the serve.
                    countdown--;

                    countdownText.setString(to_string((countdown + 39) / 40));
                    centerText(countdownText, windowWidth / 2, windowHeight / 2 - 120);

                    if (countdown == 0)
                    {
                        servePuck(puckSpeedX, puckSpeedY, nextServe);
                    }
                }
                else
                {
                    hitPuckWithEnforcer(puck, player1, puckSpeedX, puckSpeedY, player1MovedX, player1MovedY);
                    hitPuckWithEnforcer(puck, player2, puckSpeedX, puckSpeedY, player2MovedX, player2MovedY);

                    // Move puck and slow it down
                    puck.move(Vector2f(puckSpeedX, puckSpeedY));

                    puckSpeedX = puckSpeedX * friction;
                    puckSpeedY = puckSpeedY * friction;

                    if (abs(puckSpeedX) < stopSpeed)
                    {
                        puckSpeedX = 0;
                    }
                    if (abs(puckSpeedY) < stopSpeed)
                    {
                        puckSpeedY = 0;
                    }

                    float puckX = puck.getPosition().x;
                    float puckY = puck.getPosition().y;

                    // Top and bottom walls
                    if (puckY - puckRadius <= 0)
                    {
                        puckY = puckRadius;
                        puckSpeedY = abs(puckSpeedY);
                    }
                    if (puckY + puckRadius >= windowHeight)
                    {
                        puckY = windowHeight - puckRadius;
                        puckSpeedY = -abs(puckSpeedY);
                    }

                    // Left and right walls (only solid outside the goal opening)
                    bool insideGoalOpening = (puckY > goalTop && puckY < goalBottom);

                    if (!insideGoalOpening)
                    {
                        if (puckX - puckRadius <= 0)
                        {
                            puckX = puckRadius;
                            puckSpeedX = abs(puckSpeedX);
                        }
                        if (puckX + puckRadius >= windowWidth)
                        {
                            puckX = windowWidth - puckRadius;
                            puckSpeedX = -abs(puckSpeedX);
                        }
                    }

                    puck.setPosition(Vector2f(puckX, puckY));

                    // Goals: the puck must go fully through the opening
                    if (puckX > windowWidth + puckRadius)
                    {
                        player1Score++;
                        nextServe = 1;     // the player who conceded receives the next serve
                        updateScoreText(scoreText, player1Score, player2Score);
                        goalPause = goalPauseFrames;
                    }
                    else if (puckX < -puckRadius)
                    {
                        player2Score++;
                        nextServe = -1;
                        updateScoreText(scoreText, player1Score, player2Score);
                        goalPause = goalPauseFrames;
                    }
                    if (goalPause == 0)
                    {
                        timeLeftFrames--;
                        updateTimerText(timerText, timeLeftFrames);

                        if (timeLeftFrames <= 0)
                        {
                            reasonText.setString("TIME'S UP!");
                            centerText(reasonText, windowWidth / 2, 200);
                            setWinnerText(winnerText, player1Score, player2Score);
                            gameState = gameOverState;
                        }
                    }
                }
            }
        }

        // Draw
        if (gameState == menuState)
        {
            // Drawing menu screen
            window.clear(Color(12, 20, 45));
            window.draw(menuBackground);
            window.draw(menuBorder);
            window.draw(menuCircle);
            window.draw(menuPuck);

            if (showControls)
            {
                window.draw(panel);
                window.draw(controlsTitle);
                window.draw(controlsLine1);
                window.draw(controlsLine2);
                window.draw(controlsLine3);
                window.draw(controlsLine4);
                window.draw(controlsBack);
            }
            else
            {
                window.draw(titleText);
                window.draw(subtitleText);
                window.draw(accentLine);

                drawButton(window, button, startText, 300, menuSelection == optionStart);
                drawButton(window, button, controlsButtonText, 375, menuSelection == optionControls);
                drawButton(window, button, exitText, 450, menuSelection == optionExit);

                window.draw(hintText);
            }

            window.draw(footerText);
        }
        else
        {
            window.clear();
            window.draw(iceSurface);
            window.draw(centerLine);
            window.draw(centerCircle);

            if (gameState == playingState)
            {
                window.draw(goal1);
                window.draw(goal2);
                window.draw(player1);
                window.draw(player2);
                window.draw(puck);
                window.draw(scoreText);
                window.draw(timerText);

                if (goalPause > 0)
                {
                    window.draw(goalText);
                }
                else if (countdown > 0)
                {
                    window.draw(countdownText);
                }
            }
            else if (gameState == gameOverState)
            {
                centerText(winnerText, windowWidth / 2, 265);
                centerText(scoreText, windowWidth / 2, 345);

                window.draw(resultPanel);
                window.draw(reasonText);
                window.draw(winnerText);
                window.draw(scoreText);
                window.draw(backText);
            }
        }

        window.display();
    }

    return 0;
}