#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef enum {
    NORMAL = 0,
    INVALID_ARGS = 1,
    INVALID_DECK = 2,
    INVALID_PATH = 3,
    BAD_PLAYER = 4,
    COMM_ERROR = 5
} Status;

//exits the program with specified exit and message
Status exit_status(Status s) {
    const char* messages[] = {"",
	    "Usage: 2310dealer deck path p1 {p2}\n",
	    "Error reading deck\n",
	    "Error reading path\n",
	    "Error starting process\n",
	    "Communications error\n"};
    fputs(messages[s], stderr);
    exit(s);
}

typedef struct Player {
    FILE* pc;
    FILE* cp;
    pid_t pid;
    int ptoc[2];
    int ctop[2];
    char* name;
    char id;
    char lastItem;
    int money;
    int points;
    int v1;
    int v2;
    int a;
    int b;
    int c;
    int d;
    int e;
} Player;

typedef struct Game {
    char* itemFile;
    char* pathFile;
    char* itemDeck;
    char* pathDeck;
    char* rawPathDeck;
    char** board;
    int items;
    int paths;
    int players;
    int pathsDigits;
    int currentItem;
    int* scoreList;
    Player** playerList;
} Game;

//Prototype functions
void init_player_process(Game* game, char** argv);
void print_board(Game* game);
void send_your_turn(Game* game, int id);
void check_item_length(Game* game, char text[]);

//Create the game board. The board will be created sing the path
//file. The population or '-' char of each site or barrier will be
//replaced with ' '. Players will start on the board represented by their
//id's ion decending order.
void create_board(Game* game) {
    game->board = malloc(sizeof(char*) * game->players + 1);
    int k = 1; //counter for ' ' replacement on every 3rd char
    for(int i = 0; i < game->players + 1; i++) {
	game->board[i] = malloc(sizeof(char) * (game->paths * 3));
    }
    for(int i = 0; i < game->players + 1; i++) {
	for(int j = 0; j < game->paths * 3; j++) {
	    if(i != 0 && j == 0) {  //player id to board
		game->board[i][j] = game->playerList[game->players - i]->id;
	    } else if(i != 0 && j != 0) {
		game->board[i][j] = ' ';    //' ' for rest of board
	    } else if(i == 0 && k % 3 != 0) {
		game->board[i][j] = game->pathDeck[j];	//sites 
	    } else if(i == 0 && k % 3 == 0) {
		game->board[i][j] = ' ';    //' ' for site populations
	    }
	    k++;
	}
    }
}

//Count how many rows contain a char. Return the count.
int char_on_row(Game* game) {
    int k = 0;
    for(int i = 0; i < game->players + 1; i++) {
	for(int j = 0; j < game->paths * 3; j++) {
	    if(game->board[i][j] != ' ') {
		k = i;
	    }
	}
    }
    return k + 1;
}

//Prints the game board will only prints rows that counts sites and player ID
void print_board(Game* game) {
    int k = char_on_row(game);
    for(int i = 0; i < k; i++) {
	for(int j = 0; j < game->paths * 3; j++) {
	    printf("%c", game->board[i][j]);
	}
	printf("\n");
    }
}

//Prints the path deck.
void print_path_deck(Game* game) {
    for(int i = 0; i < game->paths * 3; i++) {
	printf("%c", game->pathDeck[i]);
    }
    printf("\n");
}

//Allocate the path deck
void create_path_deck(Game* game) {
    game->pathDeck = malloc(sizeof(char) * game->paths * 3);
}

//Allocate the raw path deck to send to players
void create_raw_path_deck(Game* game) {
    game->rawPathDeck = malloc(sizeof(char) * 
	    (game->paths * 3) + game->pathsDigits);
}

