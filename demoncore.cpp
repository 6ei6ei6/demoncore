// demoncore.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <cstdint>
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Font.hpp>
int;
constexpr int WINDOW_WIDTH = 1000;
constexpr int WINDOW_HEIGHT = 1000;

constexpr int FPS_MAX = 60;

constexpr int WORLD_WIDTH = 10000;
constexpr int WORLD_HEIGHT = 10000;

constexpr int ENTITY_RADIUS = 10;
constexpr int ENTITY_OUTLINE = 2;

constexpr int PLAYER_HEALTH = 200;

constexpr float PLAYER_SPEED = 200;
constexpr float PLAYER_TURNING = 20;
constexpr float PLAYER_FRICTION = 0.2;
constexpr float PLAYER_COLLISION = 100;

constexpr float GRAB_DISTANCE = 100;
constexpr float GRAB_RELEASE = 5000;

constexpr double DRAG_TIME = 0.1;
constexpr float DRAG_SQDIST = 5000;
constexpr float DROP_FORCE = 1000;

constexpr float ITEM_FRICTION = 0.97;
constexpr float ITEM_COLLISION = 100;

constexpr float ENEMY_FRICTION = 0.8;
constexpr float ENEMY_COLLISION = 100;
constexpr float ENEMY_SPREAD = 200;
constexpr float ENEMY_BOUNCE = 30;
constexpr float ENEMY_LINE = 1000;

const float FLOCK_RADIUS = 2000;
const float FLOCK_SPEED = 20;
const float FLOCK_SEPARATION = 10.0;
const float FLOCK_ALIGNMENT = 10.0;
const float FLOCK_COHESION = 5.0;

constexpr float BULLET_SPEED = 400;
constexpr float BULLET_MUZZLE = 20;
constexpr float BULLET_FORCE = 0.3;
constexpr float BULLET_TRAIL = 0.05;
constexpr float BULLET_SQDIST = 5000;
constexpr float BULLET_COLLISION = 100;

constexpr float DEATH_SPEED = 100;
constexpr float DEATH_RADIUS = 30;
constexpr float DEATH_SPREAD = 1;
constexpr float DEATH_FORCE = 5;

constexpr float REGEN_TIME = 1;
constexpr float REGEN_SPEED = 100;

constexpr float DEADZONE_WIDTH = 100;
constexpr float DEADZONE_HEIGHT = 100;

constexpr float HUD_SIZE = 40;

constexpr size_t ITEM_MAX = 100;
constexpr size_t ENEMY_MAX = 2000;
constexpr size_t BULLET_MAX = 500;

// GAME STATES
constexpr uint8_t GAME_PAUSE = 1 << 0;
constexpr uint8_t GAME_DEBUG = 1 << 1;
constexpr uint8_t GAME_VFX = 1 << 2;
constexpr uint8_t GAME_HUD = 1 << 3;
// ....

// INPUT FLAGS
constexpr uint16_t INPUT_MOVERIGHT = 1 << 0;
constexpr uint16_t INPUT_MOVEDOWN = 1 << 1;
constexpr uint16_t INPUT_MOVELEFT = 1 << 2;
constexpr uint16_t INPUT_MOVEUP = 1 << 3;
constexpr uint16_t INPUT_SHOOT = 1 << 4;
constexpr uint16_t INPUT_GRAB = 1 << 5;
constexpr uint16_t INPUT_DRAG = 1 << 6;
constexpr uint16_t INPUT_DROP = 1 << 7;
constexpr uint16_t INPUT_ZOOMIN = 1 << 8;
constexpr uint16_t INPUT_ZOOMOUT = 1 << 9;

// ENTITY STATES
constexpr uint8_t STATE_ALIVE = 1 << 0;
constexpr uint8_t STATE_MOVING = 1 << 1;
constexpr uint8_t STATE_GRABBED = 1 << 2;
constexpr uint8_t STATE_HURT = 1 << 3;
// spares......

// ENTITY TYPES
constexpr uint8_t ENTITY_ITEM = 1 << 0;
constexpr uint8_t ENTITY_ENEMY = 1 << 1;
// .....

uint8_t gameState = 0b1110;
uint8_t playerState = 1;
uint16_t playerInput = 0;

float zoom = 1.f;

int enemyCount = 1;
int health = PLAYER_HEALTH;

int grabbed = -1;
int bulletCount = 0;

sf::Clock deltaClock;
sf::Clock dragTimer;
sf::Clock dropTimer;
sf::Clock regenTimer;

sf::Vector2f aim = { 0,0 };
sf::Vector2f dragStart = { 0,0 };

sf::Vector2f position = { WORLD_WIDTH * 0.5 , WORLD_HEIGHT * 0.5 };
sf::Vector2f velocity = { 0,0 };

sf::FloatRect deadZone;

sf::Font font;

sf::Event event;

sf::CircleShape player(ENTITY_RADIUS, 3);
sf::CircleShape item(ENTITY_RADIUS, 4);
sf::CircleShape enemy(ENTITY_RADIUS, 3);
sf::CircleShape bullet(ENTITY_RADIUS, 3);
sf::CircleShape vfx(ENTITY_RADIUS, 3);

