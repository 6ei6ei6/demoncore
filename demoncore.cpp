// demoncore.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <cstdint>
#include <iostream>
#include <fstream>
#include <filesystem> 
#include <chrono>
#include <ctime>
#include <cmath>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Font.hpp>

namespace GameConfig
{
	constexpr int WINDOW_WIDTH = 1000;
	constexpr int WINDOW_HEIGHT = 1000;
	constexpr int WORLD_WIDTH = 10000;
	constexpr int WORLD_HEIGHT = 10000;
	constexpr int FPS_MAX = 60;
	constexpr float HUD_SIZE = 40;
	constexpr int HEALTH_MAX = 200;
	constexpr int HEALTH_COST = 100;
	constexpr int ENTITY_RADIUS = 10;
	constexpr int ENTITY_OUTLINE = 2;
	constexpr float DROP_FORCE = 3;
	constexpr float REGEN_TIME = 1;
	constexpr float REGEN_SPEED = 100;
	constexpr float DEADZONE_WIDTH = 100;
	constexpr float DEADZONE_HEIGHT = 100;

	constexpr size_t ITEM_MAX = 10;
	constexpr size_t ENEMY_MAX = 10;//2500;
	constexpr size_t ENTITY_MAX = ITEM_MAX + ENEMY_MAX;
}

constexpr float GRAB_DISTANCE = 100; // Handled another way in the future.

namespace FlockAI
{
	constexpr const float FLOCK_RADIUS = 200;
	constexpr const float FLOCK_SPEED = 20;
	constexpr const float FLOCK_SEPARATION = 100.0;
	constexpr const float FLOCK_ALIGNMENT = 10.0;
	constexpr const float FLOCK_COHESION = 5.0;
}

namespace GamePhysics
{
	constexpr float PLAYER_SPEED = 200;
	constexpr float PLAYER_TURNING = 15;
	constexpr float PLAYER_FRICTION = 0.2;
	constexpr float GRAB_SPRING = 5;
	constexpr float ITEM_FRICTION = 0.9f;
	constexpr float ENEMY_FRICTION = 0.8;
}

namespace GameCollision
{
	constexpr float PLAYER_COLLISION = 100;
	constexpr float BULLET_FORCE = 0.8;
	constexpr float COLLISION_ITEM_ENEMY = 100;
	constexpr float COLLISION_ITEM_PLAYER = 50;
	constexpr float COLLISION_BULLET_ITEM = 100;
	constexpr float COLLISION_BULLET_ENEMY = 50;
}

namespace GameVFX
{
	constexpr const float FLOCK_LINE = FlockAI::FLOCK_RADIUS;
	constexpr const float FLOCK_HUDDLE = 50;

	constexpr float DEATH_SPEED = 100;
	constexpr float DEATH_RADIUS = 30;
	constexpr float DEATH_SPREAD = 1;
	constexpr float DEATH_FORCE = 5;
}

// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

namespace GameFlags
{
	constexpr uint8_t GAME_PAUSE = 1 << 0;
	constexpr uint8_t GAME_STATS = 1 << 1; // Necessary?
	constexpr uint8_t GAME_OVER = 1 << 2; // Deprecated
	constexpr uint8_t GAME_HUD = 1 << 3; // Handle somewhere else in future
}

namespace EngineFlags
{
	constexpr uint8_t ENGINE_DEBUG = 1 << 0;
	constexpr uint8_t ENGINE_INPUT = 1 << 1;
	constexpr uint8_t ENGINE_AI = 1 << 2;
	constexpr uint8_t ENGINE_PHYSICS = 1 << 3;
	constexpr uint8_t ENGINE_COLLISION = 1 << 4;
	constexpr uint8_t ENGINE_RENDERING = 1 << 5;
	constexpr uint8_t ENGINE_VFX = 1 << 6;
}

namespace InputFlags
{
	constexpr uint16_t INPUT_MOVERIGHT = 1 << 0;
	constexpr uint16_t INPUT_MOVEDOWN = 1 << 1;
	constexpr uint16_t INPUT_MOVELEFT = 1 << 2;
	constexpr uint16_t INPUT_MOVEUP = 1 << 3;
	constexpr uint16_t INPUT_SHOOT = 1 << 4;
	constexpr uint16_t INPUT_GRAB = 1 << 5;
	constexpr uint16_t INPUT_DROP = 1 << 6;
	constexpr uint16_t INPUT_ZOOMIN = 1 << 7;
	constexpr uint16_t INPUT_ZOOMOUT = 1 << 8;
}

namespace EntityFlags
{
	constexpr uint8_t STATE_ALIVE = 1 << 0;
	constexpr uint8_t STATE_MOVING = 1 << 1;
	constexpr uint8_t STATE_GRABBED = 1 << 2;
	constexpr uint8_t STATE_HURT = 1 << 3;
	// ALIVE - Rendering
	// SLEEP - No physics
}

struct GameState
{
	uint8_t gameFlags = 0b1000;
	uint8_t engineFlags = 0b1111111;

	uint16_t playerInput = 0;

	int health = GameConfig::HEALTH_MAX;
	int grabbedID = -1;
	float zoom = 1.f;

	sf::Vector2f aim = { 0,0 };

	sf::Vector2f mousePos = { 0,0 };

	sf::FloatRect viewBounds;
	sf::FloatRect deadZone;
	sf::FloatRect targetBounds;


	// ======================

	int entityCount = 0;

	int availableIDs[GameConfig::ENTITY_MAX];





	sf::Vector2f positions[GameConfig::ENTITY_MAX];
	sf::Vector2f velocities[GameConfig::ENTITY_MAX];

	float rotations[GameConfig::ENTITY_MAX];

	uint8_t states[GameConfig::ENTITY_MAX];

	int itemIDs[GameConfig::ITEM_MAX];
	int enemyIDs[GameConfig::ENEMY_MAX];

	int itemCount = 0;
	int enemyCount = 0;
} game;

// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$