//Initialise all the player variables
void init_players(Game* game, char** argv, int argc) {
    game->playerList = malloc(sizeof(struct Player*) * game->players);
    for(int i = 0; i < game->players; i++) {
	game->playerList[i] = malloc(sizeof(Player)); //setup player list
	game->playerList[i]->name = argv[i + 3];
	game->playerList[i]->id = i + '0';  //convert to char
	game->playerList[i]->money = 7;
	game->playerList[i]->points = 0;
	game->playerList[i]->v1 = 0;
	game->playerList[i]->v2 = 0;
	game->playerList[i]->a = 0;
	game->playerList[i]->b = 0;
	game->playerList[i]->c = 0;
	game->playerList[i]->d = 0;
	game->playerList[i]->e = 0;
	pipe(game->playerList[i]->ptoc);
	pipe(game->playerList[i]->ctop);
    }
}

//Forks the player processes and initalise them using execvp
void init_player_process(Game* game, char** argv) {
    int file;
    for(int i = 0; i < game->players; i++) {
	game->playerList[i]->pid = fork();
	if(game->playerList[i]->pid < 0) {
	    exit_status(BAD_PLAYER);
	}
	if(game->playerList[i]->pid == 0) {	//We are in child
	    close(game->playerList[i]->ptoc[1]);
	    close(game->playerList[i]->ctop[0]);
	    file = open("/dev/null", O_WRONLY); //remove for testing
            dup2(game->playerList[i]->ptoc[0], STDIN_FILENO);
            dup2(game->playerList[i]->ctop[1], STDOUT_FILENO);
	    dup2(file, STDERR_FILENO);		//remove for testing
	    char* players = malloc(sizeof(char) * 2);
	    sprintf(players, "%d", game->players);
	    char* id = malloc(sizeof(char) * 2);
	    sprintf(id, "%c", game->playerList[i]->id);
	    char* args[] = {game->playerList[i]->name, players, id, NULL};
            execvp(args[0], args);
            exit(BAD_PLAYER);
	} else { //we are the parent
	    close(game->playerList[i]->ptoc[0]); //close other end of pipe
            close(game->playerList[i]->ctop[1]); //close other end of pipe
	    game->playerList[i]->pc = 
		    fdopen(game->playerList[i]->ptoc[1], "w");
            game->playerList[i]->cp = 
		    fdopen(game->playerList[i]->ctop[0], "r");
	}
    }
}

//Check validity of the item file.
void check_items(Game* game) {
    int items = game->items;
    for(int i = 0; i < items; i++) {
	char item = game->itemDeck[i];
	if(!(item == 'A' || item == 'B' || item == 'C' ||
		item == 'D' || item == 'E')) {
	    exit_status(INVALID_DECK);
	}
    }
    return;
}

//read the item file to be stored into the game struct
//a valid item file with have at least 4 items
void read_item_file(Game* game) {
    char itemText[256];
    int count = 0;
    FILE* f = fopen(game->itemFile, "r");
    if(f == NULL) {
	exit_status(INVALID_DECK);
    }
    fgets(itemText, 256, f);
    check_item_length(game, itemText);
    int tempItem = game->items;
    game->itemDeck = malloc(sizeof(char) * game->items);
    do {
	count++;
	tempItem /= 10;
    } while(tempItem != 0);
    for(int i = 0; i < game->items + 1; i++) {
	game->itemDeck[i] = itemText[i + count];
    }
    check_items(game);
}

//Check the length of the path file
void check_path_length(Game* game, char text[]) {
    int paths = 0;
    char* err1;
    for(int i = 0; i < 256; i++) {
	if(text[i] == ';' && paths == 0) {
	    paths = i;
	}
    }
    game->pathsDigits = paths;
    char pathsCopy[game->pathsDigits];
    strncpy(pathsCopy, text, game->pathsDigits);
    pathsCopy[game->pathsDigits] = '\0';
    game->paths = strtoul(pathsCopy, &err1, 10);
    if(game->paths < 2) {
	exit_status(INVALID_PATH);
    }
    game->pathsDigits = game->pathsDigits + 1;
    if(*err1 != '\0') {
	exit(1);
    }
}