sf::Text HUD("HUD", font, HUD_SIZE);

sf::RenderWindow window(sf::VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }), "demoncore");
sf::View view({ 0.f,200.f }, { WINDOW_WIDTH, WINDOW_HEIGHT });
const sf::View pauseView = window.getDefaultView();
const sf::View hudView = window.getDefaultView();

struct ItemPool
{
	uint8_t state[ITEM_MAX];
	sf::Vector2f position[ITEM_MAX];
	sf::Vector2f velocity[ITEM_MAX];
} items;

struct EnemyPool
{
	uint8_t state[ENEMY_MAX];
	int health[ENEMY_MAX];
	sf::Vector2f position[ENEMY_MAX];
	sf::Vector2f velocity[ENEMY_MAX];
} enemies;

struct BulletPool
{
	uint8_t state[BULLET_MAX];
	sf::Vector2f position[BULLET_MAX];
	sf::Vector2f velocity[BULLET_MAX];
	float rotation[BULLET_MAX];
} bullets;

int main()
{
	if (!font.loadFromFile("font.ttf"))
	{
		std::cout << "Error loading font!" << std::endl;
		return -1;
	}

	for (int i = 0; i < ENEMY_MAX; i++)
	{
		enemies.state[i] = 0b1;
		enemies.health[i] = 100;
		enemies.position[i] = { static_cast<float>(rand() % WORLD_WIDTH), static_cast<float>(rand() % WORLD_HEIGHT) };
		enemies.velocity[i] = { 0,0 };
	}

	for (int i = 0; i < ITEM_MAX; i++)
	{
		items.state[i] = 0b1;
		items.position[i] = { static_cast<float>(rand() % WORLD_WIDTH), static_cast<float>(rand() % WORLD_HEIGHT) };
		items.velocity[i] = { 0,0 };
	}

	for (int i = 0; i < BULLET_MAX; i++)
	{
		bullets.state[i] = 0b0;
		bullets.position[i] = { 0,0 };
		bullets.velocity[i] = { 0,0 };
	}

	for (size_t i = 0; i < ENEMY_MAX; i++)
	{
		if (enemies.state[i] & STATE_ALIVE) enemyCount++;
		else enemyCount--;
	}

	deadZone.width = DEADZONE_WIDTH;
	deadZone.height = DEADZONE_HEIGHT;

	HUD.setOutlineColor(sf::Color::Black);
	HUD.setOutlineThickness(2);
	HUD.setLetterSpacing(5);

	vfx.setOrigin({ ENTITY_RADIUS, ENTITY_RADIUS });
	vfx.setFillColor(sf::Color::Transparent);
	vfx.setOutlineColor(sf::Color::Transparent);
	vfx.setOutlineThickness(ENTITY_OUTLINE);
	
	item.setOrigin({ ENTITY_RADIUS, ENTITY_RADIUS });
	item.setFillColor(sf::Color::Green);
	item.setOutlineColor(sf::Color::Transparent);
	item.setOutlineThickness(ENTITY_OUTLINE);

	enemy.setOrigin({ ENTITY_RADIUS, ENTITY_RADIUS });
	enemy.setFillColor(sf::Color::Red);
	enemy.setOutlineColor(sf::Color::Transparent);
	enemy.setOutlineThickness(ENTITY_OUTLINE);

	bullet.setOrigin({ ENTITY_RADIUS, ENTITY_RADIUS });
	bullet.setFillColor(sf::Color::Red);
	bullet.setOutlineColor(sf::Color::Transparent);
	bullet.setOutlineThickness(ENTITY_OUTLINE);

	player.setOrigin({ ENTITY_RADIUS, ENTITY_RADIUS });
	player.setPosition(position);
	player.setFillColor(sf::Color::Blue);
	player.setOutlineColor(sf::Color::Transparent);
	player.setOutlineThickness(ENTITY_OUTLINE);

	view.setCenter(player.getPosition());
	window.setFramerateLimit(FPS_MAX);
	
	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	while (window.isOpen())
	{
		float deltaTime = deltaClock.restart().asSeconds();
		if (playerState & 0b1) window.clear();

		// UPDATE AIM GLOBAL
		// Expensive calculation. Use sparingly and make the most of it.
		sf::Vector2f dirAim = window.mapPixelToCoords(sf::Mouse::getPosition(window)) - player.getPosition();
		float distSqAim = dirAim.x * dirAim.x + dirAim.y * dirAim.y;
		float distAim = std::sqrt(distSqAim);
		if (distAim == 0.0f) distAim = 0.001f;
		sf::Vector2f normalAim = dirAim / distAim;
		aim = normalAim;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// PLAYER INPUT & EVENTS //
		// INPUTS
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed) window.close();

			if (event.type == sf::Event::KeyPressed) if (event.key.code == sf::Keyboard::Escape) gameState ^= GAME_PAUSE;

			if (event.type == sf::Event::KeyPressed) if (event.key.code == sf::Keyboard::End) playerState ^= 0b1;

			if (gameState & GAME_PAUSE || !(playerState & STATE_ALIVE)) break;

			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::D) playerInput |= INPUT_MOVERIGHT;

				if (event.key.code == sf::Keyboard::S) playerInput |= INPUT_MOVEDOWN;

				if (event.key.code == sf::Keyboard::A) playerInput |= INPUT_MOVELEFT;

				if (event.key.code == sf::Keyboard::W) playerInput |= INPUT_MOVEUP;
			}

			if (event.type == sf::Event::KeyReleased)
			{
				if (event.key.code == sf::Keyboard::D) playerInput &= ~INPUT_MOVERIGHT;

				if (event.key.code == sf::Keyboard::S) playerInput &= ~INPUT_MOVEDOWN;

				if (event.key.code == sf::Keyboard::A) playerInput &= ~INPUT_MOVELEFT;

				if (event.key.code == sf::Keyboard::W) playerInput &= ~INPUT_MOVEUP;
			}

			if (event.type == sf::Event::MouseButtonPressed)
			{
				if (event.mouseButton.button == sf::Mouse::Left) playerInput |= INPUT_SHOOT;

				if (event.mouseButton.button == sf::Mouse::Right) playerInput |= INPUT_GRAB;
			}

			if (event.type == sf::Event::MouseButtonReleased)
			{
				if (event.mouseButton.button == sf::Mouse::Left) playerInput &= ~INPUT_SHOOT;

				if (event.mouseButton.button == sf::Mouse::Right)
				{
					playerInput &= ~INPUT_GRAB;
					playerInput |= INPUT_DROP;
				}
			}

			if (event.type == sf::Event::MouseWheelScrolled)
			{
				if (event.mouseWheelScroll.delta > 0) playerInput |= INPUT_ZOOMIN;
				if (event.mouseWheelScroll.delta < 0) playerInput |= INPUT_ZOOMOUT;
			}

			if (event.type == sf::Event::MouseMoved) playerInput |= INPUT_DRAG;
		}

		// HANDLE PLAYER SHOOT INPUT
		// Get next available bullet, set its state, position and velocity.
		if (playerInput & INPUT_SHOOT)
		{
			for (size_t i = 0; i < BULLET_MAX; i++)
			{
				if (bullets.state[i] == 0)
				{
					bullets.state[i] |= 0b1;
					bullets.position[i] = player.getPosition() + aim * BULLET_MUZZLE;
					bullets.velocity[i] = aim * BULLET_SPEED;
					
					// Temp location for set rotation
					float angle = std::atan2(bullets.velocity[i].y, bullets.velocity[i].x) * 180.f / 3.14159265f;
					bullets.rotation[i] = angle + 90;

					health--;
					bulletCount++;
					regenTimer.restart();
					break;
				}
			}
		}

		// HANDLE GRAB INPUT
		// Sets grabbed to hold the index.
		if (playerInput & INPUT_GRAB)
		{
			if (grabbed == -1)
			{
				for (size_t i = 0; i < ITEM_MAX; i++)
				{
					sf::Vector2f dirMouse = window.mapPixelToCoords(sf::Mouse::getPosition(window)) - items.position[i];
					float distSqMouse = dirMouse.x * dirMouse.x + dirMouse.y * dirMouse.y;

					if (distSqMouse < GRAB_DISTANCE)
					{
						grabbed = i;
						items.state[i] |= STATE_GRABBED;
					}
				}
			}
		}

		// HANDLE DRAG INPUT
		if (playerInput & INPUT_DRAG)
		{
			if (playerInput & INPUT_GRAB && grabbed > -1)
			{
				items.position[grabbed] = static_cast<sf::Vector2f>(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
				dragStart = items.position[grabbed];

				dragTimer.restart();
			}
			else if (grabbed > -1)
			{
				if (dragTimer.getElapsedTime().asSeconds() >= DRAG_TIME)
				{
					
					sf::Vector2f dirDrag = static_cast<sf::Vector2f>(window.mapPixelToCoords(sf::Mouse::getPosition(window))) - dragStart;
					float distSqDrag = dirDrag.x * dirDrag.x + dirDrag.y * dirDrag.y;

					if (distSqDrag > DRAG_SQDIST)
					{
						float distDrag = std::sqrt(distSqDrag);
						if (distDrag == 0.0f) distDrag = 0.0001f;
						sf::Vector2f normalDrag = static_cast<sf::Vector2f>(dirDrag) / distDrag;
						
						items.state[grabbed] &= ~STATE_GRABBED;
						items.velocity[grabbed] = normalDrag * DROP_FORCE;
					}

					if (grabbed > -1) grabbed = -1;
				}
			}
		}
		
		// HANDLE DROP INPUT
		// Called when releasing grab.
		// Called once. Handle drop through drag.
		if (playerInput & INPUT_DROP)
		{
			if (grabbed > -1)
			{
				dragTimer.restart();
				// grabbed = -1;
			}
			playerInput &= ~INPUT_DROP;
		}
			  
		// HANDLE PLAYER MOVEMENT INPUT
		// Add vector velocity according to input.
		// Add friction when still.
		// Update player state.
		if (playerInput & 0b1111)
		{
			playerState |= STATE_MOVING;
			if (playerInput & 0b1) velocity += { 1 * PLAYER_TURNING, 0 };
			if (playerInput & 0b10) velocity += { 0, 1 * PLAYER_TURNING };
			if (playerInput & 0b100) velocity -= { 1 * PLAYER_TURNING, 0 };
			if (playerInput & 0b1000) velocity -= { 0, 1 * PLAYER_TURNING };
		}
		else
		{
			playerState &= ~STATE_MOVING;
			velocity *= PLAYER_FRICTION;
			if (std::abs(velocity.x) < 0.01f) velocity.x = 0.f;
			if (std::abs(velocity.y) < 0.01f) velocity.y = 0.f;
		}

		// HANDLE ZOOM
		if (playerInput & INPUT_ZOOMIN)
		{
			view.zoom(0.9f);
			playerInput &= ~INPUT_ZOOMIN;
		}

		if (playerInput & INPUT_ZOOMOUT)
		{
			view.zoom(1.1f);
			playerInput &= ~INPUT_ZOOMOUT;
		}

		view.zoom(zoom);
		if (view.getSize().x < 1 || view.getSize().y < 1) view.setSize({ 1,1 });
		if (view.getSize().x > WORLD_WIDTH || view.getSize().y > WORLD_HEIGHT) view.setSize({ WORLD_WIDTH,WORLD_HEIGHT });

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// UPDATE STATES //
		// UPDATE HEALTH
		if (health == 0)
		{
			playerState &= ~0b1;
			playerInput &= ~0b11111111;
		}

		if (health < PLAYER_HEALTH && regenTimer.getElapsedTime().asSeconds() > REGEN_TIME) health += REGEN_SPEED * deltaTime;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// COLLISIONS AND INTERACTIONS //
		if ((gameState & GAME_PAUSE) == 0 && playerState & STATE_ALIVE)
		{
			// ITEM COLLISION
			for (size_t i = 0; i < ITEM_MAX; i++)
			{
				if (!(items.state[i] & STATE_ALIVE)) continue;

				// ITEM-ENEMY COLLISION
				for (size_t j = 0; j < ENEMY_MAX; j++)
				{
					if (!(enemies.state[j] & STATE_ALIVE)) continue;

					sf::Vector2f dirCol = items.position[i] - enemies.position[j];
					float distSqBCol = dirCol.x * dirCol.x + dirCol.y * dirCol.y;

					if (distSqBCol < ITEM_COLLISION)
					{
						if(!(items.state[i] & STATE_GRABBED)) enemies.state[j] &= ~0b1;

						float distCol = std::sqrt(distSqBCol);
						if (distCol == 0.0f) distCol = 0.001f;
						sf::Vector2f normalCol = dirCol / distCol;

						enemies.position[i] -= normalCol * (30 - distCol);
						//items.state[i] &= ~0b1;
					}
				}
			}

			// BULLET COLLISION
			for (size_t i = 0; i < BULLET_MAX; i++)
			{
				if (!(bullets.state[i] & STATE_ALIVE)) continue;

				// BULLET-ENEMY COLLISION
				for (size_t j = 0; j < ENEMY_MAX; j++)
				{
					if (enemies.state[j] & 0b1)
					{
						sf::Vector2f dirBul = bullets.position[i] - enemies.position[j];
						float distSqBul = dirBul.x * dirBul.x + dirBul.y * dirBul.y;

						if (distSqBul < BULLET_COLLISION)
						{
							enemies.velocity[j] += bullets.velocity[i] * BULLET_FORCE * deltaTime;

							if (playerState & STATE_ALIVE)
							{
								enemies.state[j] &= ~0b1;
								enemyCount--;
							}
							
							bullets.state[i] &= ~0b1;
						}
					}
				}

				// BULLET-ITEM COLLISION
				for (size_t k = 0; k < ITEM_MAX; k++)
				{
					if (!(items.state[k] & STATE_ALIVE)) continue;

					sf::Vector2f dirBul = bullets.position[i] - items.position[k];
					float distSqBul = dirBul.x * dirBul.x + dirBul.y * dirBul.y;

					if (distSqBul < BULLET_COLLISION)
					{
						items.velocity[k] += bullets.velocity[i] * BULLET_FORCE * deltaTime;
						bullets.state[i] &= ~0b1;
					}
				}

				// BULLET-WINDOW FRAME COLLISION
				sf::FloatRect viewBounds;
				sf::View currentView = window.getView();
				viewBounds.left = currentView.getCenter().x - currentView.getSize().x / 2.f;
				viewBounds.top = currentView.getCenter().y - currentView.getSize().y / 2.f;
				viewBounds.width = currentView.getSize().x;
				viewBounds.height = currentView.getSize().y;

				if (!viewBounds.contains(bullets.position[i])) bullets.state[i] &= ~STATE_ALIVE;
			}

			// ENEMY COLLISION & FLOCKING
			for (size_t i = 0; i < ENEMY_MAX; i++)
			{
				if (!(enemies.state[i] & STATE_ALIVE)) continue;

				// ENEMY-PLAYER COLLISION
				sf::Vector2f dirCol = position - enemies.position[i];
				float distSqCol = dirCol.x * dirCol.x + dirCol.y * dirCol.y;

				if (distSqCol < PLAYER_COLLISION)
				{
					//enemies.state[j] &= ~0b1;
					// playerState &= ~0b1; // INVINSIBILITY &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
				}

				// ENEMY-ITEM COLLISION
				for (size_t k = 0; k < ITEM_MAX; k++)
				{
					if (items.state[i] & STATE_ALIVE)
					{

					}
				}

				// ENEMY FLOCK & HORDE BEHAVIOR $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
				sf::Vector2f pos = enemies.position[i];
				sf::Vector2f vel = enemies.velocity[i];

				sf::Vector2f separation(0.f, 0.f);
				sf::Vector2f alignment(0.f, 0.f);
				sf::Vector2f cohesion(0.f, 0.f);
				int neighborCount = 0;

				for (size_t j = 0; j < ENEMY_MAX; j++)
				{
					if (i == j || !(enemies.state[j] & STATE_ALIVE)) continue;

					sf::Vector2f otherPos = enemies.position[j];
					sf::Vector2f offset = otherPos - pos;
					float distSq = offset.x * offset.x + offset.y * offset.y;

					if (distSq < FLOCK_RADIUS * FLOCK_RADIUS)
					{
						float dist = std::sqrt(distSq);
						separation -= offset / (dist + 1.f);
						alignment += enemies.velocity[j];
						cohesion += otherPos;
						neighborCount++;
					}
				}

				if (neighborCount > 0)
				{
					alignment /= static_cast<float>(neighborCount);
					cohesion /= static_cast<float>(neighborCount);
					cohesion -= pos;

					sf::Vector2f steer = separation * FLOCK_SEPARATION + alignment * FLOCK_ALIGNMENT + cohesion * FLOCK_COHESION;
					vel += steer * deltaTime;

					float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
					if (speed > FLOCK_SPEED) vel = (vel / speed) * FLOCK_SPEED;
					enemies.velocity[i] = vel;
				}
			}

			// PLAYER COLLISION
			if (playerState & STATE_ALIVE)
			{
				// PLAYER-ENEMY COLLISION
				for (size_t i = 0; i < ENEMY_MAX; i++)
				{
					if (enemies.state[i] & STATE_ALIVE)
					{
						sf::Vector2f dirCol = position - enemies.position[i];
						float distSqBCol = dirCol.x * dirCol.x + dirCol.y * dirCol.y;

						if (distSqBCol < PLAYER_COLLISION)
						{
							//enemies.state[j] &= ~0b1;
							//playerState &= ~0b1;
						}
					}
				}

				// PLAYER-WINDOW COLLISION
				//if (position.x < 0.f || position.x > static_cast<float>(window.getSize().x)) velocity.x *= -WINDOW_BOUNCE;
				//if (position.y < 0.f || position.y > static_cast<float>(window.getSize().y)) velocity.y *= -WINDOW_BOUNCE;
			}
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// UPDATE PHYSICS //
		// Mostly update arrays and state.
		if ((gameState & GAME_PAUSE) == 0)
		{
			// UPDATE ITEM PHYSICS
			// Adds velocity to the position.
			for (size_t i = 0; i < ITEM_MAX; i++)
			{
				if (!(items.state[i] & STATE_ALIVE)) continue;

				items.position[i] += items.velocity[i] * deltaTime;

				items.velocity[i] *= ITEM_FRICTION;
			}

			// UPDATE ENEMY PHYSICS
			for (size_t i = 0; i < ENEMY_MAX; i++)
			{
				if (!(enemies.state[i] & STATE_ALIVE)) continue;

				enemies.position[i] += enemies.velocity[i] * deltaTime;

				enemies.velocity[i] *= ENEMY_FRICTION;
			}

			// UPDATE BULLET PHYSICS
			for (size_t i = 0; i < BULLET_MAX; i++)
			{
				if (bullets.state[i] & STATE_ALIVE) bullets.position[i] += bullets.velocity[i] * deltaTime;
			}

			// UPDATE PLAYER PHYSICS
			if (playerState & STATE_ALIVE && playerState & STATE_MOVING)
			{
				// Test performance - research alternatives.
				float distSqVel = velocity.x * velocity.x + velocity.y * velocity.y;
				float distVel = std::sqrt(distSqVel);
				if (distVel == 0.0f) distVel = 0.001f;
				velocity = (velocity / distVel) * PLAYER_SPEED;

				position += velocity * deltaTime;

				float velSq = velocity.x * velocity.x + velocity.y * velocity.y;
				if (velSq > 0.01f) playerState |= STATE_MOVING;

				if (playerState & STATE_MOVING) velocity *= ITEM_FRICTION;

				if (std::abs(velocity.x) < 0.01f) velocity.x = 0.f;
				if (std::abs(velocity.y) < 0.01f) velocity.y = 0.f;

				velSq = velocity.x * velocity.x + velocity.y * velocity.y;
				if (velSq == 0.0f) playerState &= ~STATE_MOVING;

				//velocity *= PLAYER_FRICTION;
			}
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// SHAPE SETTING & RENDERING & VFX & DEBUG //
		// ENEMY VFX
		for (size_t i = 0; i < ENEMY_MAX; i++)
		{
			if (enemies.state[i] & 0b1)
			{
				for (size_t k = i + 1; k < ENEMY_MAX; k++)
				{
					if (enemies.state[k] & 0b1)
					{
						sf::Vector2f dirEne = enemies.position[k] - enemies.position[i];
						float distSqEne = dirEne.x * dirEne.x + dirEne.y * dirEne.y;

						if (distSqEne < ENEMY_LINE)
						{
							sf::Vertex enemyLine[] = { sf::Vertex(enemies.position[i], sf::Color::Red), sf::Vertex(enemies.position[k], sf::Color::Red) };
							window.draw(enemyLine, 2, sf::Lines);
						}
					}
				}

			}
		}

		// SHOOT & GRAB VFX
		if (gameState & 0b100 && playerInput & 0b110000)
		{
			// Player-aim vector
			sf::Color vfxColor = sf::Color::White;
			if (playerInput & 0b10000) vfxColor = sf::Color::Red;		// This should be BULLET_COLOR
			if (playerInput & 0b100000) vfxColor = sf::Color::Cyan;		// This should be TK_COLOR

			player.setOutlineColor(vfxColor);

			//sf::Vertex aimLine[] = { sf::Vertex(player.getPosition(), vfxColor), sf::Vertex(player.getPosition() + aim * 50.f, sf::Color::Transparent) };
			//window.draw(aimLine, 2, sf::Lines);

			// Draw entity lines
			if (grabbed > -1)
			{
				sf::Vertex tkLine[] = { sf::Vertex(items.position[grabbed], vfxColor), sf::Vertex(items.position[grabbed] - aim * 50.f, sf::Color::Transparent) };
				window.draw(tkLine, 2, sf::Lines);
			}
			
			// Draw VFX outline
			if (grabbed > -1)
			{
				vfx.setPointCount(4);
				vfx.setPosition(items.position[grabbed]);

				vfx.setOutlineColor(sf::Color::Cyan);
				
				window.draw(vfx);
			}
		}
		else
		{
			player.setOutlineColor(sf::Color::Transparent);

			if (grabbed > -1)
			{
				item.setOutlineColor(sf::Color::Transparent);
			}
		}

		for (size_t i = 0; i < BULLET_MAX; i++)
		{
			if (bullets.state[i] & STATE_ALIVE)
			{
				bullet.setRotation(bullets.rotation[i]);
				bullet.setPosition(bullets.position[i]);
				window.draw(bullet);
			}
		}

		for (size_t i = 0; i < ITEM_MAX; i++)
		{
			if (items.state[i] & STATE_ALIVE)
			{
				item.setPosition(items.position[i]);
				window.draw(item);
			}
		}

		for (size_t i = 0; i < ENEMY_MAX; i++)
		{
			if (enemies.state[i] & STATE_ALIVE)
			{
				sf::Vector2f dirRot = player.getPosition() - enemies.position[i];
				float angle = std::atan2(dirRot.y, dirRot.x) * 180.f / 3.14159265f;
				if (!(enemies.state[i] & STATE_GRABBED)) enemy.setRotation(angle + 90);
				enemy.setPosition(enemies.position[i]);
				window.draw(enemy);
			}
		}
		
		if (playerState & STATE_ALIVE)
		{
			float angle = std::atan2(aim.y, aim.x) * 180.f / 3.14159265f;
			player.setRotation(angle + 90);
			
			player.setFillColor(sf::Color::Blue);
			player.setPosition(position);
		}

		// BULLET VFX
		if (gameState & 0b100)
		{
			for (size_t i = 0; i < BULLET_MAX; i++)
			{
				if (bullets.state[i] & STATE_ALIVE)
				{
					sf::Vector2f dirBul = bullets.position[i] - player.getPosition();
					float distSqBul = dirBul.x * dirBul.x + dirBul.y * dirBul.y;
					if (distSqBul > BULLET_SQDIST)
					{
						//sf::Vertex tracerLine[] = { sf::Vertex(bullets.position[i], sf::Color::Red), sf::Vertex(bullets.position[i] - bullets.velocity[i] * BULLET_TRAIL, sf::Color::Transparent) };
						//window.draw(tracerLine, 2, sf::Lines);
					}
				}
			}
		}

		// HEALTH VFX
		if (gameState & GAME_VFX)
		{
			if (health < PLAYER_HEALTH && playerState & STATE_ALIVE)
			{	
				float hurt = (1.0f - static_cast<float>(health) / PLAYER_HEALTH);
				sf::Color hurtingColor = sf::Color(255 * hurt, 0, 255 * (1 - hurt));
				player.setFillColor(hurtingColor);
			}
			if (!(playerState & STATE_ALIVE)) player.setFillColor(sf::Color::Red);
			if(health == PLAYER_HEALTH) player.setFillColor(sf::Color::Blue);
			
		}

		// DEBUG LINES
		if (gameState & 0b10)
		{
			// Player-velocity vector
			//sf::Vertex moveLine[] = { sf::Vertex(player.getPosition(), sf::Color::White), sf::Vertex(position + velocity * 0.3f, sf::Color::Blue) };
			//window.draw(moveLine, 2, sf::Lines);

			//sf::Vertex aimLine[] = { sf::Vertex(player.getPosition(), sf::Color::White), sf::Vertex(position + aim * 100.f, sf::Color::Red) };
			//window.draw(aimLine, 2, sf::Lines);
		}
		
		if ((gameState & GAME_PAUSE) == 0) window.draw(player);

		// WINDOW & VIEW
		if ((gameState & GAME_PAUSE) == 0)
		{
			deadZone.left = view.getCenter().x - deadZone.width / 2.f;
			deadZone.top = view.getCenter().y - deadZone.height / 2.f;
			
			if (position.x < deadZone.left) view.move(position.x - deadZone.left, 0.f);
			else if (position.x > deadZone.left + deadZone.width) view.move(position.x - (deadZone.left + deadZone.width), 0.f);

			if (position.y < deadZone.top) view.move(0.f, position.y - deadZone.top);
			else if (position.y > deadZone.top + deadZone.height) view.move(0.f, position.y - (deadZone.top + deadZone.height));
		}
		
		// HUD & USER INTERFACE *******************************************************************************************************
		// HUD & USER INTERFACE *******************************************************************************************************
		// HUD & USER INTERFACE *******************************************************************************************************
		// HUD & USER INTERFACE *******************************************************************************************************
		// PLAYER HEALTH
		if (gameState & GAME_HUD)
		{
			window.setView(pauseView);

			HUD.setString("HP " + std::to_string(health * 100 / PLAYER_HEALTH));
			if (!(playerState & STATE_ALIVE)) HUD.setString("HP 0");

			HUD.setFillColor(sf::Color::Red);
			if (health > PLAYER_HEALTH * 0.20) HUD.setFillColor(sf::Color::Yellow);
			if (health > PLAYER_HEALTH * 0.70) HUD.setFillColor(sf::Color::Green);
			if (!(playerState & STATE_ALIVE)) HUD.setFillColor(sf::Color::Red);

			//HUD.setPosition(player.getPosition()); // Must be on same view.

			window.draw(HUD);
		}

		// GAME OVER SCREEN ***********************************************************************************************************
		// GAME OVER SCREEN ***********************************************************************************************************
		// GAME OVER SCREEN ***********************************************************************************************************
		// GAME OVER SCREEN ***********************************************************************************************************
		// DRAW TEXT & CALCULATE KILL AREA
		if ((playerState & STATE_ALIVE) == 0)
		{
			window.setView(pauseView);

			player.setFillColor(sf::Color::Red);

			sf::Text overText("GAME OVER", font, 50);
			overText.setFillColor(sf::Color::Black);
			overText.setLetterSpacing(5);
			overText.setPosition(WINDOW_WIDTH * 0.5, WINDOW_HEIGHT * 0.5);
			overText.setOrigin(overText.getLocalBounds().width * 0.5, overText.getLocalBounds().height * 0.5);
			window.draw(overText);

			//sf::Vector2f topLeft = { overText.getPosition().x - overText.getLocalBounds().width * 0.5f, overText.getPosition().y - overText.getLocalBounds().height * 0.5f };
			//sf::Vector2f topRight = { overText.getPosition().x + overText.getLocalBounds().width * 0.5f, overText.getPosition().y - overText.getLocalBounds().height * 0.5f };
			//sf::Vector2f bottomLeft = { overText.getPosition().x - overText.getLocalBounds().width * 0.5f, overText.getPosition().y + overText.getLocalBounds().height * 0.5f };
			//sf::Vector2f bottomRight = { overText.getPosition().x + overText.getLocalBounds().width * 0.5f, overText.getPosition().y + overText.getLocalBounds().height * 0.5f };

			window.draw(overText);
		}

		// PLAYER DEATH VFX //
		// SPAWN BULLET VFX
		if ((playerState & STATE_ALIVE) == 0)
		{
			for (size_t i = 0; i < BULLET_MAX; i++)
			{
				if (bullets.state[i] == 0)
				{
					float deathSpin = rand() % 360 * DEATH_SPREAD;
					if (deathSpin > 360.f) deathSpin -= 360.f;

					float radAngle = deathSpin * 3.14159265f / 180.f;
					sf::Vector2f dirAngle = { std::cos(radAngle), std::sin(radAngle) };

					bullets.state[i] = 1;
					bullets.position[i] = position + dirAngle * DEATH_RADIUS;
					bullets.velocity[i] = dirAngle * DEATH_SPEED;

					float angle = std::atan2(bullets.velocity[i].y, bullets.velocity[i].x) * 180.f / 3.14159265f;
					bullets.rotation[i] = angle + 90;
					break;
				}
				
			}

			// DEATH VFX COLLISION
			for (size_t i = 0; i < BULLET_MAX; i++)
			{
				if (!(bullets.state[i] & STATE_ALIVE)) continue;

				// DEATH-ENEMY COLLISION
				for (size_t j = 0; j < ENEMY_MAX; j++)
				{
					if (enemies.state[j] & 0b1)
					{
						sf::Vector2f dirBul = bullets.position[i] - enemies.position[j];
						float distSqBul = dirBul.x * dirBul.x + dirBul.y * dirBul.y;

						if (distSqBul < BULLET_COLLISION)
						{
							enemies.velocity[j] += bullets.velocity[i] * DEATH_FORCE * deltaTime;
							bullets.state[i] &= ~0b1;
						}
					}
				}

				// DEATH-ITEM COLLISION
				for (size_t k = 0; k < ITEM_MAX; k++)
				{
					if (!(items.state[k] & STATE_ALIVE)) continue;

					sf::Vector2f dirBul = bullets.position[i] - items.position[k];
					float distSqBul = dirBul.x * dirBul.x + dirBul.y * dirBul.y;

					if (distSqBul < BULLET_COLLISION)
					{
						items.velocity[k] += bullets.velocity[i] * DEATH_FORCE * deltaTime;
						bullets.state[i] &= ~0b1;
					}
				}
			}
		}

		// PAUSE MENU SCREEN **********************************************************************************************************
		// PAUSE MENU SCREEN **********************************************************************************************************
		// PAUSE MENU SCREEN **********************************************************************************************************
		// PAUSE MENU SCREEN **********************************************************************************************************
		if (gameState & GAME_PAUSE)
		{
			window.clear();
			window.setView(pauseView);

			sf::Text pauseText("The game is paused. Press [Esc] to continue.", font, 24);
			sf::Text triangleText("Enemy", font, 24);
			sf::Text squareText("Item", font, 24);
			sf::Text circleText("Player", font, 24);

			pauseText.setFillColor(sf::Color::White);
			pauseText.setPosition(WINDOW_WIDTH * 0.5, WINDOW_HEIGHT * 0.2);
			pauseText.setOrigin(pauseText.getLocalBounds().width * 0.5, pauseText.getLocalBounds().height * 0.5);

			triangleText.setFillColor(sf::Color::White);
			triangleText.setPosition(WINDOW_WIDTH * 0.25, WINDOW_HEIGHT * 0.7);
			triangleText.setOrigin(triangleText.getLocalBounds().width * 0.5, triangleText.getLocalBounds().height * 0.5);

			squareText.setFillColor(sf::Color::White);
			squareText.setPosition(WINDOW_WIDTH * 0.5, WINDOW_HEIGHT * 0.7);
			squareText.setOrigin(squareText.getLocalBounds().width * 0.5, squareText.getLocalBounds().height * 0.5);

			circleText.setFillColor(sf::Color::White);
			circleText.setPosition(WINDOW_WIDTH * 0.75, WINDOW_HEIGHT * 0.7);
			circleText.setOrigin(circleText.getLocalBounds().width * 0.5, circleText.getLocalBounds().height * 0.5);

			sf::CircleShape triangle(100.f, 3);
			sf::CircleShape square(100.f, 4);
			sf::CircleShape triangle2(100.f, 3);

			triangle.setOrigin({ 100, 100 });
			triangle.setFillColor(sf::Color::Red);
			triangle.setPosition(WINDOW_WIDTH * 0.25, WINDOW_HEIGHT * 0.5);

			square.setOrigin({ 100, 100 });
			square.setFillColor(sf::Color::Green);
			square.setPosition(WINDOW_WIDTH * 0.5, WINDOW_HEIGHT * 0.5);

			triangle2.setOrigin({ 100, 100 });
			triangle2.setFillColor(sf::Color::Blue);
			triangle2.setPosition(WINDOW_WIDTH * 0.75, WINDOW_HEIGHT * 0.5);

			std::string countString;
			if (enemyCount > 1 && enemyCount < ENEMY_MAX) countString = std::to_string(enemyCount) + " enemies left";
			else if(enemyCount == 1 && enemyCount != ENEMY_MAX) countString = "only 1 enemy left";

			sf::Text countText(countString, font, 24);
			countText.setFillColor(sf::Color::Red);
			//countText.setPosition(WINDOW_WIDTH * 0.5, WINDOW_HEIGHT * 0.3);
			//countText.setOrigin(countText.getLocalBounds().width * 0.5, countText.getLocalBounds().height * 0.5);

			window.draw(triangle);
			window.draw(square);
			window.draw(triangle2);
			window.draw(pauseText);
			window.draw(triangleText);
			window.draw(squareText);
			window.draw(circleText);
			window.draw(countText);
		}
		
		window.setView(view);
        window.display();
    }

    return 0;
};