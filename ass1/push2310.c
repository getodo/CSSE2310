#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

typedef enum {
    OK = 0,
    INCORRECTARGS = 1,
    BADPLAYER = 2,
    BADFILE = 3,
    BADSAVE = 4,
    ENDOFFILE = 5,
    FULLBOARD = 6
} Status;

typedef struct Game {
    int height;
    int width;
    int turn;
    char p1;
    char p2;
    char players[2];
    char pType[2];
    char* file;
    char** board;
} Game;

//Exit the game and show the error message of the exit
Status exit_status(Status s) {
    const char* messages[] = {"",
	    "Usage: push2310 typeO typeX fname\n",
	    "Invalid player type\n",
	    "No file to load from\n",
	    "Invalid file contents\n",
	    "End of file\n",
	    "Full board in load\n"};
    fputs(messages[s], stderr);
    exit(s);
}

//Create the game board from values in read files. Save board to struct.
void create_board(Game* game) {
    game->board = malloc(sizeof(char*) * game->height);
    for(int i = 0; i < game->height; i++) {
	game->board[i] = malloc(sizeof(char*) * (game->width * 2));
    }
}

//Print the current board from the game struct
void print_board(Game* game) {
    for(int i = 0; i < game->height; i++) {
	for(int j = 0; j < game->width * 2; j++) {
	    printf("%c", game->board[i][j]);
	}
	printf("\n");
    }
}

//Read load file. Load files contents will be check for validity. If a 
//content are not valid game will exit with status else, the game values will
//be saved in the game struct
void read_file(Game* game) {
    char text[256];
    char lines[1024];
    FILE* fileRead = fopen(game->file, "r");
    if(fileRead == NULL) {
	exit_status(BADFILE);
    }
    fgets(text, 256, fileRead);
    sscanf(text, "%d %d\n", &(game->height), &(game->width));
    if((game->height < 3 || game->width < 3)) {
	exit_status(4);
    }
    fgets(text, 256, fileRead);
    sscanf(text, "%c\n", &(game->p1));
    if(!(game->p1 == 'X' || game->p1 == 'O')) { //check valid chars
	exit_status(4);
    }
    create_board(game); //create and save game board
    for(int i = 0; i < game->height; i++) {
	if(fgets(lines, 1024, fileRead) != NULL) {
	    for(int j = 0; j < game->width * 2; j++) { 
		game->board[i][j] = lines[j];
	    }
	}
    }
    fclose(fileRead);
}

//Save the game in its current state with given file name
void save_game(Game* game, char* fileName) {
    char name[strlen(fileName) - 1]; 
    for(int i = 0; i < strlen(fileName) - 1; i++) {
	name[i] = fileName[i];
    }
    name[strcspn(name, "\n")] = 0; //remove '\n' from array
    FILE* fileSave = fopen(name, "w");
    if(fileSave != NULL) {
	fprintf(fileSave, "%d %d\n", game->height, game->width);
	fprintf(fileSave, "%c\n", game->players[game->turn]);
	for(int i = 0; i < game->height; i++) {
	    for(int j = 0; j < game->width * 2; j++) {
		fprintf(fileSave, "%c", game->board[i][j]);
	    }
	    fprintf(fileSave, "\n");
	}
    }
    fclose(fileSave);
}

//Check if the board is full on load. If the board is full on load 
//game will exit with status.
void is_board_full(Game* game) {
    for(int i = 1; i < game->height - 1; i++) {
	for(int j = 3; j < (game->width * 2) - 2; j++) {
	    if(game->board[i][j] == '.') { 
		return;
	    }
	}
    }
    exit_status(FULLBOARD);
}
		
//Check to see if a piece placed on the top row will cause a push down
int can_push_down(Game* game, int row, int col) {
    for(int i = 0; i < game->height - 1; i++) {
    	if(game->board[row + 1 + i][col] == '.') {
            return 1;
        }
    }
    return 0;
} 