//Check the validity of the sites from the path file
void check_site(char* site) {
    if(site[2] != '-') {
	if(!isdigit(site[2])) {
	    exit_status(INVALID_PATH);
	}
    }
    if(site[0] == 'M' && site[1] == 'o') {
	return;
    } else if(site[0] == 'V' && (site[1] == '1' || site[1] == '2')) {
	return;
    } else if(site[0] == 'D' && site[1] == 'o') {
	return;
    } else if(site[0] == 'R' && site[1] == 'i') {
	return;
    } else if(site[0] == ':' && site[1] == ':' && site[2] && '-') {
	return;
    } else {
	exit_status(INVALID_PATH);
    }
}

//Check the length of the item list and store it
void check_item_length(Game* game, char text[]) {
    int items = 0;
    for(int i = 0; i < 256; i++) {
	if(isdigit(text[i])) {
	    items = i;
	}
    }
    char* err1;
    char itemCopy[items];
    strncpy(itemCopy, text, items);
    itemCopy[items] = '\0';
    game->items = strtoul(itemCopy, &err1, 10);
}

//Check the validity of the path file
void check_path_deck(Game* game) {
    int paths = game->paths * 3;
    char* site = malloc(sizeof(char) * 3);
    char si;
    char te;
    char siteLimit;
    for(int i = 0; i < paths; i++) {
	if((i == 0 || i == 1 || i == (paths - 2) || i == (paths - 3)) &&
		game->pathDeck[i] != ':') {
	    exit_status(INVALID_PATH);
	}
	if(i % 3 == 0) {
	    si = game->pathDeck[i];
	    te = game->pathDeck[i + 1];
	    siteLimit = game->pathDeck[i + 2];
	    sprintf(site, "%c%c%c", si, te, siteLimit);
	    check_site(site);
	}
    }
}

//read the path file and store contents in the game struct
//a valid path file will have at least 4 paths to move
void read_path_file(Game* game) {
    char text[1024];
    FILE* f = fopen(game->pathFile, "r");
    if(f == NULL) {
	exit_status(INVALID_PATH);
    }
    fgets(text, 1024, f);
    check_path_length(game, text);
    create_path_deck(game);
    create_raw_path_deck(game);
    for(int i = 0; i < game->pathsDigits + (game->paths * 3); i++) {
	game->rawPathDeck[i] = text[i];
    }
    for(int i = 0; i < game->paths * 3; i++) {
	game->pathDeck[i] = text[i + game->pathsDigits];
    }
    check_path_deck(game);
}

//if all players are at the final barrier end the game
int end_game(Game* game) {
    int endGame = 1;
    int j = ((game->paths * 3) - 3); //final columns players can be in
    for(int i = 1; i < game->players + 1; i++) {
	if(game->board[i][j] == ' ') {
	    endGame = 0;
	}
    }
    return endGame;
}

//Send the raw path deck to the players.
void send_path_deck(Game* game) {
    for(int i = 0; i < game->players; i++) {
	fprintf(game->playerList[i]->pc, "%s\n", game->rawPathDeck);
	fflush(game->playerList[i]->pc);
    }
}

//Find which player will make the next turn. The next turn
//is based on which player is the most far back on the board
//or the lowest in the farest back column
int this_players_turn(Game* game) {
    char playerId = '.';
    for(int j = 0; j < game->paths * 3; j++) {
	for(int i = game->players; i > 0; i--) {
	    if(game->board[i][j] != ' ' && playerId != ' ') {
		playerId = game->board[i][j];
		return playerId - '0';
	    }
	}
    }
    return playerId;
}

//Send YT message to player
void send_your_turn(Game* game, int id) {
    fprintf(game->playerList[id]->pc, "YT\n");
    fflush(game->playerList[id]->pc);
}

