#include <ctime>
#include <chrono>

#include <ncurses.h>
#include <stdlib.h>

// Constants
constexpr int BOARD_WIDTH = 20;
constexpr int BOARD_HEIGHT = 20;
constexpr int UNIT_TICK = 200;

// Type definitions
enum TileType { Empty, SnakeBody, Apple };

struct Position {
	int x, y;
};

class SnakeTile {
public:
	Position position;
	SnakeTile* next;

	SnakeTile(int x, int y) : position{x, y}, next(nullptr) {}
};

// Globals
SnakeTile* head = nullptr;
Position apple;

int direction_x = 0, direction_y = -1;
int delta_tick = 0;
int score = 1;

// Render board
void draw() {
	// Clear board
	attron(COLOR_PAIR(TileType::Empty));
	for (int y = 0; y < BOARD_HEIGHT; y++) {
		for (int x = 0; x < BOARD_WIDTH; x++) {
			move(y + 1, x * 2 + 1);
			printw("  ");
		}
	}
	attroff(COLOR_PAIR(TileType::Empty));

	// Draw snake
	attron(COLOR_PAIR(TileType::SnakeBody));
	for (SnakeTile* current_tile = head; current_tile; current_tile = current_tile->next) {
		move(current_tile->position.y + 1, current_tile->position.x * 2 + 1);
		printw("  ");
	}
	attroff(COLOR_PAIR(TileType::SnakeBody));

	// Draw apple
	move(apple.y + 1, apple.x * 2 + 1);
	attron(COLOR_PAIR(TileType::Apple));
	printw("  ");
	attroff(COLOR_PAIR(TileType::Apple));

	// Draw score
	move(BOARD_HEIGHT + 1, 1);
	printw("Score: %03d", score);

	refresh();
}

// Check if the snake's body collides with a given position
bool checkSnakeCollision(Position pos) {
	for (SnakeTile* current_tile = head; current_tile; current_tile = current_tile->next) {
		if (current_tile->position.x == pos.x && current_tile->position.y == pos.y)
			return true;
	}
	return false;
}

// Check if an apple is located at the given position
bool checkAppleCollision(Position position) {
	return (position.x == apple.x && position.y == apple.y);
}

// Place a new apple at a random location
void placeApple() {
	do {
		apple.x = rand() % BOARD_WIDTH;
		apple.y = rand() % BOARD_HEIGHT;
	} while (checkSnakeCollision(apple));
}

// Handle eating an apple: Grow snake and place new one
void eatApple() {
	// Increase score
	score++;

	// Grow snake
	SnakeTile* current_tile = head;
	while (current_tile->next)
		current_tile = current_tile->next;

	current_tile->next = new SnakeTile(current_tile->position.x, current_tile->position.y);

	// Place new apple
	placeApple();
}

// Advance the game state by one tick if enough time has passed (frame safe)
bool tick(int deltaFrame) {
	delta_tick += deltaFrame;

	if (delta_tick > UNIT_TICK) {
		delta_tick -= UNIT_TICK;

		Position next_position = { head->position.x + direction_x, head->position.y + direction_y };

		// Check collision with self or walls
		if (checkSnakeCollision(next_position) ||
			next_position.x < 0 || next_position.x >= BOARD_WIDTH ||
			next_position.y < 0 || next_position.y >= BOARD_HEIGHT) {
			return false;
		}

		Position old_position = head->position;
		head->position = next_position;

		for (SnakeTile* current_tile = head->next; current_tile; current_tile = current_tile->next) {
			next_position = old_position;
			old_position = current_tile->position;
			current_tile->position = next_position;
		}
	}

	return true;
}

// Main game loop
void game_loop() {
	using clock = std::chrono::system_clock;
	auto lastFrame = std::chrono::time_point_cast<std::chrono::milliseconds>(clock::now());

	while (true) {
		auto thisFrame = std::chrono::time_point_cast<std::chrono::milliseconds>(clock::now());
		int delta = std::chrono::duration_cast<std::chrono::milliseconds>(thisFrame - lastFrame).count();
		lastFrame = thisFrame;

		int key = getch();
		switch (key) {
			case 'a': case KEY_LEFT:	direction_x = -1; direction_y =  0; break;
			case 'd': case KEY_RIGHT:	direction_x =  1; direction_y =  0; break;
			case 'w': case KEY_UP:		direction_x =  0; direction_y = -1; break;
			case 's': case KEY_DOWN:	direction_x =  0; direction_y =  1; break;
			case 'q': return;
		}

		if (!tick(delta)) return;

		if (checkAppleCollision(head->position))
			eatApple();

		draw();
	}
}

int main() {
	srand(time(0));

	// Initialize ncurses
	initscr();
	cbreak();
	noecho();
	curs_set(0);
	timeout(0);
	keypad(stdscr, TRUE);

	// Setup terminal
	resizeterm(BOARD_HEIGHT + 2, BOARD_WIDTH * 2 + 2);
	box(stdscr, 0, 0);

	// Setup colors
	start_color();
	init_pair(TileType::Empty, COLOR_BLACK, COLOR_BLACK);
	init_pair(TileType::SnakeBody, COLOR_GREEN, COLOR_GREEN);
	init_pair(TileType::Apple, COLOR_RED, COLOR_RED);

	// Create initial snake
	head = new SnakeTile(BOARD_WIDTH / 2, BOARD_HEIGHT / 2);
	SnakeTile* current_tile = head;
	for (int i = 1; i <= 3; i++) {
		current_tile->next = new SnakeTile(head->position.x, head->position.y + i);
		current_tile = current_tile->next;
		score++;
	}

	placeApple();
	draw();
	game_loop();

	// Game over screen
	move(BOARD_HEIGHT / 2 + 1, BOARD_WIDTH - 5);
	printw("Game Over!");
	move(BOARD_HEIGHT / 2 + 2, BOARD_WIDTH - 12);
	printw("Your final score is: %03d", score);
	refresh();

	napms(500);
	timeout(-1);
	getch();
	endwin();

	return 0;
}