void printVec(const std::vector<int>& vec, const std::string& header)
{
	std::cout << "\n" << header << " : " << vec.size() << "\n";
	for (int i = 0; i < vec.size(); i++)
	{
		std::cout << vec[i] << " ";
	}
	std::cout << "\n";
}





namespace PoolFlags
{
	constexpr uint8_t Spawned = 1 << 0;
	constexpr uint8_t Possessed = 1 << 1;
 	constexpr uint8_t Awake = 1 << 2;
 	constexpr uint8_t Solid = 1 << 3;
	constexpr uint8_t Visible = 1 << 4;
	//constexpr uint8_t Visible = 1 << 5;
	// 	constexpr uint8_t X = 1 << 6;
	// 	constexpr uint8_t X = 1 << 7;
}

struct EntityPool
{
public:
	int poolSize;

	std::vector<int> availableIDs;
	std::vector<int> activeIDs;

	std::vector<uint8_t> flags;

	std::vector<sf::Vector2f> position;
	std::vector<sf::Vector2f> velocity;

	EntityPool(int size) : poolSize(size)
	{
		availableIDs.reserve(size);
		activeIDs.reserve(size);

		flags.resize(size, 0);

		position.resize(size, {0,0});
		velocity.resize(size, {0,0});

		for (int i = 0; i < poolSize; i++)
		{
			availableIDs.push_back(i);
		}
	}
};

EntityPool entities(5);

int acquireEntity()
{
	if (entities.availableIDs.empty()) return -1;

	int id = entities.availableIDs.back();
	entities.availableIDs.pop_back();

	entities.activeIDs.push_back(id);

	return id;
}

void releaseEntity(int id)
{
	auto it = std::find(entities.activeIDs.begin(), entities.activeIDs.end(), id);
	if (it != entities.activeIDs.end())
	{
		entities.activeIDs.erase(it);
		entities.availableIDs.push_back(id);
	}
}

void sspawnEnemy()
{
	int id = acquireEntity();
	if (id == -1) return;
	
	entities.flags[id] |= PoolFlags::Spawned;
	entities.flags[id] |= PoolFlags::Possessed;
	entities.flags[id] |= PoolFlags::Awake;
	entities.flags[id] |= PoolFlags::Solid;
	entities.flags[id] |= PoolFlags::Visible;

	sf::Vector2f spawnPos = { static_cast<float>(rand() % GameConfig::WORLD_WIDTH),
									static_cast<float>(rand() % GameConfig::WORLD_HEIGHT) };

	entities.position[id] = spawnPos;
	entities.velocity[id] = { 0,0 };
}

void sspawnItem()
{
	int id = acquireEntity();
	if (id == -1) return;

	entities.flags[id] |= PoolFlags::Spawned;
	entities.flags[id] |= PoolFlags::Solid;
	entities.flags[id] |= PoolFlags::Visible;

	sf::Vector2f spawnPos = { static_cast<float>(rand() % GameConfig::WORLD_WIDTH),
									static_cast<float>(rand() % GameConfig::WORLD_HEIGHT) };

	entities.position[id] = spawnPos;
	entities.velocity[id] = { 0,0 };
}


int playerID = -1;

void sspawnPlayer()
{
	int id = acquireEntity();
	if (id == -1) return;
	playerID = id;

	entities.flags[id] |= PoolFlags::Spawned;
	entities.flags[id] |= PoolFlags::Awake;
	entities.flags[id] |= PoolFlags::Solid;
	entities.flags[id] |= PoolFlags::Visible;

	entities.position[id] = { 0,0 };
	entities.velocity[id] = { 0,0 };
}

void sdestroyEntity()
{
	int id = entities.activeIDs.back();
	//entities.flags[id] 0;
	releaseEntity(id);
}






/*
sf::CircleShape entityShape(60, 50);

namespace RenderFlags
{
	constexpr uint8_t Enemy = 1 << 0;
	constexpr uint8_t Item = 1 << 1;
	constexpr uint8_t Player = 1 << 2;
	constexpr uint8_t Pause = 1 << 3;
	//constexpr uint8_t Visible = 1 << 4;
	//constexpr uint8_t X = 1 << 5;
	//constexpr uint8_t X = 1 << 6;
}

void setShape(sf::CircleShape& shape, uint8_t renderFlag)
{
	//shape.setRadius(GameConfig::SHAPE_RADIUS);
	//shape.setOrigin({ GameConfig::SHAPE_RADIUS, GameConfig::SHAPE_RADIUS });
	//shape.setOutlineColor(GameConfig::SHAPE_COLOR);
	//shape.setOutlineThickness(GameConfig::SHAPE_OUTLINE);

	if (renderFlag & RenderFlags::Enemy)
	{
		shape.setFillColor(sf::Color::Red);
		shape.setPointCount(3);
	}

	if (renderFlag & RenderFlags::Item)
	{
		shape.setFillColor(sf::Color::Green);
		shape.setPointCount(4);
	}

	if (renderFlag & RenderFlags::Player)
	{
		shape.setFillColor(sf::Color::Blue);
		shape.setPointCount(3);

	}

	if (renderFlag & RenderFlags::Pause)
	{
		//shape.setScale()??
		//shape.setRadius(100);
		//shape.setRadius(GameConfig::PAUSE_RADIUS);
	}
}

*/





















bool screensave = false;
int screenshotCount = 0;

float deltaTime;

sf::Clock deltaClock;
sf::Clock regenTimer;

sf::Font font;

sf::Event event;

std::string countString;
sf::Text countText(countString, font, 24);

sf::Text HUD("HUD", font, GameConfig::HUD_SIZE);
sf::Text overText("GAME OVER", font, 50);
sf::Text pauseText("The game is paused. Press [Esc] to continue.", font, 24);

sf::Text triangleText("Enemy", font, 24);
sf::Text squareText("Item", font, 24);
sf::Text circleText("Player", font, 24);

sf::Color hurtingColor = sf::Color::Blue;