//Check to see if a piece placed on the bottom row will cause a push up
int can_push_up(Game* game, int row, int col) {
    for(int i = 0; i < game->height - 1; i++) {
    	if(game->board[row - 1 - i][col] == '.') {
            return 1;                                                 
    	}
    }
    return 0;
}

//Check to see if a piece placed in the left most column will cause a push 
//to the right
int can_push_right(Game* game, int row, int col) {
    for(int i = 0; i < game->width * 2 - 1; i++) {
	if(game->board[row][col + 2 + i] == '.') {
	    return 1;
	}
    }
    return 0;
}

//Check to see if a piece placed in the right most column will cause a push
//to the left
int can_push_left(Game* game, int row, int col) {
    for(int i = 0; i < game->width * 2 - 1; i++) {
	if(game->board[row][col - 2 - i] == '.') {
	    return 1;
	}
    }
    return 0;
}

//Check if a legal move has been made by the player
int can_place(Game* game, int row, int col) {
    //check if play out of array bounds
    if(row > game->height - 1 || col > game->width * 2 - 1 ||
	    row < 0 || col < 0) {
        return 0;
    }
    //check if push down legal
    if(row == 0 && game->board[row + 1][col] == '.') {
        return 0;
    }
    //check if up legal
    if(row == game->height - 1 && game->board[row - 1][col] == '.') { 
        return 0;
    }
    //check if push right legal
    if(col == 1 && game->board[row][col + 2] == '.') {
        return 0;
    }
    //check if push left legal
    if(col == game->width * 2 - 1 && game->board[row][col - 2] == '.') {
        return 0;
    }
    if(col == 1 && game->board[row][col + 2] == '.') {
    	return 0;
    }
    //check oif piece is not being placed in board corners
    if(game->board[row][col] == ' ') {
        return 0;
    }
    //check if piece jis placed on vacant spot
    if(game->board[row][col] != '.') {
        return 0;
    }
    return 1;
}

//Check all direction to see if a legal push can be made
int can_push(Game* game, int row, int col) {
    if(row == 0) {    
	if(!(can_push_down(game, row, col))) { //legal push down
	    return 0;
        }
    }
    if(row == game->height - 1) {
        if(!(can_push_up(game, row, col))) { //legal push up
            return 0;
        }
    }
    if(col == 1) {
	if(!(can_push_right(game, row, col))) { //legal push right
	    return 0;
	}
    }
    if(col == game->width * 2 - 1) {
	if(!(can_push_left(game, row, col))) { //legal push left
	    return 0;
	}
    }
    return 1;
}

//Check if move selected is legal
int legal_move(Game* game, int row, int col) {
    if(!(can_place(game, row, col))) {
	return 0;
    }
    if(!(can_push(game, row, col))) {
	return 0;
    }
    return 1;
}

//If a legal play was made on the top row push down all applicable 
//pieces in the selected column
void push_down(Game* game, int row, int col) {
    int i = 0;
    while(game->board[row + i + 1][col] != '.') {
	i++; //increments based on how many consecutive player chars
    }
    for(int j = i; j > 0; j--) { //push pieces 
	game->board[row + j + 1][col] = game->board[row + j][col];
    }
    game->board[row + 1][col] = game->players[game->turn];
}

//If a legal play was made on the bottom row push down all applicable  
//pieces in the selected column
void push_up(Game* game, int row, int col) {
    int i = 0;
    while(game->board[row - i - 1][col] != '.') {
	i++;
    }
    for(int j = i; j > 0; j--) {
	game->board[row - j - 1][col] = game->board[row - j][col];
    }
    game->board[row - 1][col] = game->players[game->turn];
}

//If a legal play was made in the left most column push to the right all
//applicable  pieces in the select row
void push_right(Game* game, int row, int col) {
    int i = 0;
    while(game->board[row][2 * i + 3] != '.') {
	i++;
    }
    for(int j = i; j > 0; j--) {
	game->board[row][2 * j + 3] = game->board[row][j * 2 + 1];
    }
    game->board[row][3] = game->players[game->turn];
}