//Send HAP message to players. Player will update their variables
//based on this message.
void send_hap(Game* game, int p, int n) {
    int s = 0;
    int m = 0;
    char c = '0';
    if(game->board[0][n * 3] == 'M') {
	m = 3;
    } else if(game->board[0][n * 3] == 'R') {
	c = game->playerList[p]->lastItem;
    } else if(game->board[0][n * 3] == 'D') {
	s = (game->playerList[p]->money / 2);
	m = (-1 * game->playerList[p]->money);
    }
    for(int i = 0; i < game->players; i++) {
	fprintf(game->playerList[i]->pc, "HAP%d,%d,%d,%d,%c\n", p, n, s, m, c);
	fflush(game->playerList[i]->pc);
    }
    if(game->board[0][n * 3] == 'D') {
	game->playerList[p]->points = 
		game->playerList[p]->points + game->playerList[p]->money / 2;
	game->playerList[p]->money = 0;
    }
}

//Move the player on the hubs board.
void move_player(Game* game, int site, char id) {
    for(int i = 1; i < game->players + 1; i++) {
	for(int j = 0; j < game->paths * 3; j++) {
	    if(game->board[i][j] == id) {
		game->board[i][j] = ' ';
	    }
	}
    }
    for(int i = 1; i < game->players + 1; i++) {
	if(game->board[i][site] == ' ') {
	    game->board[i][site] = id;
	    return;
	}
    }
}

//Move to the next item.
void update_current_item(Game* game) {
    game->currentItem = game->currentItem + 1;
    if(game->currentItem == game->items) {
    	game->currentItem = 0;
    }
}

//Update the player stats after the move has been sent 
//from the player to the hub.
void update_player_stats(Game* game, int i, int j) {
    char siteType = game->board[0][j];
    if(siteType == 'M') {
	game->playerList[i]->money = game->playerList[i]->money + 3;
    } else if(siteType == 'V') {
	if(game->board[0][j + 1] == '1') {
	    game->playerList[i]->v1 = game->playerList[i]->v1 + 1;
	} else if(game->board[0][j + 1] == '2') {
	    game->playerList[i]->v2 = game->playerList[i]->v2 + 1;
	}
    } else if(siteType == 'R') {
	char item = game->itemDeck[game->currentItem];
	switch(item) {
	    case 'A':
		game->playerList[i]->a = game->playerList[i]->a + 1;
		game->playerList[i]->lastItem = '1';
		break;
			
	    case 'B':
		game->playerList[i]->b = game->playerList[i]->b + 1;
		game->playerList[i]->lastItem = '2';
		break;

	    case 'C':
	        game->playerList[i]->c = game->playerList[i]->c + 1;
		game->playerList[i]->lastItem = '3';
		break;

	    case 'D':
		game->playerList[i]->d = game->playerList[i]->d + 1;
		game->playerList[i]->lastItem = '4';
		break;

	    case 'E':
		game->playerList[i]->e = game->playerList[i]->e + 1;
		game->playerList[i]->lastItem = '5';
		break;
	}
	update_current_item(game);
    }
}

//Print the player update to the stdout. This will be the current 
//player variables.
void print_player_update(Game* game, int i) {	
    char id = game->playerList[i]->id;
    int money = game->playerList[i]->money;
    int v1 = game->playerList[i]->v1;
    int v2 = game->playerList[i]->v2;
    int points = game->playerList[i]->points;
    int a = game->playerList[i]->a;
    int b = game->playerList[i]->b;
    int c = game->playerList[i]->c;
    int d = game->playerList[i]->d;
    int e = game->playerList[i]->e;
    fprintf(stdout, "Player %c Money=%d V1=%d V2=%d Points=%d ", 
	    id, money, v1, v2, points);
    fprintf(stdout, "A=%d B=%d C=%d D=%d E=%d\n", a, b, c, d, e);
}

