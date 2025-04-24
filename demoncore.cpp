// demoncore.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Font.hpp>

namespace ConfigData
{
	constexpr int WINDOW_WIDTH = 1000;
	constexpr int WINDOW_HEIGHT = 1000;
	constexpr int WORLD_WIDTH = 1000;
	constexpr int WORLD_HEIGHT = 1000;
	constexpr int FPS_MAX = 60;
	constexpr int HUD_SIZE = 40;
	constexpr float HEALTH_MAX = 200;
	constexpr float HEALTH_COST = 70;
	constexpr float DROP_FORCE = 3;
	constexpr float REGEN_TIME = 0.2;
	constexpr float REGEN_SPEED = 100;
	constexpr float DEADZONE_WIDTH = 100;
	constexpr float DEADZONE_HEIGHT = 100;
}

namespace FlockData
{
	constexpr const float FLOCK_RADIUS = 200;
	constexpr const float FLOCK_SPEED = 20;
	constexpr const float FLOCK_SEPARATION = 100.0;
	constexpr const float FLOCK_ALIGNMENT = 10.0;
	constexpr const float FLOCK_COHESION = 5.0;
}

namespace PhysicsData
{
	constexpr float MAX_SPEED = 200; // Sprint = 300 ?
	constexpr float TURN_SPEED = 10;
	constexpr float ACCELERATION = 100;
	constexpr float FRICTION = 0.8;
	constexpr float GRAB_SPRING = 5;
}

namespace CollisionData
{
	constexpr float ENTITY_RADIUS = 10;

	constexpr float TARGET_PADDING = 2;
	constexpr float ENTITY_TARGET = (ENTITY_RADIUS + TARGET_PADDING)
								  * (ENTITY_RADIUS + TARGET_PADDING);

	constexpr float GRAB_PADDING = 10;
	constexpr float ENTITY_GRAB = (ENTITY_RADIUS + GRAB_PADDING)
								* (ENTITY_RADIUS + GRAB_PADDING);

	constexpr float COLLISION_PADDING = 10;
	constexpr float ENTITY_COLLISION = (ENTITY_RADIUS + COLLISION_PADDING)
									 * (ENTITY_RADIUS + COLLISION_PADDING);
}

namespace EngineFlags
{
	constexpr uint16_t ENGINE_PAUSE = 1 << 0;
	constexpr uint16_t ENGINE_DEBUG = 1 << 1;
	constexpr uint16_t ENGINE_INPUT = 1 << 2;
	constexpr uint16_t ENGINE_AI = 1 << 3;
	constexpr uint16_t ENGINE_PHYSICS = 1 << 4;
	constexpr uint16_t ENGINE_COLLISION = 1 << 5;
	constexpr uint16_t ENGINE_RENDERING = 1 << 6;
	constexpr uint16_t ENGINE_VFX = 1 << 7;
	constexpr uint16_t ENGINE_HUD = 1 << 8;
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
	constexpr uint16_t INPUT_BRAKERIGHT = 1 << 9;
	constexpr uint16_t INPUT_BRAKEDOWN = 1 << 10;
	constexpr uint16_t INPUT_BRAKELEFT = 1 << 11;
	constexpr uint16_t INPUT_BRAKEUP = 1 << 12;
}

namespace RenderFlags
{
	constexpr uint8_t Enemy = 1 << 0;
	constexpr uint8_t Item = 1 << 1;
	constexpr uint8_t Player = 1 << 2;
}

namespace EntityFlags
{
	constexpr uint8_t Spawned = 1 << 0;
	constexpr uint8_t Possessed = 1 << 1;
	constexpr uint8_t PhysicsEnabled = 1 << 2;
	constexpr uint8_t CollisionEnabled = 1 << 3;
	constexpr uint8_t Visible = 1 << 4;
	constexpr uint8_t Hit = 1 << 5;
	constexpr uint8_t Grabbed = 1 << 6;
}

struct GameState
{
	uint16_t engineFlags = 0b111111110;

