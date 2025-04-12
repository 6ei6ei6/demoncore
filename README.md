# demoncore
Ongoing open source 2D shooter game in C++ and SFML.

This amateur project serves as an open testing ground for independent C++ game development. While the surface-level game is designed for fun and presentation, the true goal is to learn through practice — by building a custom game engine and experimenting with performance-optimized structures and, later on, enhanced graphical fidelity.

This project has started as a solo build but is open for all to participate and contribute. It's been built watching lectures on data orientated programming and the goal is to continue exploring this design approach. That said, contributors are free — and encouraged — to approach problems and implement ideas however they see fit in their own branches.

///////////////////////////////////////////////////////////////////////

The following are suggestions and common practices in the current code:

- Data should be stored in the smallest appropriate form. Minimizing memory usage is a priority and considering the size and type of data needed is vital. Using the most convenient types is fine for prototyping but should not necessarily be final.

- Instances are stored using a struct of arrays layout, where each index across arrays represents a single instance. There is no shared base class or object hierarchy — "objects" exist purely as their data. They only come together during system execution, when their values are processed in parallel.

- Systems operate by iterating over arrays, checking and setting values. These loops should be structured for predictable execution, maximizing access to shared or related data. The goal is to process as much relevant data as possible in a single pass — especially when fetching from deeper cache levels or main memory — to reduce latency and improve performance.

- The game is divided into pipelines or sections — first it handles input, followed by physics and collision, and then rendering. This is for organization and debugging purposes — even if it means looping through the same data over again.

///////////////////////////////////////////////////////////////////////

Pseudocode example:

// Objects have a state and a position in the world. Object oriented programming would have us make an object class. But in data orientated programming, we make a struct. This struct represents a pool of objects and has arrays for each of the object's state and position.

struct ObjectPool
{
	uint8_t state[MAX_OBJECTS]
	sf::Vector2f position[MAX_OBJECTS];
} objects;

// The state variable is 1 byte long (8 bits) and gives us 8 possible states (Booleans). We can define what we want those states to be by setting const expressions for the file.

constexpr uint8_t STATE_ALIVE = 1 << 0;
constexpr uint8_t STATE_VISIBLE = 1 << 1;
...
constexpr uint8_t STATE_BROKEN = 1 << 7;

// Once in the main function, we iterate through the objects array to initialize, get or set variables and apply game logic.

for(size_t i = 0; i < MAX_OBJECTS; i++) objects.position[i] = {0,0};

// We then use bit masking to set and test the states.

for(size_t i = 0; i < MAX_OBJECTS; i++) objects.state[i] |= STATE_ALIVE;

for(size_t i = 0; i < MAX_OBJECTS; i++)
{
	if(objects.state[i] & STATE_ALIVE) render(object, objects.position[i]);
}