//If a legal play was made in the right most column push to the left all
//applicable pieces in the selected row
void push_left(Game* game, int row, int col) {
    int i = 0;
    while(game->board[row][col * 2 - 1 - (2 * i)] != '.') {
	i++; 
    }
    for(int j = i; j > 0; j--) {
	game->board[row][col * 2 - 1 - (2 * j)] = 
		game->board[row][col * 2 + 1 - (2 * j)];
    }
    game->board[row][col * 2 - 1] = game->players[game->turn];
}

//If a legal move was made that could cause a push check to see if a push
//could be made if not player the piece normally
void check_push(Game* game, int row, int col) {
    if(row == 0) {
	push_down(game, row, col * 2 + 1); //top row
    } else if(row == game->height - 1) {
	push_up(game, row, col * 2 + 1); //bottom row
    } else if(col == 0) {
	push_right(game, row, col); //left col
    } else if(col == game->width - 1) {
	push_left(game, row, col); //right col
    } else { //no push found player move
	game->board[row][col * 2 + 1] = game->players[game->turn];
    }
}

//If player is human, wait for player input and check if a legal move was 
//selected. If a move made was illigal or a game was saved, repromt the player
//to make a move.
void human_move(Game* game) {
    char move[64];
    char temp[64];
    char* err1, *err2, *input1, *input2;
    int legal = 0;
    int row, col;
    while(legal == 0) {
	int invalid = 0;
        printf("%c:(R C)> ", game->players[game->turn]);
	if(!(fgets(move, 64, stdin))) {
	    exit_status(ENDOFFILE);
	}
	memcpy(temp, move, 64);
	input1 = strtok(temp, " "); //get the first player input
	for(int i = 0; i < strlen(input1); i++) {
	    if(input1[i] == '/') { //check for / in save file name
		invalid = 1;
	    }
	}
	if(invalid == 1) {
	    fprintf(stderr, "Save failed\n");
	    continue;
	}
	if(input1[0] == 's' && input1[1] != '\n') { //save must be 1 string
	    char* fileName = input1 + 1;
	    save_game(game, fileName);
	    continue;
	}
	if(input1 == NULL) { //player entered nothing (just '\n')
	    continue;
	}
	input2 = strtok(NULL, "\n");
	if(input2 == NULL) {
	    continue;
	}
	row = strtoul(input1, &err1, 10);
	col = strtoul(input2, &err2, 10);
	if(*err1 != '\0' || *err2 != '\0') {
	    continue;
	}
	if((!legal_move(game, row, col * 2 + 1))) {
	    continue;
	}
	legal = 1;
    }
    check_push(game, row, col); //check if player will cause push
}

//If automated player type 0 char is O, scan the board from the top left to
//find a vacant spot to play piece
void type0_omove(Game* game) {
    for(int i = 1; i < game->height - 1; i++) {
	for(int j = 3; j < game->width * 2 - 1; j += 2) {
	    if(game->board[i][j] == '.') {
		game->board[i][j] = 'O';
		printf("Player O placed at %d %d\n", i, j / 2);
		return;
	    }
	}
    }
}

//If automated player type 0 char is X, scan the board from the bottom right 
//to find a vacant spot to play piece
void type0_xmove(Game* game) {
    for(int i = game->height - 2; i > 0; i--) {
	for(int j = game->width * 2 - 3; j > 1; j -= 2) {
	    if(game->board[i][j] == '.') {
		game->board[i][j] = 'X';
		printf("Player X placed at %d %d\n", i, j / 2);
		return;
	    }
	}
    }
}

//If automated player is type 0, check which player char they are and make
//a move accordingly
void type0_move(Game* game) {
    if(game->players[game->turn] == 'O') {
	type0_omove(game);
    }
    if(game->players[game->turn] == 'X') {
	type0_xmove(game);
    }
}

