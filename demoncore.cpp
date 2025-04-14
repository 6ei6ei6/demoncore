// demoncore.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <cstdint>
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Font.hpp>

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
constexpr float GRAB_SPRING = 5;
constexpr float DROP_FORCE = 3;

constexpr float ITEM_FRICTION = 0.9f;

constexpr float ENEMY_FRICTION = 0.8;
constexpr float ENEMY_SPREAD = 200;
constexpr float ENEMY_BOUNCE = 30;
constexpr float ENEMY_LINE = 1000;

constexpr const float FLOCK_RADIUS = 200;
constexpr const float FLOCK_HUDDLE = 100;
constexpr const float FLOCK_SPEED = 200;
constexpr const float FLOCK_SEPARATION = 3.0;
constexpr const float FLOCK_ALIGNMENT = 10.0;
constexpr const float FLOCK_COHESION = 5.0;
/*
LEVELS:
// NAME
- FLOCK RADIUS 300


// SPIKES
const float FLOCK_RADIUS = 2000;
const float FLOCK_SPEED = 20;
const float FLOCK_SEPARATION = 10.0;
const float FLOCK_ALIGNMENT = 10.0;
const float FLOCK_COHESION = 5.0;
Red to Black lines between all.
*/

constexpr float BULLET_SPEED = 400;
constexpr float BULLET_MUZZLE = 20;
constexpr float BULLET_FORCE = 0.8;
constexpr float BULLET_TRAIL = 0.05;
constexpr float BULLET_SQDIST = 5000;

constexpr float COLLISION_ITEM_ENEMY = 100;
constexpr float COLLISION_ITEM_PLAYER = 50;
constexpr float COLLISION_BULLET_ITEM = 100;
constexpr float COLLISION_BULLET_ENEMY = 50;

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
constexpr size_t ENEMY_MAX = 1000;
constexpr size_t BULLET_MAX = 500;
constexpr size_t ENTITY_MAX = ITEM_MAX + ENEMY_MAX + BULLET_MAX;

// GAME STATES
constexpr uint8_t GAME_PAUSE = 1 << 0;
constexpr uint8_t GAME_DEBUG = 1 << 1;
constexpr uint8_t GAME_VFX = 1 << 2;
constexpr uint8_t GAME_HUD = 1 << 3;
constexpr uint8_t GAME_STATS = 1 << 4;
// ....

/*
GAME
1 - pause
2 - hud
3 - 
4 - 
5 - 
6 - 
7 - 
8 - over

ENGINE
1 - debug
2 - input
3 - physics
4 - collisions
5 - rendering
6 - vfx
7 - 
8 - 
*/

// INPUT FLAGS
constexpr uint16_t INPUT_MOVERIGHT = 1 << 0;
constexpr uint16_t INPUT_MOVEDOWN = 1 << 1;
constexpr uint16_t INPUT_MOVELEFT = 1 << 2;
constexpr uint16_t INPUT_MOVEUP = 1 << 3;
constexpr uint16_t INPUT_SHOOT = 1 << 4;
constexpr uint16_t INPUT_GRAB = 1 << 5;
constexpr uint16_t INPUT_DROP = 1 << 6;
//constexpr uint16_t INPUT_DROP = 1 << 7;
constexpr uint16_t INPUT_ZOOMIN = 1 << 8;
constexpr uint16_t INPUT_ZOOMOUT = 1 << 9;

// ENTITY STATES
constexpr uint8_t STATE_ALIVE = 1 << 0;
constexpr uint8_t STATE_MOVING = 1 << 1;
constexpr uint8_t STATE_GRABBED = 1 << 2;
constexpr uint8_t STATE_HURT = 1 << 3;
// spares......

// ALIVE - Rendering
// SLEEP - No physics
// 

// ENTITY TYPES
constexpr uint8_t ENTITY_ITEM = 1 << 0;
constexpr uint8_t ENTITY_ENEMY = 1 << 1;
// .....

