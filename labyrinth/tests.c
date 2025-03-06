#include <string.h>
#include "testkit.h"
#include "labyrinth.h"

SystemTest(test_version, ((const char *[]){ "--version" })) {
    tk_assert(result->exit_status == 0, "Must exit 0");
    tk_assert(
        strstr(result->output, "Labyrinth Game") != NULL,
        "Must have correct message"
    );
}

SystemTest(test_version_fail, ((const char *[]){ "--version", "??" })) {
    tk_assert(result->exit_status == 1, "Must exit 1");
}

SystemTest(invalid_args_1, ((const char *[]){ "--nonexist", "--another" })) {
    tk_assert(result->exit_status == 1, "Must exit 1");
}

SystemTest(invalid_args_2, ((const char *[]){ "hello os world" })) {
    tk_assert(result->exit_status == 1, "Must exit 1");
}


// Test initialization function
static void setup_test_map() {
    FILE *f = fopen("test.map", "w");
    tk_assert(f != NULL, "Should be able to create test map file");
    fprintf(f, "....\n");
    fprintf(f, "....\n");
    fprintf(f, "....\n");
    fclose(f);
}

static void cleanup_test_map() {
    remove("test.map");
}

// Test basic map loading and player movement
SystemTest(test_basic_move,
    ((const char *[]){ "--map", "test.map", "--player", "1", "--move", "right" }),
    .init = setup_test_map,
    .fini = cleanup_test_map) {
    tk_assert(result->exit_status == 0, "Must exit 0");
}

// Test invalid move direction
SystemTest(test_invalid_move,
    ((const char *[]){ "--map", "test.map", "--player", "1", "--move", "invalid" })) {
    tk_assert(result->exit_status == 1, "Invalid move direction should return error");
}

// Test invalid player ID
SystemTest(test_invalid_player,
    ((const char *[]){ "--map", "test.map", "--player", "X" })) {
    tk_assert(result->exit_status == 1, "Invalid player ID should return error");
}


UnitTest(example_test) {
    tk_assert(1 == 1, "This should never fail.");
}


UnitTest(test_valid_player_id) {
    tk_assert(isValidPlayer('0') == true, "0 should be a valid player ID");
    tk_assert(isValidPlayer('9') == true, "9 should be a valid player ID");
    tk_assert(isValidPlayer('5') == true, "5 should be a valid player ID");
    tk_assert(isValidPlayer('a') == false, "Letters should not be valid player IDs");
    tk_assert(isValidPlayer('#') == false, "Special characters should not be valid player IDs");
}

UnitTest(test_empty_space) {
    Labyrinth lab = {
        .rows = 3,
        .cols = 3,
        .map = {
            "..#",
            "#..",
            "..."
        }
    };

    tk_assert(isEmptySpace(&lab, 0, 0) == true, "Should be an empty space");
    tk_assert(isEmptySpace(&lab, 0, 2) == false, "Wall position should not be empty");
    tk_assert(isEmptySpace(&lab, -1, 0) == false, "Outside boundary should not be empty");
    tk_assert(isEmptySpace(&lab, 3, 0) == false, "Outside boundary should not be empty");
}

// Test maze connectivity check
UnitTest(test_maze_connectivity) {
    // Test connected maze
    Labyrinth connected = {
        .rows = 3,
        .cols = 3,
        .map = {
            "...",
            ".#.",
            "..."
        }
    };
    tk_assert(isConnected(&connected) == true, "Connected maze should return true");

    // Test disconnected maze
    Labyrinth disconnected = {
        .rows = 3,
        .cols = 3,
        .map = {
            "..#",
            "###",
            "#.."
        }
    };
    tk_assert(isConnected(&disconnected) == false, "Disconnected maze should return false");
}


UnitTest(test_find_player) {
    Labyrinth lab = {
        .rows = 3,
        .cols = 3,
        .map = {
            "..1",
            "...",
            "..."
        }
    };

    Position pos = findPlayer(&lab, '1');
    tk_assert(pos.row == 0 && pos.col == 2, "Should find correct player position");

    pos = findPlayer(&lab, '2');
    tk_assert(pos.row == -1 && pos.col == -1, "Should return (-1,-1) for player not found");
}


UnitTest(test_find_first_empty) {
    Labyrinth lab = {
        .rows = 2,
        .cols = 2,
        .map = {
            "#.",
            "##"
        }
    };

    Position pos = findFirstEmptySpace(&lab);
    tk_assert(pos.row == 0 && pos.col == 1, "Should find first empty position");
}