//Check if the type 1 automated player move will lower the opposing players 
//score if pushing down
int lower_down(Game* game, int col) {
    int newScore = 0;
    int currentScore = 0;
    char p2 = game->players[(game->turn + 1) % 2];
    for(int i = 1; i < game->height; i++) {
	if(game->board[i][col] == '.') {
	    break;
	}
	if(game->board[i][col] == p2 && ((i + 1) < game->height)) {
	    currentScore = currentScore + game->board[i][col - 1];
	    newScore = newScore + game->board[i + 1][col - 1];
	}
    }
    if(newScore < currentScore) {
	return 1;
    } else {
	return 0;
    }
}

//Check if the type 1 automated player move will lower the opposing players
//score if pushing to the left
int lower_left(Game* game, int row) {
    int newScore = 0;
    int currentScore = 0;
    char p2 = game->players[(game->turn + 1) % 2];
    for(int i = game->width * 2 - 3; i > 1; i -= 2) {
	if(game->board[row][i] == '.') {
	    break;      
	}
	if(game->board[row][i] == p2 && ((i - 3) >= 0)) {
	    currentScore = currentScore + game->board[row][i - 1];
	    newScore = newScore + game->board[row][i - 3];
	}
    }
    if(newScore < currentScore) {
	return 1;
    } else {
	return 0;
    }
}

//Check if the type 1 automated player move will lower the opposing players
//score if pushing up
int lower_up(Game* game, int col) {
    int newScore = 0;
    int currentScore = 0;
    char p2 = game->players[(game->turn + 1) % 2];
    for(int i = game->height - 2; i > 0; i--) {
	if(game->board[i][col] == '.') {
	    break;
	}
	if(game->board[i][col] == p2) {
	    currentScore = currentScore + game->board[i][col - 1];
	    newScore = newScore + game->board[i - 1][col - 1];
	}
    }
    if(newScore < currentScore) {
	return 1;
    } else {
	return 0;
    }
}

//Check if the type 1 automated player move will lower the opposing players
//score if pushing to the right
int lower_right(Game* game, int row) {
    int newScore = 0;
    int currentScore = 0;
    char p2 = game->players[(game->turn + 1) % 2];
    for(int i = 3; i < game->width * 2; i += 2) {
	if(game->board[row][i] == '.') {
	    break;
	}
	if(game->board[row][i] == p2) {
	    currentScore = currentScore + game->board[row][i - 1];
	    newScore = newScore + game->board[row][i + 1];
	}
    }
    if(newScore < currentScore) {
	return 1;
    } else {
	return 0;
    }
}

//If a move cannot be found that will cause a push resulting in the opposing
//players score being reduced. Make a move in the highest value cell
void type1_first_move(Game* game) {
    int row, col;
    int highNumber = 0;
    for(int i = 1; i < game->height - 1; i++) {                       
	for(int j = 3; j < game->width * 2 - 1; j += 2) {
	    if(game->board[i][j - 1] > highNumber && 
		    game->board[i][j] == '.') { //compare cell to current high
		row = i;
		col = j;
		highNumber = game->board[i][j - 1];
	    }
	}
    }
    printf("Player %c placed at %d %d\n", game->players[game->turn], 
	    row, col / 2);
    game->board[row][col] = game->players[game->turn];
}