uint8_t gameState = 0b1110;
uint8_t playerState = 1;
uint16_t playerInput = 0;

float zoom = 1.f;

int health = PLAYER_HEALTH;



int grabbedID = -1;



sf::Clock deltaClock;


sf::Clock regenTimer;

sf::Vector2f aim = { 0,0 };


sf::Vector2f position = { WORLD_WIDTH * 0.5 , WORLD_HEIGHT * 0.5 };
sf::Vector2f velocity = { 0,0 };

sf::Vector2f mousePos = { 0,0 };

sf::FloatRect deadZone;

// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

sf::Vector2f positions[ENTITY_MAX];
sf::Vector2f velocities[ENTITY_MAX];

float rotations[ENTITY_MAX];

uint8_t states[ENTITY_MAX];

int itemIDs[ITEM_MAX];
int enemyIDs[ENEMY_MAX];
int bulletIDs[BULLET_MAX];

int entityCount = 0;
int itemCount = 0;
int enemyCount = 0;
int bulletCount = 0;

// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

sf::Font font;
sf::Event event;
sf::Text HUD("HUD", font, HUD_SIZE);

sf::CircleShape player(ENTITY_RADIUS, 3);
sf::CircleShape item(ENTITY_RADIUS, 4);
sf::CircleShape enemy(ENTITY_RADIUS, 3);
sf::CircleShape bullet(ENTITY_RADIUS, 3);
sf::CircleShape vfx(ENTITY_RADIUS, 3);

sf::RenderWindow window(sf::VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }), "demoncore");
sf::View view({ 0.f,200.f }, { WINDOW_WIDTH, WINDOW_HEIGHT });
const sf::View pauseView = window.getDefaultView();
const sf::View hudView = window.getDefaultView();

// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// HELPERS //
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// All-encompasing spawn entity function.
int spawnEntity(sf::Vector2f pos, sf::Vector2f vel, float rot, uint8_t state)
{
	if (entityCount == ENTITY_MAX) return -1;

	int id = entityCount++;

	positions[id] = pos;
	velocities[id] = vel;
	rotations[id] = rot;
	states[id] = state;

	return id;
}

void spawnItem(sf::Vector2f pos)
{
	if (itemCount >= ITEM_MAX) return;

	int id = spawnEntity(pos, {}, 0, STATE_ALIVE);

	if (id != -1) itemIDs[itemCount++] = id;
}

void spawnEnemy(sf::Vector2f pos)
{
	if (enemyCount == ENEMY_MAX) return;

	int id = spawnEntity(pos, {}, 0, STATE_ALIVE);
	if (id != -1) enemyIDs[enemyCount++] = id;
}

void initBullet()
{
	if (bulletCount == BULLET_MAX) return;

	int id = spawnEntity({0,0}, {}, 0, 0);
	if (id != -1) bulletIDs[bulletCount++] = id;
}

void killItem(int i)
{
	if (i < 0 || i >= itemCount) return;

	int id = itemIDs[i];

	states[id] &= ~STATE_ALIVE;

	itemIDs[i] = itemIDs[--itemCount];
}

void killEnemy(int i)
{
	if (i < 0 || i >= enemyCount) return;

	int id = enemyIDs[i];

	states[id] &= ~STATE_ALIVE;

	enemyIDs[i] = enemyIDs[--enemyCount];
}

void killBullet(int i)
{
	if (i < 0 || i >= bulletCount) return;

	int id = bulletIDs[i];

	states[id] &= ~STATE_ALIVE;

	bulletIDs[i] = bulletIDs[--bulletCount];
}