sf::CircleShape player(GameConfig::ENTITY_RADIUS, 3);
sf::CircleShape item(GameConfig::ENTITY_RADIUS, 4);
sf::CircleShape enemy(GameConfig::ENTITY_RADIUS, 3);
sf::CircleShape bullet(GameConfig::ENTITY_RADIUS, 3);
sf::CircleShape vfx(GameConfig::ENTITY_RADIUS, 3);

sf::CircleShape triangle(100.f, 3);
sf::CircleShape square(100.f, 4);
sf::CircleShape triangle2(100.f, 3);

sf::RenderWindow window(sf::VideoMode({ GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT }), "demoncore");
sf::View view({ 0.f,200.f }, { GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT });
const sf::View pauseView = window.getDefaultView();
const sf::View hudView = window.getDefaultView();


void initShapes()
{
	player.setOrigin({ GameConfig::ENTITY_RADIUS, GameConfig::ENTITY_RADIUS });
	//player.setPosition(game.position);
	player.setFillColor(sf::Color::Blue);
	player.setOutlineColor(sf::Color::Transparent);
	player.setOutlineThickness(GameConfig::ENTITY_OUTLINE);

	bullet.setOrigin({ GameConfig::ENTITY_RADIUS, GameConfig::ENTITY_RADIUS });
	bullet.setFillColor(sf::Color::Red);
	bullet.setOutlineColor(sf::Color::Transparent);
	bullet.setOutlineThickness(GameConfig::ENTITY_OUTLINE);

	enemy.setOrigin({ GameConfig::ENTITY_RADIUS, GameConfig::ENTITY_RADIUS });
	enemy.setFillColor(sf::Color::Red);
	enemy.setOutlineColor(sf::Color::Transparent);
	enemy.setOutlineThickness(GameConfig::ENTITY_OUTLINE);

	item.setOrigin({ GameConfig::ENTITY_RADIUS, GameConfig::ENTITY_RADIUS });
	item.setFillColor(sf::Color::Green);
	item.setOutlineColor(sf::Color::Transparent);
	item.setOutlineThickness(GameConfig::ENTITY_OUTLINE);

	vfx.setOrigin({ GameConfig::ENTITY_RADIUS, GameConfig::ENTITY_RADIUS });
	vfx.setFillColor(sf::Color::Transparent);
	vfx.setOutlineColor(sf::Color::Transparent);
	vfx.setOutlineThickness(GameConfig::ENTITY_OUTLINE);

	triangle.setOrigin({ 100, 100 });
	triangle.setFillColor(sf::Color::Red);
	triangle.setPosition(GameConfig::WINDOW_WIDTH * 0.25, GameConfig::WINDOW_HEIGHT * 0.5);

	square.setOrigin({ 100, 100 });
	square.setFillColor(sf::Color::Green);
	square.setPosition(GameConfig::WINDOW_WIDTH * 0.5, GameConfig::WINDOW_HEIGHT * 0.5);

	triangle2.setOrigin({ 100, 100 });
	triangle2.setFillColor(sf::Color::Blue);
	triangle2.setPosition(GameConfig::WINDOW_WIDTH * 0.75, GameConfig::WINDOW_HEIGHT * 0.5);
}

void initTexts()
{
	if (!font.loadFromFile("font.ttf"))
	{
		std::cout << "Error loading font!\n";
		return;
	}

	HUD.setOutlineColor(sf::Color::Black);
	HUD.setOutlineThickness(2);
	HUD.setLetterSpacing(5);

	pauseText.setFillColor(sf::Color::Black);
	pauseText.setPosition(GameConfig::WINDOW_WIDTH * 0.5, GameConfig::WINDOW_HEIGHT * 0.2);
	pauseText.setOrigin(pauseText.getLocalBounds().width * 0.5, pauseText.getLocalBounds().height * 0.5);

	triangleText.setFillColor(sf::Color::White);
	triangleText.setPosition(GameConfig::WINDOW_WIDTH * 0.25, GameConfig::WINDOW_HEIGHT * 0.7);
	triangleText.setOrigin(triangleText.getLocalBounds().width * 0.5, triangleText.getLocalBounds().height * 0.5);

	squareText.setFillColor(sf::Color::White);
	squareText.setPosition(GameConfig::WINDOW_WIDTH * 0.5, GameConfig::WINDOW_HEIGHT * 0.7);
	squareText.setOrigin(squareText.getLocalBounds().width * 0.5, squareText.getLocalBounds().height * 0.5);

	circleText.setFillColor(sf::Color::White);
	circleText.setPosition(GameConfig::WINDOW_WIDTH * 0.75, GameConfig::WINDOW_HEIGHT * 0.7);
	circleText.setOrigin(circleText.getLocalBounds().width * 0.5, circleText.getLocalBounds().height * 0.5);

	overText.setLetterSpacing(5);
	overText.setFillColor(sf::Color::Black);
	overText.setOutlineThickness(0);
	overText.setOutlineColor(sf::Color::Black);
	overText.setPosition(GameConfig::WINDOW_WIDTH * 0.5, GameConfig::WINDOW_HEIGHT * 0.5);
	overText.setOrigin(overText.getLocalBounds().width * 0.5, overText.getLocalBounds().height * 0.5);

	countText.setFillColor(sf::Color::Red);
}

void saveGame()
{
	std::ofstream out("save.txt", std::ios::binary);
	if (out)
	{
		if (game.engineFlags & EngineFlags::ENGINE_DEBUG) std::cout << "Saving game...\n";
	}
	else
	{
		if (game.engineFlags & EngineFlags::ENGINE_DEBUG) std::cout << "Error saving game.\n";
		return;
	}
	
	out.write(reinterpret_cast<const char*>(&game), sizeof(GameState));

	if (game.engineFlags & EngineFlags::ENGINE_DEBUG) std::cout << "Game saved!\n";
}