	uint16_t playerInput = 0;

	int playerID = -1;

	float health = ConfigData::HEALTH_MAX;

	float zoom = 1.f;

	sf::Vector2f dir = { 0,0 };
	sf::Vector2f aim = { 0,0 };
	sf::Vector2f mousePos = { 0,0 };

	sf::FloatRect viewBounds;
	sf::FloatRect deadZone;
} game;

struct EntityPool
{
	int poolSize;

	std::vector<int> availableIDs;
	std::vector<int> activeIDs;

	std::vector<uint8_t> flags;

	std::vector<sf::Vector2f> position;
	std::vector<sf::Vector2f> velocity;

	std::vector<uint8_t> render;

	EntityPool(int size) : poolSize(size)
	{
		availableIDs.reserve(size);
		activeIDs.reserve(size);

		flags.resize(size, 0);

		position.resize(size, {0,0});
		velocity.resize(size, {0,0});

		render.resize(size, 0);

		for (int i = 0; i < poolSize; i++)
		{
			availableIDs.push_back(i);
		}
	}
} entities(100);

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

void spawnEnemy()
{
	int id = acquireEntity();
	if (id == -1) return;

	entities.flags[id] |= EntityFlags::Spawned;
	entities.flags[id] |= EntityFlags::Possessed;
	entities.flags[id] |= EntityFlags::PhysicsEnabled;
	entities.flags[id] |= EntityFlags::CollisionEnabled;
	entities.flags[id] |= EntityFlags::Visible;

	sf::Vector2f spawnPos = { static_cast<float>(rand() % ConfigData::WORLD_WIDTH),
							  static_cast<float>(rand() % ConfigData::WORLD_HEIGHT) };

	entities.position[id] = spawnPos;
	entities.velocity[id] = { 0,0 };
	entities.render[id] |= RenderFlags::Enemy;
}

void spawnItem()
{
	int id = acquireEntity();
	if (id == -1) return;

	entities.flags[id] |= EntityFlags::Spawned;
	entities.flags[id] |= EntityFlags::CollisionEnabled;
	entities.flags[id] |= EntityFlags::Visible;

	sf::Vector2f spawnPos = { static_cast<float>(rand() % ConfigData::WORLD_WIDTH),
							  static_cast<float>(rand() % ConfigData::WORLD_HEIGHT) };

	entities.position[id] = spawnPos;
	entities.velocity[id] = { 0,0 };
	entities.render[id] |= RenderFlags::Item;
}

void spawnPlayer()
{
	int id = acquireEntity();
	if (id == -1) return;
	game.playerID = id;

	entities.flags[id] |= EntityFlags::Spawned;
	entities.flags[id] |= EntityFlags::PhysicsEnabled;
	entities.flags[id] |= EntityFlags::CollisionEnabled;
	entities.flags[id] |= EntityFlags::Visible;

	entities.position[id] = { 0,0 };
	entities.velocity[id] = { 0,0 };

	entities.render[id] |= RenderFlags::Player;
}

void destroyEntity(int id)
{
	entities.flags[id] = 0;
	entities.position[id] = { 0, 0 };
	entities.velocity[id] = { 0, 0 };
	entities.render[id] = 0;

	releaseEntity(id);
}

void initShape(sf::CircleShape& shape, uint8_t renderFlag)
{
	shape.setRadius(CollisionData::ENTITY_RADIUS);
	shape.setOrigin({ CollisionData::ENTITY_RADIUS, CollisionData::ENTITY_RADIUS });

	shape.setScale({ 1, 1 });
	shape.setOutlineThickness(0);
	shape.setOutlineColor(sf::Color::Transparent);
	
	if (renderFlag & RenderFlags::Enemy)
	{
		shape.setPointCount(3);
		shape.setFillColor(sf::Color::Red);
	}

	if (renderFlag & RenderFlags::Item)
	{
		shape.setPointCount(4);
		shape.setFillColor(sf::Color::Green);
	}

	if (renderFlag & RenderFlags::Player)
	{
		shape.setPointCount(30);
		shape.setFillColor(sf::Color::Blue);
	}
}