int main()
{
	if (!font.loadFromFile("font.ttf"))
	{
		std::cout << "Error loading font!" << std::endl;
		return -1;
	}

	// SPAWN ITEMS
	for (int i = 0; i < ITEM_MAX; i++)
	{
		if (itemCount >= ITEM_MAX) break;

		sf::Vector2f spawnPos = {	static_cast<float>(rand() % WORLD_WIDTH),
									static_cast<float>(rand() % WORLD_HEIGHT)};

		spawnItem(spawnPos);
	}

	// SPAWN ENEMIES
	for (int i = 0; i < ENEMY_MAX; i++)
	{
		if (enemyCount < ENEMY_MAX)
		{
			sf::Vector2f spawnPos = { static_cast<float>(rand() % WORLD_WIDTH),
									static_cast<float>(rand() % WORLD_HEIGHT) };

			spawnEnemy(spawnPos);
		}

		
	}

	// INITIALIZE BULLETS
	for (int i = 0; i < BULLET_MAX; i++)
	{
		if (bulletCount >= BULLET_MAX) break;

		initBullet();
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
	
	// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
	// WINDOW LOOP //
	// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
	while (window.isOpen())
	{
		if (playerState & 0b1) window.clear();
		
		// GET CURRENT VARIABLES //
		float deltaTime = deltaClock.restart().asSeconds();

		mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

		// Calculate aim
		sf::Vector2f deltaAim = mousePos - player.getPosition();
		float distSqAim = deltaAim.x * deltaAim.x + deltaAim.y * deltaAim.y;
		float distAim = std::sqrt(distSqAim);
		if (distAim == 0.0f) distAim = 0.0001f;
		sf::Vector2f normalAim = deltaAim / distAim;
		aim = normalAim;

		// Get view bounds
		sf::FloatRect viewBounds;
		sf::View currentView = window.getView();
		viewBounds.left = currentView.getCenter().x - currentView.getSize().x / 2.f;
		viewBounds.top = currentView.getCenter().y - currentView.getSize().y / 2.f;
		viewBounds.width = currentView.getSize().x;
		viewBounds.height = currentView.getSize().y;

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// PLAYER INPUT & EVENTS //
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// INPUTS
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed) window.close();

			if (event.type == sf::Event::KeyPressed) if (event.key.code == sf::Keyboard::Escape) gameState ^= GAME_PAUSE;

			if (event.type == sf::Event::KeyPressed) if (event.key.code == sf::Keyboard::End) playerState ^= 0b1;

			if (gameState & GAME_PAUSE)
			{
				if (event.type == sf::Event::MouseButtonPressed) if (event.mouseButton.button == sf::Mouse::Right) gameState ^= GAME_STATS;
			}
			else gameState &= ~GAME_STATS;

			if (!(playerState & STATE_ALIVE)) break;

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

			//if (event.type == sf::Event::MouseMoved) playerInput |= INPUT_DRAG;
		}

		// HANDLE PLAYER SHOOT INPUT
		// Get next available bullet, set its state, position and velocity.
		if (playerInput & INPUT_SHOOT)
		{
			for (int i = 0; i < bulletCount; i++)
			{
				int id = bulletIDs[i];

				if (!(states[id] & STATE_ALIVE))
				{
					states[id] |= STATE_ALIVE;
					positions[id] = player.getPosition() + aim * BULLET_MUZZLE;
					velocities[id] = aim * BULLET_SPEED;

					health--;
					regenTimer.restart();
					break;
				}
			}
		}

		// HANDLE GRAB INPUT
		// Sets grabbed to hold the index.
		if (playerInput & INPUT_GRAB)
		{
			if (grabbedID == -1)
			{
				
				for (int i = 0; i < itemCount; i++)
				{
					int id = itemIDs[i];

					if (states[id] & STATE_ALIVE)
					{
						sf::Vector2f delta = mousePos - positions[id];
						float distSq = delta.x * delta.x + delta.y * delta.y;

						if (distSq < GRAB_DISTANCE)
						{
							grabbedID = id;
							states[id] |= STATE_GRABBED;
							break;
						}
					}
				}

			}
		}

		if (playerInput & INPUT_DROP)
		{
			velocities[grabbedID] *= DROP_FORCE;
			states[grabbedID] &= ~STATE_GRABBED;
			grabbedID = -1;
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

		// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
		// AI //
		// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
		for (int i = 0; i < enemyCount; i++)
		{
			int id = enemyIDs[i];
			
			if (!(states[id] & STATE_ALIVE)) continue;

			int neighborCount = 0;

			sf::Vector2f separation(0.f, 0.f);
			sf::Vector2f alignment(0.f, 0.f);
			sf::Vector2f cohesion(0.f, 0.f);
			
			sf::Vector2f vel = velocities[id];

			for (int j = 0; j < enemyCount; j++)
			{
				int jd = enemyIDs[j];

				if (!(states[jd] & STATE_ALIVE)) continue;

				sf::Vector2f delta = positions[jd] - positions[id];
				float distSq = delta.x * delta.x + delta.y * delta.y;

				if (distSq < FLOCK_RADIUS * FLOCK_RADIUS)
				{
					float dist = std::sqrt(distSq);
					separation -= delta / (dist + 1.f);
					alignment += velocities[jd];
					cohesion += positions[jd];
					neighborCount++;

					sf::Vector2f midPoint = (positions[id] + positions[jd]) * 0.5f;

					sf::Vertex flockLine[] = {
						sf::Vertex(positions[id], sf::Color::Red),
						sf::Vertex(midPoint, sf::Color::Black),
						sf::Vertex(positions[jd], sf::Color::Red)
					};

					window.draw(flockLine, 3, sf::LineStrip);

					if (distSq < FLOCK_HUDDLE * FLOCK_HUDDLE)
					{
						sf::Vector2f midPoint = (positions[id] + positions[jd]) * 0.5f;

						sf::Vertex flockLine[] = { sf::Vertex(positions[id], sf::Color::Red), sf::Vertex(positions[jd], sf::Color::Red) };

						window.draw(flockLine, 2, sf::LineStrip);
					}
				}

				//
				//sf::Vertex moveLine[] = { sf::Vertex(positions[id], sf::Color::Red), sf::Vertex(positions[jd], sf::Color::Black)};
				//window.draw(moveLine, 2, sf::Lines);
			}

			if (neighborCount > 0)
			{
				alignment /= static_cast<float>(neighborCount);
				cohesion /= static_cast<float>(neighborCount);
				cohesion -= positions[id];

				sf::Vector2f steer = separation * FLOCK_SEPARATION + alignment * FLOCK_ALIGNMENT + cohesion * FLOCK_COHESION;
				vel += steer * deltaTime;

				float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
				if (speed > FLOCK_SPEED) vel = (vel / speed) * FLOCK_SPEED;
				velocities[i] = vel;
			}
		}
		/*
		// ENEMY COLLISION & FLOCKING
		for (size_t i = 0; i < ENEMY_MAX; i++)
		{
			if (!(enemies.state[i] & STATE_ALIVE)) continue;

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
		*/

		// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		// UPDATE PHYSICS //
		// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		if ((gameState & GAME_PAUSE) == 0)
		{
			// ITEM PHYSICS
			for (int i = 0; i < itemCount; i++)
			{
				int id = itemIDs[i];

				if (!(states[id] & STATE_ALIVE)) continue;

				if (states[id] & STATE_GRABBED)
				{
					sf::Vector2f delta = mousePos - positions[id];
					velocities[id] = delta * GRAB_SPRING;
				}

				velocities[id] *= ITEM_FRICTION;
			}

			// ENEMY PHYSICS
			for (int i = 0; i < enemyCount; i++)
			{
				int id = enemyIDs[i];

				if (!(states[id] & STATE_ALIVE)) continue;

				velocities[id] *= ENEMY_FRICTION;
			}

			// BULLET PHYSICS
			for (int i = 0; i < bulletCount; i++)
			{
				int id = bulletIDs[i];

				if (!(states[id] & STATE_ALIVE)) continue;

				float angle = std::atan2(velocities[id].y, velocities[id].x) * 180.f / 3.14159265f;
				rotations[id] = angle + 90;
			}

			// PLAYER PHYSICS
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

			// ALL ENTITIES
			for (int i = 0; i < entityCount; i++)
			{
				if (!(states[i] & STATE_ALIVE)) continue;
					
				positions[i] += velocities[i] * deltaTime;
			}
		}

		// ############################################################################################################################
		// COLLISIONS AND INTERACTIONS //
		// ############################################################################################################################
		if ((gameState & GAME_PAUSE) == 0 && playerState & STATE_ALIVE)
		{
			/*
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
			*/

			// ITEM-ENEMY COLLISION
			for (int i = 0; i < itemCount; i++)
			{
				int id = itemIDs[i];

				if (!(states[id] & STATE_ALIVE)) continue;

				for (int j = 0; j < enemyCount; j++)
				{
					int jd = itemIDs[j];

					if (!(states[jd] & STATE_ALIVE)) continue;

					sf::Vector2f delta = positions[id] - positions[jd];
					float distSq = delta.x * delta.x + delta.y * delta.y;

					if (distSq < COLLISION_ITEM_ENEMY)
					{
						velocities[jd] += velocities[id] * BULLET_FORCE * deltaTime;

						if (playerState & STATE_ALIVE)
						{
							//enemies.state[j] &= ~0b1;
							//enemyCount--;
						}

						//states[id] &= ~STATE_ALIVE;
						break;
					}
				}
			}

			// BULLET-WINDOW COLLISION
			for (int i = 0; i < bulletCount; i++)
			{
				int id = bulletIDs[i];

				if (!(states[id] & STATE_ALIVE)) continue;

				// BULLET-WINDOW COLLISION
				if (!viewBounds.contains(positions[id]))
				{
					killBullet(i);
					break;
				}
			}

			// BULLET-ITEM COLLISION
			for (int i = 0; i < bulletCount; i++)
			{
				int id = bulletIDs[i];

				if (!(states[id] & STATE_ALIVE)) continue;

				for (int j = 0; j < itemCount; j++)
				{
					int jd = itemIDs[j];

					if (!(states[jd] & STATE_ALIVE)) continue;

					sf::Vector2f delta = positions[id] - positions[jd];
					float distSq = delta.x * delta.x + delta.y * delta.y;

					if (distSq < COLLISION_BULLET_ITEM)
					{
						velocities[jd] += velocities[id] * BULLET_FORCE * deltaTime;
						killBullet(i);
						break;
					}
				}
			}
						
			// BULLET-ENEMY COLLISION
			for (int i = 0; i < bulletCount; i++)
			{
				int id = bulletIDs[i];

				if (!(states[id] & STATE_ALIVE)) continue;

				// BULLET-ENEMY COLLISION
				for (int j = 0; j < enemyCount; j++)
				{
					int jd = enemyIDs[j];

					if (!(states[jd] & STATE_ALIVE)) continue;

					sf::Vector2f delta = positions[id] - positions[jd];
					float distSq = delta.x * delta.x + delta.y * delta.y;

					if (distSq < COLLISION_BULLET_ENEMY)
					{
						velocities[jd] += velocities[id] * BULLET_FORCE * deltaTime;

						if (playerState & STATE_ALIVE)
						{
							//enemies.state[j] &= ~0b1;
							//enemyCount--;
						}

						killBullet(i);
						break;
					}
				}


			}
			
			// ENEMY-PLAYER COLLISION
			for (int i = 0; i < enemyCount; i++)
			{
				int id = enemyIDs[i];

				if (id < 0 || id >= ENEMY_MAX) continue;
				if (!(states[id] & STATE_ALIVE)) continue;

				sf::Vector2f delta = position - positions[id];
				float distSq = delta.x * delta.x + delta.y * delta.y;

				if (distSq < PLAYER_COLLISION)
				{
					health--;
					regenTimer.restart();
				}
			}
		}

		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// UPDATE STATES //
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// UPDATE HEALTH
		if (health <= 0) playerState &= ~STATE_ALIVE;

		if (health < PLAYER_HEALTH && regenTimer.getElapsedTime().asSeconds() > REGEN_TIME) health += REGEN_SPEED * deltaTime;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// RENDERING & VFX //
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// RENDER ITEMS
		for (int i = 0; i < itemCount; i++)
		{
			int id = itemIDs[i];

			if (states[id] & STATE_ALIVE)
			{
				item.setPosition(positions[id]);
				window.draw(item);
			}
		}

		// RENDER ENEMIES
		for (int i = 0; i < enemyCount; i++)
		{
			int id = enemyIDs[i];

			if (id < 0 || id >= ENEMY_MAX) continue;
			if (states[id] & STATE_ALIVE)
			{
				enemy.setPosition(positions[id]);
				window.draw(enemy);
			}
		}

		// RENDER BULLETS
		for (int i = 0; i < bulletCount; i++)
		{
			int id = bulletIDs[i];

			if (states[id] & STATE_ALIVE)
			{
				bullet.setRotation(rotations[id]);
				bullet.setPosition(positions[id]);
				window.draw(bullet);
			}
		}

		/*
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
		*/

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

			/*
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
			*/
		}
		else
		{
			player.setOutlineColor(sf::Color::Transparent);

			//if (grabbed > -1)
			//{
			//	item.setOutlineColor(sf::Color::Transparent);
			//}
		}

		/*
		for (size_t i = 0; i < BULLET_MAX; i++)
		{
			if (bullets.state[i] & STATE_ALIVE)
			{
				bullet.setRotation(bullets.rotation[i]);
				bullet.setPosition(bullets.position[i]);
				window.draw(bullet);
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
		*/
		if (playerState & STATE_ALIVE)
		{
			float angle = std::atan2(aim.y, aim.x) * 180.f / 3.14159265f;
			player.setRotation(angle + 90);
			
			player.setFillColor(sf::Color::Blue);
			player.setPosition(position);
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

		/*
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
		*/
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
			//if (enemyCount > 1 && enemyCount < ENEMY_MAX) countString = std::to_string(enemyCount) + " enemies left";
			//else if(enemyCount == 1 && enemyCount != ENEMY_MAX) countString = "only 1 enemy left";!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

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
		
		// GAME STATS SCREEN **********************************************************************************************************
		if (gameState & GAME_STATS)
		{
			window.clear(sf::Color::White);

			sf::Text pauseText("The game is paused. Press [Esc] to continue.", font, 24);
			//sf::Text enemyCountText("enemy count: ", font, 24);
			//sf::Text itemCountText("item count: ", font, 24);

			pauseText.setFillColor(sf::Color::Black);
			pauseText.setPosition(WINDOW_WIDTH * 0.5, WINDOW_HEIGHT * 0.2);
			pauseText.setOrigin(pauseText.getLocalBounds().width * 0.5, pauseText.getLocalBounds().height * 0.5);
			
			//enemyCountText.setFillColor(sf::Color::Black);
			//enemyCountText.setPosition(WINDOW_WIDTH * 0.3, WINDOW_HEIGHT * 0.45);
			//enemyCountText.setOrigin(enemyCountText.getLocalBounds().width * 0.5, enemyCountText.getLocalBounds().height * 0.5);

			//itemCountText.setFillColor(sf::Color::Black);
			//itemCountText.setPosition(WINDOW_WIDTH * 0.3, WINDOW_HEIGHT * 0.55);
			//itemCountText.setOrigin(itemCountText.getLocalBounds().width * 0.5, itemCountText.getLocalBounds().height * 0.5);
			
			window.draw(pauseText);
			//window.draw(enemyCountText);
			//window.draw(itemCountText);
		}

		// ****************************************************************************************************************************
		window.setView(view);
        window.display();
    }

    return 0;
};