void loadGame()
{
	std::ifstream in("save.txt", std::ios::binary);
	
	if (in)
	{
		if(game.engineFlags & EngineFlags::ENGINE_DEBUG) std::cout << "Loading latest save...\n";
	}
	else
	{
		if (game.engineFlags & EngineFlags::ENGINE_DEBUG) std::cout << "Error loading game.\n";
		return;
	}

	in.read(reinterpret_cast<char*>(&game), sizeof(GameState));
	if (game.engineFlags & EngineFlags::ENGINE_DEBUG) std::cout << "Game loaded!\n";
}

void takeScreenshot(sf::RenderWindow& window)
{
	sf::Texture texture;
	texture.create(window.getSize().x, window.getSize().y);
	texture.update(window);
	sf::Image screenshot = texture.copyToImage();

	std::string filename = "screenshot" + std::to_string(screenshotCount) + ".png";
	if (screenshot.saveToFile(filename) && game.engineFlags & EngineFlags::ENGINE_DEBUG) std::cout << "Screenshot saved to " << filename << "\n";

	screenshotCount++;
	screensave = false;
}

int main()
{
	sspawnPlayer();







	sf::Vector2f mouseStart = game.mousePos;
	
	std::vector<int> values = { 100, 75, 42, 9001 };

	// SPAWN ITEMS
	for (int i = 0; i < GameConfig::ITEM_MAX; i++)
	{
		if (game.itemCount >= GameConfig::ITEM_MAX) break;

		sf::Vector2f spawnPos = {	static_cast<float>(rand() % GameConfig::WORLD_WIDTH),
									static_cast<float>(rand() % GameConfig::WORLD_HEIGHT)};

		//spawnItem(spawnPos); <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3
	}

	// SPAWN ENEMIES
	for (int i = 0; i < GameConfig::ENEMY_MAX; i++)
	{
		if (game.enemyCount < GameConfig::ENEMY_MAX)
		{
			sf::Vector2f spawnPos = { static_cast<float>(rand() % GameConfig::WORLD_WIDTH),
									static_cast<float>(rand() % GameConfig::WORLD_HEIGHT) };

			//spawnEnemy(spawnPos); <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME
		}
	}

	game.deadZone.width = GameConfig::DEADZONE_WIDTH;
	game.deadZone.height = GameConfig::DEADZONE_HEIGHT;

	initShapes();
	initTexts();
	
	view.setCenter(player.getPosition());
	window.setFramerateLimit(GameConfig::FPS_MAX);

	// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
	// MAIN LOOP //
	// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
	while (window.isOpen())
	{
		// SET UP FRAME //
		if (entities.flags[playerID] & PoolFlags::Spawned)
		{
			if (!(game.engineFlags & EngineFlags::ENGINE_INPUT)) game.engineFlags |= EngineFlags::ENGINE_INPUT;
			if (!(game.engineFlags & EngineFlags::ENGINE_AI)) game.engineFlags |= EngineFlags::ENGINE_AI;
			if (!(game.engineFlags & EngineFlags::ENGINE_COLLISION)) game.engineFlags |= EngineFlags::ENGINE_COLLISION;


			window.clear();
			deltaTime = deltaClock.restart().asSeconds();

			game.mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

			// Calculate aim
			sf::Vector2f deltaAim = game.mousePos - player.getPosition();
			float distSqAim = deltaAim.x * deltaAim.x + deltaAim.y * deltaAim.y;
			float distAim = std::sqrt(distSqAim);
			if (distAim == 0.0f) distAim = 0.0001f;
			sf::Vector2f normalAim = deltaAim / distAim;
			game.aim = normalAim;

			// Get view bounds
			sf::View currentView = window.getView();
			game.viewBounds.left = currentView.getCenter().x - currentView.getSize().x / 2.f;
			game.viewBounds.top = currentView.getCenter().y - currentView.getSize().y / 2.f;
			game.viewBounds.width = currentView.getSize().x;
			game.viewBounds.height = currentView.getSize().y;
		}
		else if (!(entities.flags[playerID] & PoolFlags::Spawned))
		{
			game.engineFlags &= ~EngineFlags::ENGINE_INPUT;
			game.engineFlags &= ~EngineFlags::ENGINE_AI;
			game.engineFlags &= ~EngineFlags::ENGINE_COLLISION;
		}





















		for (int i = 0; i < entities.activeIDs.size(); i++)
		{
			int id = entities.activeIDs[i];
			if (id == playerID) continue;

			sf::CircleShape shape(40, 40);
			shape.setPosition(entities.position[id]);
			shape.setFillColor(sf::Color::White);
			shape.setOrigin(30, 40);
			window.draw(shape);
		}







		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// PLAYER INPUT & EVENTS //
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// INPUTS
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed) window.close();

			if (event.type == sf::Event::KeyPressed) if (event.key.code == sf::Keyboard::Escape) game.gameFlags ^= GameFlags::GAME_PAUSE;

			if (event.type == sf::Event::KeyPressed) if (event.key.code == sf::Keyboard::End) entities.flags[playerID] ^= PoolFlags::Spawned;


			if (game.gameFlags & GameFlags::GAME_PAUSE)
			{
				if (event.type == sf::Event::MouseButtonPressed) if (event.mouseButton.button == sf::Mouse::Right) game.gameFlags ^= GameFlags::GAME_STATS;
			}
			else game.gameFlags &= ~GameFlags::GAME_STATS;

			if (game.engineFlags & EngineFlags::ENGINE_DEBUG)
			{
				if (event.type == sf::Event::KeyPressed)
				{
					if (event.key.code == sf::Keyboard::P) screensave = true;
					if (event.key.code == sf::Keyboard::Tab) saveGame();
					if (event.key.code == sf::Keyboard::BackSpace) loadGame();

					if (event.key.code == sf::Keyboard::Add) sspawnEnemy();
					if (event.key.code == sf::Keyboard::Subtract) sdestroyEntity();
					if (event.key.code == sf::Keyboard::Enter) ;

				}
			}

			if (!(entities.flags[playerID] & PoolFlags::Spawned)) break;

			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::D) game.playerInput |= InputFlags::INPUT_MOVERIGHT;

				if (event.key.code == sf::Keyboard::S) game.playerInput |= InputFlags::INPUT_MOVEDOWN;

				if (event.key.code == sf::Keyboard::A) game.playerInput |= InputFlags::INPUT_MOVELEFT;

				if (event.key.code == sf::Keyboard::W) game.playerInput |= InputFlags::INPUT_MOVEUP;
			}

			if (event.type == sf::Event::KeyReleased)
			{
				if (event.key.code == sf::Keyboard::D) game.playerInput &= ~InputFlags::INPUT_MOVERIGHT;

				if (event.key.code == sf::Keyboard::S) game.playerInput &= ~InputFlags::INPUT_MOVEDOWN;

				if (event.key.code == sf::Keyboard::A) game.playerInput &= ~InputFlags::INPUT_MOVELEFT;

				if (event.key.code == sf::Keyboard::W) game.playerInput &= ~InputFlags::INPUT_MOVEUP;
			}

			if (event.type == sf::Event::MouseButtonPressed)
			{
				if (event.mouseButton.button == sf::Mouse::Left) game.playerInput |= InputFlags::INPUT_SHOOT;

				if (event.mouseButton.button == sf::Mouse::Right) game.playerInput |= InputFlags::INPUT_GRAB;
			}

			if (event.type == sf::Event::MouseButtonReleased)
			{
				if (event.mouseButton.button == sf::Mouse::Left) game.playerInput &= ~InputFlags::INPUT_SHOOT;

				if (event.mouseButton.button == sf::Mouse::Right)
				{
					game.playerInput &= ~InputFlags::INPUT_GRAB;
					game.playerInput |= InputFlags::INPUT_DROP;
				}
			}

			if (event.type == sf::Event::MouseWheelScrolled)
			{
				if (event.mouseWheelScroll.delta > 0) game.playerInput |= InputFlags::INPUT_ZOOMIN;
				if (event.mouseWheelScroll.delta < 0) game.playerInput |= InputFlags::INPUT_ZOOMOUT;
			}	
		}

		if (game.engineFlags & EngineFlags::ENGINE_INPUT)
		{
			// HANDLE PLAYER SHOOT INPUT
			if (game.playerInput & InputFlags::INPUT_SHOOT)
			{
				
			}

			// HANDLE GRAB INPUT
			if (game.playerInput & InputFlags::INPUT_GRAB)
			{
				if (game.grabbedID == -1)
				{

					for (int i = 0; i < game.itemCount; i++)
					{
						int id = game.itemIDs[i];

						if (game.states[id] & EntityFlags::STATE_ALIVE)
						{
							sf::Vector2f delta = game.mousePos - game.positions[id];
							float distSq = delta.x * delta.x + delta.y * delta.y;

							if (distSq < GRAB_DISTANCE)
							{
								game.grabbedID = id;
								game.states[id] |= EntityFlags::STATE_GRABBED;
								break;
							}
						}
					}

				}
			}

			// HANDLE SHOOT & GRAB INPUT
			if (game.playerInput & 0b110000)
			{
				//game.health -= GameConfig::HEALTH_COST * deltaTime;
				//regenTimer.restart();
			}


			sf::View currentView = window.getView();
			game.viewBounds.left = currentView.getCenter().x - currentView.getSize().x / 2.f;
			game.viewBounds.top = currentView.getCenter().y - currentView.getSize().y / 2.f;
			game.viewBounds.width = currentView.getSize().x;
			game.viewBounds.height = currentView.getSize().y;


			if (game.playerInput & InputFlags::INPUT_DROP)
			{
				game.velocities[game.grabbedID] *= GameConfig::DROP_FORCE;
				game.states[game.grabbedID] &= ~EntityFlags::STATE_GRABBED;
				game.grabbedID = -1;
				game.playerInput &= ~InputFlags::INPUT_DROP;
			}

			// HANDLE PLAYER MOVEMENT INPUT
			if (game.playerInput & 0b1111)
			{
				//game.playerState |= EntityFlags::STATE_MOVING;
				//if (game.playerInput & 0b1) game.velocity += { 1 * GamePhysics::PLAYER_TURNING, 0 };
				//if (game.playerInput & 0b10) game.velocity += { 0, 1 * GamePhysics::PLAYER_TURNING };
				//if (game.playerInput & 0b100) game.velocity -= { 1 * GamePhysics::PLAYER_TURNING, 0 };
				//if (game.playerInput & 0b1000) game.velocity -= { 0, 1 * GamePhysics::PLAYER_TURNING };
				
				if (game.playerInput & InputFlags::INPUT_MOVERIGHT) entities.velocity[playerID] += { 1 * GamePhysics::PLAYER_TURNING, 0 };
				if (game.playerInput & InputFlags::INPUT_MOVEDOWN) entities.velocity[playerID] += { 0, 1 * GamePhysics::PLAYER_TURNING };
				if (game.playerInput & InputFlags::INPUT_MOVELEFT) entities.velocity[playerID] -= { 1 * GamePhysics::PLAYER_TURNING, 0 };
				if (game.playerInput & InputFlags::INPUT_MOVEUP) entities.velocity[playerID] -= { 0, 1 * GamePhysics::PLAYER_TURNING };

			}
			else
			{
				//game.playerState &= ~EntityFlags::STATE_MOVING;
				//game.velocity *= GamePhysics::PLAYER_FRICTION;
				//if (std::abs(game.velocity.x) < 0.01f) game.velocity.x = 0.f;
				//if (std::abs(game.velocity.y) < 0.01f) game.velocity.y = 0.f;

				if (std::abs(entities.velocity[playerID].x) < 0.01f) entities.velocity[playerID].x = 0.f;
				if (std::abs(entities.velocity[playerID].y) < 0.01f) entities.velocity[playerID].y = 0.f;
			}

			// HANDLE ZOOM
			if (game.playerInput & InputFlags::INPUT_ZOOMIN)
			{
				view.zoom(0.9f);
				game.playerInput &= ~InputFlags::INPUT_ZOOMIN;
			}
			if (game.playerInput & InputFlags::INPUT_ZOOMOUT)
			{
				view.zoom(1.1f);
				game.playerInput &= ~InputFlags::INPUT_ZOOMOUT;
			}
			view.zoom(game.zoom);
		}
		
		// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
		// AI //
		// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
		if (game.engineFlags & EngineFlags::ENGINE_AI)
		{
			for (int i = 0; i < game.enemyCount; i++)
			{
				int id = game.enemyIDs[i];

				if (!(game.states[id] & EntityFlags::STATE_ALIVE)) continue;

				int neighborCount = 0;

				sf::Vector2f separation(0.f, 0.f);
				sf::Vector2f alignment(0.f, 0.f);
				sf::Vector2f cohesion(0.f, 0.f);

				sf::Vector2f vel = game.velocities[id];

				for (int j = 0; j < game.enemyCount; j++)
				{
					int jd = game.enemyIDs[j];

					if (!(game.states[jd] & EntityFlags::STATE_ALIVE)) continue;

					sf::Vector2f delta = game.positions[jd] - game.positions[id];
					float distSq = delta.x * delta.x + delta.y * delta.y;

					if (distSq < FlockAI::FLOCK_RADIUS * FlockAI::FLOCK_RADIUS)
					{
						float dist = std::sqrt(distSq);
						separation -= delta / (dist + 1.f);
						alignment += game.velocities[jd];
						cohesion += game.positions[jd];
						neighborCount++;
					}
				}

				if (neighborCount > 0)
				{
					alignment /= static_cast<float>(neighborCount);
					cohesion /= static_cast<float>(neighborCount);
					cohesion -= game.positions[id];

					sf::Vector2f steer = separation * FlockAI::FLOCK_SEPARATION + alignment * FlockAI::FLOCK_ALIGNMENT + cohesion * FlockAI::FLOCK_COHESION;
					vel += steer * deltaTime;

					float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
					if (speed > FlockAI::FLOCK_SPEED) vel = (vel / speed) * FlockAI::FLOCK_SPEED;
					game.velocities[id] = vel;
				}
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
		if (game.engineFlags & EngineFlags::ENGINE_PHYSICS)
		{
			// ITEM PHYSICS
			for (int i = 0; i < game.itemCount; i++)
			{
				int id = game.itemIDs[i];

				if (!(game.states[id] & EntityFlags::STATE_ALIVE)) continue;

				if (game.states[id] & EntityFlags::STATE_GRABBED)
				{
					sf::Vector2f delta = game.mousePos - game.positions[id];
					game.velocities[id] = delta * GamePhysics::GRAB_SPRING;
				}

				game.velocities[id] *= GamePhysics::ITEM_FRICTION;
			}

			// ENEMY PHYSICS
			for (int i = 0; i < game.enemyCount; i++)
			{
				int id = game.enemyIDs[i];

				if (!(game.states[id] & EntityFlags::STATE_ALIVE)) continue;

				game.velocities[id] *= GamePhysics::ENEMY_FRICTION;
			}

			// PLAYER PHYSICS
			if (entities.flags[playerID] & PoolFlags::Spawned) //&& game.playerState & EntityFlags::STATE_MOVING)
			{
				// Test performance - research alternatives.
				float distSqVel = entities.velocity[playerID].x * entities.velocity[playerID].x + entities.velocity[playerID].y * entities.velocity[playerID].y;
				float distVel = std::sqrt(distSqVel);
				if (distVel == 0.0f) distVel = 0.001f;
				entities.velocity[playerID] = (entities.velocity[playerID] / distVel) * GamePhysics::PLAYER_SPEED;

				entities.velocity[playerID] *= GamePhysics::PLAYER_FRICTION;
				entities.position[playerID] += entities.velocity[playerID] * deltaTime;
			}

			// ALL ENTITIES
			for (int i = 0; i < game.entityCount; i++)
			{
				if (!(game.states[i] & EntityFlags::STATE_ALIVE)) continue;
					
				game.positions[i] += game.velocities[i] * deltaTime;
			}
		}

		// ############################################################################################################################
		// COLLISIONS AND INTERACTIONS //
		// ############################################################################################################################
		if (game.engineFlags & EngineFlags::ENGINE_COLLISION)
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
			for (int i = 0; i < game.itemCount; i++)
			{
				int id = game.itemIDs[i];

				if (!(game.states[id] & EntityFlags::STATE_ALIVE)) continue;

				for (int j = 0; j < game.enemyCount; j++)
				{
					int jd = game.itemIDs[j];

					if (!(game.states[jd] & EntityFlags::STATE_ALIVE)) continue;

					sf::Vector2f delta = game.positions[id] - game.positions[jd];
					float distSq = delta.x * delta.x + delta.y * delta.y;

					if (distSq < GameCollision::COLLISION_ITEM_ENEMY)
					{
						game.velocities[jd] += game.velocities[id] * GameCollision::BULLET_FORCE * deltaTime;

						if (entities.flags[playerID] & PoolFlags::Spawned)
						{
							//enemies.state[j] &= ~0b1;
							//enemyCount--;
						}

						//states[id] &= ~STATE_ALIVE;
						break;
					}
				}
			}
			
			// ENEMY-PLAYER COLLISION
			for (int i = 0; i < game.enemyCount; i++)
			{
				int id = game.enemyIDs[i];

				if (id < 0 || id >= GameConfig::ENEMY_MAX) continue;
				if (!(game.states[id] & EntityFlags::STATE_ALIVE)) continue;

				sf::Vector2f delta = entities.position[playerID] - game.positions[id];
				float distSq = delta.x * delta.x + delta.y * delta.y;

				if (distSq < GameCollision::PLAYER_COLLISION)
				{
					game.health--;
					regenTimer.restart();
				}
			}
		}

		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// UPDATE STATES //
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// UPDATE HEALTH
		if (game.health <= 0) entities.flags[playerID] &= ~PoolFlags::Spawned;

		if (game.health < GameConfig::HEALTH_MAX && regenTimer.getElapsedTime().asSeconds() > GameConfig::REGEN_TIME) game.health += GameConfig::REGEN_SPEED * deltaTime;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// RENDERING //
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// VFX
		if (game.engineFlags & EngineFlags::ENGINE_RENDERING && game.engineFlags & EngineFlags::ENGINE_VFX)
		{
			// PLAYER VFX
			if (game.health < GameConfig::HEALTH_MAX && entities.flags[playerID] & PoolFlags::Spawned)
			{
				float hurt = (1.0f - static_cast<float>(game.health) / GameConfig::HEALTH_MAX);
				
				if (game.playerInput & 0b110000)
				{
					if (game.playerInput & InputFlags::INPUT_SHOOT)
					{
						float hurting = hurt * 0.8;
						hurtingColor = sf::Color(255 * hurting, 0, 255 * (1 - hurting));
					}

					if (game.playerInput & InputFlags::INPUT_GRAB)
					{
						float hurting = hurt * 0.8;
						hurtingColor = sf::Color(0, 255 * hurting, 255);
					}

					sf::FloatRect& bounds = game.targetBounds;

					sf::ConvexShape convex(4);
					convex.setPoint(0, sf::Vector2f(bounds.left, bounds.top));
					convex.setPoint(1, sf::Vector2f(bounds.left + bounds.width, bounds.top));
					convex.setPoint(2, sf::Vector2f(bounds.left + bounds.width, bounds.top + bounds.height));
					convex.setPoint(3, sf::Vector2f(bounds.left, bounds.top + bounds.height));

					convex.setFillColor(sf::Color(0, 0, 0, 0));
					convex.setOutlineColor(hurtingColor); // or hurtingColor
					convex.setOutlineThickness(2.0f);
					window.setView(view);

					window.draw(convex);
				}
				else
				{
					hurtingColor = sf::Color(0, 0, 255);
				}
				
				//sf::Vertex line[] = { sf::Vertex(game.position, hurtingColor), sf::Vertex(game.mousePos, sf::Color::Black)};
				//window.draw(line, 2, sf::Lines);

				player.setOutlineThickness(hurt);
				player.setOutlineColor(hurtingColor);
				player.setFillColor(hurtingColor);

				std::cout << hurt << std::endl;
			}

			// ENEMY FLOCK DEBUG VFX
			if (game.engineFlags & EngineFlags::ENGINE_DEBUG && entities.flags[playerID] & PoolFlags::Spawned)
			{
				for (int i = 0; i < game.enemyCount; i++)
				{
					int id = game.enemyIDs[i];

					if (!(game.states[id] & EntityFlags::STATE_ALIVE)) continue;

					for (int j = 0; j < game.enemyCount; j++)
					{
						int jd = game.enemyIDs[j];

						if (!(game.states[jd] & EntityFlags::STATE_ALIVE)) continue;

						sf::Vector2f delta = game.positions[jd] - game.positions[id];
						float distSq = delta.x * delta.x + delta.y * delta.y;

						if (distSq < GameVFX::FLOCK_LINE * GameVFX::FLOCK_LINE)
						{
							sf::Vector2f midPoint = (game.positions[id] + game.positions[jd]) * 0.5f;

							sf::Vertex flockLine[] = {
								sf::Vertex(game.positions[id], sf::Color::Red),
								sf::Vertex(midPoint, sf::Color::Black),
								sf::Vertex(game.positions[jd], sf::Color::Red)
							};

							window.draw(flockLine, 3, sf::LineStrip);

							if (distSq < GameVFX::FLOCK_HUDDLE * GameVFX::FLOCK_HUDDLE)
							{
								sf::Vector2f midPoint = (game.positions[id] + game.positions[jd]) * 0.5f;

								sf::Vertex flockLine[] = { sf::Vertex(game.positions[id], sf::Color::Red), sf::Vertex(game.positions[jd], sf::Color::Red) };

								window.draw(flockLine, 2, sf::LineStrip);
							}
						}

						//
						//sf::Vertex moveLine[] = { sf::Vertex(positions[id], sf::Color::Red), sf::Vertex(positions[jd], sf::Color::Black)};
						//window.draw(moveLine, 2, sf::Lines);
					}
				}
			}
			

		}
		
		// RENDER
		if (game.engineFlags & EngineFlags::ENGINE_RENDERING)
		{
			// RENDER ITEMS
			for (int i = 0; i < game.itemCount; i++)
			{
				int id = game.itemIDs[i];

				if (game.states[id] & EntityFlags::STATE_ALIVE)
				{
					item.setPosition(game.positions[id]);
					window.draw(item);
				}
			}

			// RENDER ENEMIES
			for (int i = 0; i < game.enemyCount; i++)
			{
				int id = game.enemyIDs[i];

				if (game.states[id] & EntityFlags::STATE_ALIVE)
				{
					enemy.setPosition(game.positions[id]);
					window.draw(enemy);
				}
			}

			// RENDER PLAYER
			if (entities.flags[playerID] & PoolFlags::Spawned)
			{
				float angle = std::atan2(game.aim.y, game.aim.x) * 180.f / 3.14159265f;
				player.setRotation(angle + 90);
			}
			player.setPosition(entities.position[playerID]);
			window.draw(player);

			// MOVE VIEW WINDOW
			game.deadZone.left = view.getCenter().x - game.deadZone.width / 2.f;
			game.deadZone.top = view.getCenter().y - game.deadZone.height / 2.f;

			//if (game.position.x < game.deadZone.left) view.move(game.position.x - game.deadZone.left, 0.f);
			//else if (game.position.x > game.deadZone.left + game.deadZone.width) view.move(game.position.x - (game.deadZone.left + game.deadZone.width), 0.f);

			//if (game.position.y < game.deadZone.top) view.move(0.f, game.position.y - game.deadZone.top);
			//else if (game.position.y > game.deadZone.top + game.deadZone.height) view.move(0.f, game.position.y - (game.deadZone.top + game.deadZone.height));

			// CLAMP VIEW SIZE
			if (view.getSize().x < 1 || view.getSize().y < 1) view.setSize({ 1,1 });
			//if (view.getSize().x > WORLD_WIDTH || view.getSize().y > WORLD_HEIGHT) view.setSize({ WORLD_WIDTH,WORLD_HEIGHT });
		}
		
		// HUD
		if (game.engineFlags & EngineFlags::ENGINE_RENDERING && game.gameFlags & GameFlags::GAME_HUD)
		{
			window.setView(pauseView);

			HUD.setString("HP " + std::to_string(game.health * 100 / GameConfig::HEALTH_MAX));
			if (!(entities.flags[playerID] & PoolFlags::Spawned)) HUD.setString("HP 0");

			HUD.setFillColor(sf::Color::Red);
			if (game.health > GameConfig::HEALTH_MAX * 0.20) HUD.setFillColor(sf::Color::Yellow);
			if (game.health > GameConfig::HEALTH_MAX * 0.70) HUD.setFillColor(sf::Color::Green);
			if (!(entities.flags[playerID] & PoolFlags::Spawned)) HUD.setFillColor(sf::Color::Red);

			//HUD.setPosition(player.getPosition()); // Must be on same view.

			window.draw(HUD);
		}

		// PAUSE SCREEN
		if (game.engineFlags & EngineFlags::ENGINE_RENDERING && game.gameFlags & GameFlags::GAME_PAUSE)
		{
			window.clear();
			window.setView(pauseView);

			//if (enemyCount > 1 && enemyCount < ENEMY_MAX) countString = std::to_string(enemyCount) + " enemies left";
			//else if(enemyCount == 1 && enemyCount != ENEMY_MAX) countString = "only 1 enemy left";!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

			//countText.setPosition(WINDOW_WIDTH * 0.5, WINDOW_HEIGHT * 0.3);
			//countText.setOrigin(countText.getLocalBounds().width * 0.5, countText.getLocalBounds().height * 0.5);

			pauseText.setFillColor(sf::Color::White);

			//overText.setFillColor(sf::Color::Red);
			//overText.setOutlineThickness(2);

			window.draw(triangle);
			window.draw(square);
			window.draw(triangle2);
			window.draw(pauseText);
			window.draw(triangleText);
			window.draw(squareText);
			window.draw(circleText);
			window.draw(countText);
		}

		// GAME OVER SCREEN
		if (game.engineFlags & EngineFlags::ENGINE_RENDERING && (entities.flags[playerID] & PoolFlags::Spawned) == 0 && (game.gameFlags & GameFlags::GAME_PAUSE) == 0)
		{
			window.setView(hudView);
			window.draw(overText);

			// GAME OVER VFX
			if (game.engineFlags & EngineFlags::ENGINE_VFX)
			{
				/*
				for (int i = 0; i < game.bulletCount; i++)
				{
					int id = game.bulletIDs[i];

					// SPAWN DEATH BULLETS
					if (!(game.states[id] & EntityFlags::STATE_ALIVE))
					{
						float deathSpin = rand() % 360 * GameVFX::DEATH_SPREAD;
						if (deathSpin > 360.f) deathSpin -= 360.f;

						float angle = deathSpin * 3.14159265f / 180.f;
						sf::Vector2f dir = { std::cos(angle), std::sin(angle) };

						game.states[id] |= EntityFlags::STATE_ALIVE;
						game.positions[id] = game.position + dir * GameVFX::DEATH_RADIUS;
						game.velocities[id] = dir * GameVFX::DEATH_SPEED;
						game.rotations[id] = angle + 90;
						break;
					}

					// DEATH BULLET-WINDOW COLLISION
					if (!game.viewBounds.contains(game.positions[id]))
					{
						//killBullet(id);  <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME
						break;
					}

					// DEATH BULLET-ITEM COLLISION
					for (int j = 0; j < game.itemCount; j++)
					{
						int jd = game.itemIDs[j];

						if (!(game.states[jd] & EntityFlags::STATE_ALIVE)) continue;

						sf::Vector2f delta = game.positions[id] - game.positions[jd];
						float distSq = delta.x * delta.x + delta.y * delta.y;

						if (distSq < GameCollision::COLLISION_BULLET_ITEM)
						{
							game.velocities[jd] += game.velocities[id] * GameVFX::DEATH_FORCE * deltaTime;
							//killBullet(id);  <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME
							break;
						}
					}

					// DEATH BULLET-ENEMY COLLISION
					for (int k = 0; k < game.enemyCount; k++)
					{
						int kd = game.enemyIDs[k];

						if (!(game.states[kd] & EntityFlags::STATE_ALIVE)) continue;

						sf::Vector2f delta = game.positions[id] - game.positions[kd];
						float distSq = delta.x * delta.x + delta.y * delta.y;

						if (distSq < GameCollision::COLLISION_BULLET_ENEMY)
						{
							game.velocities[kd] += game.velocities[id] * GameVFX::DEATH_FORCE * deltaTime;
							//killBullet(id);  <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME <3 FIX ME
							break;
						}
					}
				}
				*/
			}
		}

		// STATS SCREEN
		if (game.engineFlags & EngineFlags::ENGINE_RENDERING && game.gameFlags & GameFlags::GAME_STATS)
		{
			window.clear(sf::Color::White);
			pauseText.setFillColor(sf::Color::Black);

			window.draw(pauseText);


		}

		// VFX
		if (game.engineFlags & EngineFlags::ENGINE_RENDERING && game.engineFlags & EngineFlags::ENGINE_VFX)
		{
			// SHOOT & GRAB VFX
			if (game.playerInput & 0b110000)
			{
				













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
		}

		// HANDLE WINDOW
		window.setView(view);
		window.display();

		// HANDLE SCREENSHOT
		if (game.engineFlags & EngineFlags::ENGINE_DEBUG && screensave) takeScreenshot(window);
    }

    return 0;
};