bool screensave = false;
int screenshotCount = 0;

float deltaTime;

sf::Clock deltaClock;
sf::Clock regenTimer;

sf::Font font;

sf::Event event;

sf::CircleShape shape(1, 1);

sf::Text text("", font);
sf::Text HUD("HUD", font, ConfigData::HUD_SIZE);

sf::RenderWindow window(sf::VideoMode({ ConfigData::WINDOW_WIDTH, ConfigData::WINDOW_HEIGHT }), "demoncore");
sf::View view({ 0.f,200.f }, { ConfigData::WINDOW_WIDTH, ConfigData::WINDOW_HEIGHT });
const sf::View pauseView = window.getDefaultView();
const sf::View hudView = window.getDefaultView();

void initHUD()
{
	if (!font.loadFromFile("font.ttf"))
	{
		std::cout << "Error loading font!\n";
		return;
	}

	HUD.setOutlineColor(sf::Color::Black);
	HUD.setOutlineThickness(2);
	HUD.setLetterSpacing(5);	
}

// NEEDS REWRITING <3 NEEDS REWRITING <3 NEEDS REWRITING <3 NEEDS REWRITING <3 NEEDS REWRITING <3 NEEDS REWRITING <3
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

// NEEDS REWRITING <3 NEEDS REWRITING <3 NEEDS REWRITING <3 NEEDS REWRITING <3 NEEDS REWRITING <3 NEEDS REWRITING <3
void loadGame() 
{
	std::ifstream in("save.txt", std::ios::binary);
	
	if (in)
	{
		if(game.engineFlags & EngineFlags::ENGINE_DEBUG)
		std::cout << "Loading latest save...\n";
	}
	else
	{
		if (game.engineFlags & EngineFlags::ENGINE_DEBUG)
		std::cout << "Error loading game.\n";

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

	if (screenshot.saveToFile(filename) && game.engineFlags & EngineFlags::ENGINE_DEBUG)
		std::cout << "Screenshot saved to " << filename << "\n";

	screenshotCount++;
	screensave = false;
}

void printVec(const std::vector<int>& vec, const std::string& header)
{
	std::cout << "\n" << header << " : " << vec.size() << "\n";
	for (int i = 0; i < vec.size(); i++)
	{
		std::cout << vec[i] << " ";
	}
	std::cout << "\n";
}

sf::Vector2f normalize(sf::Vector2f vec)
{
	float distSq = vec.x * vec.x + vec.y * vec.y;
	float dist = sqrt(distSq);
	if (dist == 0.0f) dist = 0.000001f;
	return vec / dist;
}

int main()
{
	spawnEnemy(); // NEEDS LOVE <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3
	spawnItem(); // NEEDS LOVE <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3
	spawnPlayer(); // NEEDS LOVE <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3

	initHUD();
	
	game.deadZone.width = ConfigData::DEADZONE_WIDTH;
	game.deadZone.height = ConfigData::DEADZONE_HEIGHT;

	view.setCenter(entities.position[game.playerID]);
	window.setFramerateLimit(ConfigData::FPS_MAX);

	// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
	// MAIN LOOP //
	// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
	while (window.isOpen())
	{
		// HANDLE FRAME WHEN SPAWNED
		if (entities.flags[game.playerID] & EntityFlags::Spawned)
		{
			window.clear();
			deltaTime = deltaClock.restart().asSeconds();

			game.mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

			// Calculate aim
			sf::Vector2f deltaAim = game.mousePos - entities.position[game.playerID];
			game.aim = normalize(deltaAim);

			// Get view bounds
			sf::View currentView = window.getView();
			game.viewBounds.left = currentView.getCenter().x - currentView.getSize().x / 2.f;
			game.viewBounds.top = currentView.getCenter().y - currentView.getSize().y / 2.f;
			game.viewBounds.width = currentView.getSize().x;
			game.viewBounds.height = currentView.getSize().y;
		}

		// 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
		// PLAYER INPUT & EVENTS //
		// 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
		// INPUTS
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed) window.close();
			
			if (event.type == sf::Event::KeyPressed)
			if (event.key.code == sf::Keyboard::Escape) game.engineFlags ^= EngineFlags::ENGINE_PAUSE;

			if (event.type == sf::Event::KeyPressed)
			if (event.key.code == sf::Keyboard::End) entities.flags[game.playerID] ^= EntityFlags::Spawned;

			if (game.engineFlags & EngineFlags::ENGINE_DEBUG)
			{
				if (event.type == sf::Event::KeyPressed)
				{
					if (event.key.code == sf::Keyboard::P) screensave = true;
					if (event.key.code == sf::Keyboard::Tab) saveGame(); // SAVE ME (ironic haha) <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3
					if (event.key.code == sf::Keyboard::BackSpace) loadGame(); // SAVE ME (ironic haha) <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3

					if (event.key.code == sf::Keyboard::Add) spawnEnemy();
					if (event.key.code == sf::Keyboard::Subtract) //destroyEntity();
					if (event.key.code == sf::Keyboard::Enter) ;

				}
			}

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

		// HANDLE INPUT //
		if (game.engineFlags & EngineFlags::ENGINE_INPUT)
		{
			// Hanlde movement
			if (game.playerInput & 0b1111)
			{
				sf::Vector2f dir = { 0,0 };

				if (game.playerInput & InputFlags::INPUT_MOVERIGHT) dir.x += 1.f;
				if (game.playerInput & InputFlags::INPUT_MOVELEFT)  dir.x -= 1.f;
				if (game.playerInput & InputFlags::INPUT_MOVEDOWN)  dir.y += 1.f;
				if (game.playerInput & InputFlags::INPUT_MOVEUP)    dir.y -= 1.f;

				game.dir += (normalize(dir) - game.dir) * deltaTime * PhysicsData::TURN_SPEED;
				game.dir = normalize(game.dir);
			}
			else
			{
				game.dir = { 0,0 };
			}

			// Handle shoot & grab
			for (int i = 0; i < entities.activeIDs.size(); i++)
			{
				int id = entities.activeIDs[i];

				if (game.playerInput & 0b110000)
				{
					sf::Vector2f delta = game.mousePos - entities.position[id];
					float distSq = delta.x * delta.x + delta.y * delta.y;

					if (game.playerInput & InputFlags::INPUT_SHOOT)
					{
						if (distSq < CollisionData::ENTITY_TARGET)
						{
							if(id != game.playerID) destroyEntity(id);
						}
					}
					
					if (game.playerInput & InputFlags::INPUT_GRAB)
					{
						if (distSq < CollisionData::ENTITY_GRAB)
						{
							entities.flags[id] |= EntityFlags::Grabbed;
						}
					}
				}
			}

			// Handle shoot | grab
			if (game.playerInput & 0b110000)
			{
				game.health -= ConfigData::HEALTH_COST * deltaTime;
				regenTimer.restart();
			}

			if (game.playerInput & InputFlags::INPUT_DROP);

			
			// Handle zoom
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

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// HANDLE PAUSE //
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if (game.engineFlags & EngineFlags::ENGINE_PAUSE)
		{
			game.engineFlags &= ~EngineFlags::ENGINE_AI;
			game.engineFlags &= ~EngineFlags::ENGINE_PHYSICS;
			game.engineFlags &= ~EngineFlags::ENGINE_COLLISION;
		}
		else if (!(game.engineFlags & EngineFlags::ENGINE_PAUSE))
		{
			if (!(game.engineFlags & EngineFlags::ENGINE_AI)) game.engineFlags |= EngineFlags::ENGINE_AI;
			if (!(game.engineFlags & EngineFlags::ENGINE_PHYSICS)) game.engineFlags |= EngineFlags::ENGINE_PHYSICS;
			if (!(game.engineFlags & EngineFlags::ENGINE_COLLISION)) game.engineFlags |= EngineFlags::ENGINE_COLLISION;
		}

		// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
		// AI //
		// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
		if (game.engineFlags & EngineFlags::ENGINE_AI)
		{
			// FLOCKING BEHAVIOR
			for (int i = 0; i < entities.activeIDs.size(); i++)
			{
				int id = entities.activeIDs[i];

				if (!(entities.flags[id] & EntityFlags::Possessed)) continue;

				sf::Vector2f vel = entities.velocity[id];

				int neighborCount = 0;

				sf::Vector2f separation(0.f, 0.f);
				sf::Vector2f alignment(0.f, 0.f);
				sf::Vector2f cohesion(0.f, 0.f);

				for (int j = i + 1; j < entities.activeIDs.size(); j++)
				{
					int jd = entities.activeIDs[j];

					if (!(entities.flags[jd] & EntityFlags::Possessed)) continue;
					
					sf::Vector2f delta = entities.position[jd] - entities.position[id];
					float distSq = delta.x * delta.x + delta.y * delta.y;

					if (distSq < FlockData::FLOCK_RADIUS * FlockData::FLOCK_RADIUS)
					{
						sf::Vertex moveLine[] = { sf::Vertex(entities.position[jd], sf::Color::Red)
												, sf::Vertex(entities.position[id], sf::Color::Black) };

						window.draw(moveLine, 2, sf::Lines);

						float dist = std::sqrt(distSq);
						separation -= delta / (dist + 1.f);
						alignment += entities.velocity[jd];
						cohesion += entities.position[jd];
						neighborCount++;
					}
				}

				if (neighborCount > 0)
				{
					alignment /= static_cast<float>(neighborCount);
					cohesion /= static_cast<float>(neighborCount);
					cohesion -= entities.position[id];

					sf::Vector2f steer = separation * FlockData::FLOCK_SEPARATION
										+ alignment * FlockData::FLOCK_ALIGNMENT
										+ cohesion * FlockData::FLOCK_COHESION;

					vel += steer * deltaTime;

					float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
					if (speed > FlockData::FLOCK_SPEED) vel = (vel / speed) * FlockData::FLOCK_SPEED;
					entities.velocity[id] = vel;
				}
			}
		}

		// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		// PHYSICS //
		// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		if (game.engineFlags & EngineFlags::ENGINE_PHYSICS)
		{
			// Player physics
			if (entities.flags[game.playerID] & EntityFlags::Spawned)
			{
				entities.velocity[game.playerID] += game.dir * PhysicsData::ACCELERATION;

				entities.velocity[game.playerID].x = std::max(-PhysicsData::MAX_SPEED, 
													 std::min(entities.velocity[game.playerID].x, PhysicsData::MAX_SPEED));

				entities.velocity[game.playerID].y = std::max(-PhysicsData::MAX_SPEED,
													 std::min(entities.velocity[game.playerID].y, PhysicsData::MAX_SPEED));
			}

			// Entity physics
			for (int i = 0; i < entities.activeIDs.size(); i++)
			{
				int id = entities.activeIDs[i];
				
				if (!(entities.flags[id] & EntityFlags::PhysicsEnabled)) continue;

				entities.velocity[id] *= PhysicsData::FRICTION;

				entities.position[id] += entities.velocity[id] * deltaTime;
			}
		}

		// ############################################################################################################################
		// COLLISION //
		// ############################################################################################################################
		if (game.engineFlags & EngineFlags::ENGINE_COLLISION)
		{
			for (int i = 0; i < entities.activeIDs.size(); i++)
			{
				int id = entities.activeIDs[i];

				if (!(entities.flags[id] & EntityFlags::CollisionEnabled)) continue;
				
				for (int j = i + 1; j < entities.activeIDs.size(); j++)
				{
					int jd = entities.activeIDs[j];

					if (!(entities.flags[jd] & EntityFlags::CollisionEnabled)) continue;

					sf::Vector2f delta = entities.position[id] - entities.position[jd];
					float distSq = delta.x * delta.x + delta.y * delta.y;
					
					if (distSq < CollisionData::ENTITY_COLLISION)
					{
						sf::Vector2f push = normalize(delta) * 0.5f;
						entities.position[id] += push;
						entities.position[jd] -= push;

						// Enemy-player collision
						if ((id == game.playerID && entities.flags[jd] & EntityFlags::Possessed)
						|| (jd == game.playerID && entities.flags[id] & EntityFlags::Possessed))
						{
							game.health--;
							regenTimer.restart();
						}
					}
				}
			}
		}

		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// UPDATE STATES (?) //
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// Update health
		//if (game.health <= 0) entities.flags[game.playerID] &= ~EntityFlags::Spawned;

		if (game.health < ConfigData::HEALTH_MAX && regenTimer.getElapsedTime().asSeconds() > ConfigData::REGEN_TIME)
		{
			float t = game.health / ConfigData::HEALTH_MAX;
			float smoothT = t*t;
			float regenAmount = (1.0f - smoothT) * ConfigData::REGEN_SPEED * deltaTime;

			game.health += regenAmount;

			game.health = std::max(0.f, 
						  std::min(game.health, ConfigData::HEALTH_MAX));

			if (game.health / ConfigData::HEALTH_MAX > 0.995) game.health = ConfigData::HEALTH_MAX;
		}

		// Update and move window
		game.deadZone.left = view.getCenter().x - game.deadZone.width / 2.f;
		game.deadZone.top = view.getCenter().y - game.deadZone.height / 2.f;

		if (entities.position[game.playerID].x < game.deadZone.left)
			view.move(entities.position[game.playerID].x - game.deadZone.left, 0.f);

		else if (entities.position[game.playerID].x > game.deadZone.left + game.deadZone.width)
			view.move(entities.position[game.playerID].x - (game.deadZone.left + game.deadZone.width), 0.f);

		if (entities.position[game.playerID].y < game.deadZone.top)
			view.move(0.f, entities.position[game.playerID].y - game.deadZone.top);

		else if (entities.position[game.playerID].y > game.deadZone.top + game.deadZone.height)
			view.move(0.f, entities.position[game.playerID].y - (game.deadZone.top + game.deadZone.height));

		// Clamp view size
		if (view.getSize().x < 1 || view.getSize().y < 1) view.setSize({ 1,1 });

		// Update window titles
		if (game.engineFlags & EngineFlags::ENGINE_DEBUG)
		{
			std::string title = "demoncore";

			float fps = 1.0f / deltaTime;
			int level = 0;
			int enemies = 0;

			for (int i = 0; i < entities.activeIDs.size(); i++)
			{
				int id = entities.activeIDs[i];

				if (entities.flags[id] & EntityFlags::Possessed)
				{
					enemies++;
				}
			}
			
			title = title + " "
				+ "(" + std::to_string(static_cast<int>(fps)) + " FPS) : "
				+ std::to_string(entities.activeIDs.size()) + " entities / "
				+ std::to_string(enemies) + " enemies"
				;

			window.setTitle(title);
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// RENDERING //
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (game.engineFlags & EngineFlags::ENGINE_RENDERING)
		{
			// VFX - UNDER ENTITIES
			if (game.engineFlags & EngineFlags::ENGINE_VFX)
			{
				// Telekinesis vfx
				if (game.playerInput & InputFlags::INPUT_GRAB)
				{
					static float t = 0.0f;
					t += deltaTime * 8.f;
					float strobe = 0.5f + 0.5f * std::sin(t);
					
					sf::Vector2f scale = { 1 + strobe, 1 + strobe };

					strobe = std::max(0.2f,
							 std::min(strobe, 1.f));

					sf::Color color = sf::Color(0, 255, 255, 255 * strobe);

					sf::CircleShape shapeT(12, 30);
					shapeT.setFillColor(sf::Color::Transparent);
					shapeT.setPosition(game.mousePos);
					shapeT.setOutlineThickness(1);
					shapeT.setOrigin(12, 12);

					shapeT.setOutlineColor(color);
					shapeT.setScale(scale);
					window.draw(shapeT);
				}
			}

			// RENDER ENTITIES
			for (int i = 0; i < entities.activeIDs.size(); i++)
			{
				int id = entities.activeIDs[i];

				if (!(entities.flags[id] & EntityFlags::Visible)) continue;

				initShape(shape, entities.render[id]);
				shape.setPosition(entities.position[id]);
				window.draw(shape);
			}

			// VFX - OVER ENTITIES
			if (game.engineFlags & EngineFlags::ENGINE_VFX)
			{
				// Laser vfx
				if (game.playerInput & InputFlags::INPUT_SHOOT)
				{
					sf::Vertex lineL[] = { sf::Vertex(entities.position[game.playerID], sf::Color::Red),
										  sf::Vertex(game.mousePos, sf::Color::Red) };

					window.draw(lineL, 2, sf::Lines);
				}

				// Player health vfx
				if (game.health < ConfigData::HEALTH_MAX && entities.flags[game.playerID] & EntityFlags::Spawned)
				{
					float hurt = (1.0f - static_cast<float>(game.health) / ConfigData::HEALTH_MAX);

					if (game.playerInput & 0b110000)
					{
						float padding = 1.f;
						if (game.playerInput & InputFlags::INPUT_SHOOT)
						{
							float hurting = hurt * padding;
							//hurtingColor = sf::Color(255 * hurting, 0, 0);
						}

						if (game.playerInput & InputFlags::INPUT_GRAB)
						{
							float hurting = hurt * padding;
							//hurtingColor = sf::Color(0, 255 * hurting, 255 * hurting);
						}
					}
					else
					{
						//hurtingColor = sf::Color::Transparent;
					}

					//sf::Vertex lineH[] = { sf::Vertex(entities.position[game.playerID], hurtingColor), sf::Vertex(game.mousePos, sf::Color::Black)};
					//window.draw(lineH, 2, sf::Lines);

					shape.setOutlineThickness(hurt);
				//	shape.setOutlineColor(hurtingColor);
				//	shape.setFillColor(hurtingColor);
				}
			}

			// HUD
			if (game.engineFlags & EngineFlags::ENGINE_HUD)
			{
				window.setView(pauseView);
				
				int value = game.health * 100 / ConfigData::HEALTH_MAX;
				HUD.setString("HP " + std::to_string(value));
				if (!(entities.flags[game.playerID] & EntityFlags::Spawned)) HUD.setString("HP 0");

				HUD.setFillColor(sf::Color::Red);
				if (game.health > ConfigData::HEALTH_MAX * 0.20) HUD.setFillColor(sf::Color::Yellow);
				if (game.health > ConfigData::HEALTH_MAX * 0.70) HUD.setFillColor(sf::Color::Green);
				if (!(entities.flags[game.playerID] & EntityFlags::Spawned)) HUD.setFillColor(sf::Color::Red);

				window.draw(HUD);
			}

			// PAUSE SCREEN
			if (game.engineFlags & EngineFlags::ENGINE_PAUSE)
			{
				window.clear();
				window.setView(pauseView);

				for (int i = 0; i < 3; i++)
				{
										
					sf::Vector2f posText;
					std::string string;

					sf::Vector2f posShape;
					sf::Color color;
					int points;
					
					shape.setRadius(100);
					shape.setOrigin(100, 100);

					switch (i)
					{
					case 0:
						posShape = { ConfigData::WINDOW_WIDTH * 0.25, ConfigData::WINDOW_HEIGHT * 0.5 };
						color = sf::Color::Red;
						points = 3;
						break;

					case 1:
						posShape = { ConfigData::WINDOW_WIDTH * 0.5, ConfigData::WINDOW_HEIGHT * 0.5 };
						color = sf::Color::Green;
						points = 4;
						break;

					case 2:
						posShape = { ConfigData::WINDOW_WIDTH * 0.75, ConfigData::WINDOW_HEIGHT * 0.5 };
						color = sf::Color::Blue;
						points = 30;
						break;
					}

					shape.setPosition(posShape);
					shape.setFillColor(color);
					shape.setPointCount(points);
										
					sf::Text pauseText("The game is paused. Press [Esc] to continue.", font, 24);
					pauseText.setOrigin(pauseText.getLocalBounds().width * 0.5, pauseText.getLocalBounds().height * 0.5);
					pauseText.setPosition(ConfigData::WINDOW_WIDTH * 0.5, ConfigData::WINDOW_HEIGHT * 0.2);
					pauseText.setFillColor(sf::Color::White);

					sf::Text enemyText("Enemy", font, 24);
					enemyText.setOrigin(enemyText.getLocalBounds().width * 0.5, enemyText.getLocalBounds().height * 0.5);
					enemyText.setPosition(ConfigData::WINDOW_WIDTH * 0.25, ConfigData::WINDOW_HEIGHT * 0.7);
					enemyText.setFillColor(sf::Color::White);

					sf::Text itemText("Item", font, 24);
					itemText.setOrigin(itemText.getLocalBounds().width * 0.5, itemText.getLocalBounds().height * 0.5);
					itemText.setPosition(ConfigData::WINDOW_WIDTH * 0.5, ConfigData::WINDOW_HEIGHT * 0.7);
					itemText.setFillColor(sf::Color::White);

					sf::Text playerText("Player", font, 24);
					playerText.setOrigin(playerText.getLocalBounds().width * 0.5, playerText.getLocalBounds().height * 0.5);
					playerText.setPosition(ConfigData::WINDOW_WIDTH * 0.75, ConfigData::WINDOW_HEIGHT * 0.7);
					playerText.setFillColor(sf::Color::White);

					window.draw(shape);
					window.draw(pauseText);
					window.draw(enemyText);
					window.draw(itemText);
					window.draw(playerText);
				}			
			}

			// GAME OVER SCREEN <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3
			if (!(entities.flags[game.playerID] & EntityFlags::Spawned) && !(game.engineFlags & EngineFlags::ENGINE_PAUSE))
			{
				sf::Text overText("GAME OVER", font, 50);
				overText.setPosition(ConfigData::WINDOW_WIDTH * 0.5, ConfigData::WINDOW_HEIGHT * 0.5);
				overText.setOrigin(overText.getLocalBounds().width * 0.5, overText.getLocalBounds().height * 0.5);
				overText.setOutlineColor(sf::Color::Black);
				overText.setFillColor(sf::Color::Black);
				overText.setOutlineThickness(0);
				overText.setLetterSpacing(5);			
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

			// DEBUG
			if (game.engineFlags & EngineFlags::ENGINE_DEBUG)
			{
				sf::Color debugColor = sf::Color(255, 255, 255, 255 * 0.2f);

				// World boundaries.
				window.setView(view);
				sf::RectangleShape rectW({ ConfigData::WORLD_WIDTH, ConfigData::WORLD_HEIGHT });
				rectW.setFillColor(sf::Color::Transparent);
				rectW.setOutlineColor(debugColor);
				rectW.setOutlineThickness(1);
				rectW.setPosition(0, 0);
				window.draw(rectW);

				// Aim debug line
				sf::Vertex lineA[] = { sf::Vertex(entities.position[game.playerID], debugColor),
									   sf::Vertex(entities.position[game.playerID] + game.aim * 20.f, debugColor) };
				window.draw(lineA, 2, sf::Lines);

				// Move debug line
				sf::Vertex lineM[] = { sf::Vertex(entities.position[game.playerID], debugColor),
									   sf::Vertex(entities.position[game.playerID] + game.dir * 20.f, debugColor) };
				window.draw(lineM, 2, sf::Lines);
			}

			// Display
			window.setView(view);
			window.display();

			// HANDLE SCREENSHOT
			if (game.engineFlags & EngineFlags::ENGINE_DEBUG && screensave) takeScreenshot(window);
		}
		
    }

    return 0;
};