//If automated play is type 1, scan the board to check for the most 
//appropriate move based off the required movement pattern of the player
void type1_move(Game* game) {
    char** board = game->board; //temp struct variables to reduce clutter
    int height = game->height; 
    int width = game->width * 2;
    for(int i = 3; i < width - 2; i += 2) { //check top row
	if(board[1][i] != '.' && board[height - 1][i] == '.' && 
		board[0][i] == '.') { 
	    if(lower_down(game, i)) { //check if a piece can be pushed
		check_push(game, 0, i / 2);
		printf("Player %c placed at 0 %d\n", 
			game->players[game->turn], i / 2);
		return;
	    }
	}
    }
    for(int i = 1; i < height - 1; i++) { //right column
	if(board[i][width - 3] != '.' && board[i][1] == '.' && 
		board[i][width - 1] == '.') {
	    if(lower_left(game, i) && board[i][1] == '.') {
		check_push(game, i, game->width - 1);
		printf("Player %c placed at %d %d\n", 
			game->players[game->turn], i, width / 2 - 1);
		return;
	    }
	}
    }
    for(int i = width - 3; i > 0; i -= 2) { //check bottom row
	if(board[height - 2][i] != '.' && board[0][i] == '.' && 
		board[height - 1][i] == '.') {
	    if(lower_up(game, i)) {
		check_push(game, height - 1, i / 2);
		printf("Player %c placed at %d %d\n",
			game->players[game->turn], height - 1, i / 2);
		return;
	    }
	}
    }
    for(int i = height - 2; i > 0; i--) { //check left column
	if(board[i][3] != '.' && board[i][width - 1] == '.' && 
		board[i][1] == '.') {
	    if(lower_right(game, i)) {
		check_push(game, i, 0);
		printf("Player %c placed at %d 0\n",
			game->players[game->turn], i);
		return;
	    }
	}
    }
    type1_first_move(game);
}

//Check if the interior of the board is full will return 1 if the board 
//is not full else 0
int end_game(Game* game) {
    int endGame = 0;
    for(int i = 1; i < game->height - 1; i++) {
	for(int j = 2; j < game->width * 2 - 2; j++) {
	    if(game->board[i][j] == '.') {
		endGame = 1;
	    }
	}
    }
    return endGame;
}			

//When the game ends calculate the score and the winner of the game
void do_score(Game* game) {
    int xScore = 0;
    int oScore = 0;
    for(int i = 0; i < game->height; i++) {
	for(int j = 0; j < game->width * 2; j++) {
	    if(game->board[i][j] == 'X') {
		xScore = game->board[i][j - 1] - '0' + xScore;
	    }
	    if(game->board[i][j] == 'O') {
		oScore = game->board[i][j - 1] - '0' + oScore;
	    }
	}
    }
    if(xScore > oScore) {
	printf("Winners: X\n");
    } else if(oScore > xScore) {
	printf("Winners: O\n");
    } else {
	printf("Winners: O X\n");
    }
}

//Initialise the game assets
void init_game_assets(Game* game, char** argv) {
    game->file = argv[3];
    read_file(game);
    is_board_full(game);
    game->p2 = (game->p1 == 'X') ? 'O' : 'X';
    game->players[0] = 'O';
    game->players[1] = 'X';
    if(game->p1 == 'X') {
	game->turn = 1;
    } else {
	game->turn = 0;
    }
}

//Check the validity of user input arguement. If an incorrect arguement 
//is found the game will exit, else return 1
int check_args(Game* game, int argc, char** argv) {
    if(argc != 4) {
	exit_status(INCORRECTARGS);
    }
    for(int i = 0; i < 2; i++) {
	if(!strcmp(argv[1 + i], "H")) {
	    game->pType[i] = 'H';
	} else if(!strcmp(argv[1 + i], "0")) {
	    game->pType[i] = '0';
	} else if(!strcmp(argv[1 + i], "1")) {
	    game->pType[i] = '1';
	} else {
	    exit_status(BADPLAYER);
	}
    }
    return 1;
}

//Update the game turn after a player make a turn
void update_turn(Game* game) {
    game->turn = (game->turn + 1) % 2; //Turn will be 0 or 1
}

//Start the game. Depending on the type of player will change the type of 
//move made
void start_game(Game* game) {
    switch(game->pType[game->turn]) {
	case 'H':
	    human_move(game);
	    break;
	case '0':
	    type0_move(game);
	    break;
	case '1':
	    type1_move(game);
	    break;
    }
    print_board(game);
    update_turn(game);
}

int main(int argc, char** argv) {
    Game* game = malloc(sizeof(Game));
    check_args(game, argc, argv);
    init_game_assets(game, argv);
    print_board(game);
    while(1) {
	start_game(game);
	if(!(end_game(game))) {
	    do_score(game);
	    exit(0);
	}
    }
}