//Calculate the players scores based off the stored score variables.
int score_items(Game* game, int i) {
    int points = 0;
    int scores[5];
    scores[0] = game->playerList[i]->a;
    scores[1] = game->playerList[i]->b;
    scores[2] = game->playerList[i]->c;
    scores[3] = game->playerList[i]->d;
    scores[4] = game->playerList[i]->e;
    while(1) {
	int count = 0;
	for(int i = 0; i < 5; i++) {
	    if(scores[i] > 0) {
		count++;
	    }
	}
	if(count == 5) {
	    points = points + 10;
	} else if(count == 4) {
	    points = points + 7;
	} else if(count == 3) {
	    points = points + 5;
	} else if(count == 2) {
	    points = points + 3;
	} else if(count == 1) {
	    points = points + 1;
	} else {
	    return points;
	}
	for(int i = 0; i < 5; i++) {
	    if(scores[i] != 0) {
		scores[i] = scores[i] - 1;
	    }
	}
    }
}

//Acknowledge the player. 
void acknowledge_player(Game* game) {
    for(int i = 0; i < game->players; i++) {
	char ack = fgetc(game->playerList[i]->cp);
	if(ack != '^') {
	    exit_status(BAD_PLAYER);
	}
	fputc('\n', game->playerList[i]->cp);
	fflush(game->playerList[i]->cp);
    }
}

//Calculate the score of the players
void do_score(Game* game) {
    for(int i = 0; i < game->players; i++) {
	int points = game->playerList[i]->v1 + game->playerList[i]->v2;
	int itemPoints = score_items(game, i);
	int moneyPoints = game->playerList[i]->points;
	int total = points + itemPoints + moneyPoints;
	game->scoreList[i] = total;
    }
    fprintf(stdout, "Scores: ");
    fprintf(stdout, "%d", game->scoreList[0]);
    for(int i = 1; i < game->players; i++) {
	fprintf(stdout, ",%d", game->scoreList[i]);
    }
    fprintf(stdout, "\n");
}

//Send done message to players 
void send_done_message(Game* game) {
    for(int i = 0; i < game->players; i++) {
	fprintf(game->playerList[i]->pc, "DONE");
	fflush(game->playerList[i]->pc);
    }
}

//Start the game reading messages from players
void game_loop(Game* game) {
    print_board(game);
    char rxMsg[16];
    char* rxScan = &rxMsg[0];
    int site;
    send_path_deck(game);
    while(1) {
	send_your_turn(game, this_players_turn(game));
	int playerId = this_players_turn(game);
	if(!fgets(rxScan, 16, game->playerList[playerId]->cp)) {
	    exit(1);
	}
	if(!strncmp("DO", rxScan, 2)) {
	    sscanf(rxMsg, "DO%d", &site);
	    move_player(game, site * 3, game->playerList[playerId]->id);
	    update_player_stats(game, playerId, site * 3);
	    send_hap(game, playerId, site);
	    print_player_update(game, playerId);
	    print_board(game);
	}
	if(end_game(game)) {
	    do_score(game);
	    send_done_message(game);
	    exit_status(NORMAL);
	}
    }
}

//read both files
void read_files(Game* game) {
    read_item_file(game);
    read_path_file(game);
}

//load initial game data
void validate_arguements(Game* game, int argc, char** argv) {
    if(argc < 4) {
	exit_status(INVALID_ARGS);
    }
    game->itemFile = argv[1];
    game->pathFile = argv[2];
    game->players = argc - 3;
    game->scoreList = malloc(sizeof(int) * game->players);
    game->currentItem = 0;
    read_files(game);
    init_players(game, argv, argc);
    create_board(game);
}

int main(int argc, char** argv) {
    Game* game = malloc(sizeof(Game));
    validate_arguements(game, argc, argv);
    init_player_process(game, argv);
    acknowledge_player(game);
    game_loop(game);
    return 1;
}
