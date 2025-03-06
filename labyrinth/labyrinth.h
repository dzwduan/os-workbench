#define MAX_ROWS 100
#define MAX_COLS 100
#define VERSION_INFO "Labyrinth Game"

typedef struct {
    char map[MAX_ROWS][MAX_COLS];
    int rows;
    int cols;
} Labyrinth;

typedef struct {
    int row;
    int col;
} Position;


bool isValidPlayer(char playerId);
bool loadMap(Labyrinth *labyrinth, const char *filename);
Position findPlayer(Labyrinth *labyrinth, char playerId);
Position findFirstEmptySpace(Labyrinth *labyrinth);
bool isEmptySpace(Labyrinth *labyrinth, int row, int col);
bool movePlayer(Labyrinth *labyrinth, char playerId, const char *direction);
bool saveMap(Labyrinth *labyrinth, const char *filename);
bool isConnected(Labyrinth *labyrinth);
