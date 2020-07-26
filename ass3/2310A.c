#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

typedef struct Player {
    char* name;
    char* pathDeck;
    char** board;
    int paths;
    int ptotal;
    int id;
    int currentRow;
    int currentCol;
    int pathsDigits;
    int* playerScore;
    int* playerPoints;
    int* sitesVisited;
    int* playerIDList;
    int* playerMoney;
    int** playerItems;
    int** vScores;
} Player;

typedef enum {
    NORMAL = 0,
    INVALID_ARGS = 1,
    INVALID_PCOUNT = 2,
    INVALID_ID = 3,
    INVALID_PATH = 4,
    EOG = 5,
    COMM_ERROR = 6
} Status;

//Exit status method. Will print error to stderr and exit.
Status exit_status(Status s) {
    const char* message[] = {"",
	    "Usage: player pcount ID",
	    "Invalid player count",
	    "Invalid ID",
	    "Invalid path",
	    "Early game over",
	    "Communications error"};
    fprintf(stderr, "%s", message[s]);
    fprintf(stderr, "\n");
    exit(s);
}

//Prototype function
void create_board(Player* player);

//Count how many rows contain a char. Return the count.
int char_on_row(Player* player) {
    int k = 0;
    for(int i = 0; i < player->ptotal + 1; i++) {
	for(int j = 0; j < player->paths * 3; j++) {
	    if(player->board[i][j] != ' ') {
		k = i;
	    }
	}
    }
    return k + 1;
}

//Print the board. The board will only print rows that have
//sites ore players id's store on them. char_on_row() will
//provide the count for how many rows need to be print.
void print_board(Player* player) {
    int c = char_on_row(player);
    int k = 1;
    for(int i = 0; i < c; i++) {
	for(int j = 0; j < player->paths * 3; j++) {
	    if(i == 0 && k % 3 == 0) {
		fprintf(stderr, " ");
	    } else {
		fprintf(stderr, "%c", player->board[i][j]);
	    }
	    k++;
	}
	fprintf(stderr, "\n");
    }
}

//Check the validity of the sites. If a sites is invalid
//exit the game.
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
    } else if(site[0] == ':' && site[1] == ':' && site[2] == '-') {
	return;
    } else {
	exit_status(INVALID_PATH);
    }
}

//Check the path deck to ensure content are valid.
void check_path_deck(Player* player) {
    int paths = player->paths * 3;
    char* site = malloc(sizeof(char) * 3);
    char si, te, siteLimit;
    int digits = player->pathsDigits;
    for(int i = 0; i < paths; i++) {
	if((i == 0 || i == 1 || i == (paths - 2) || i == (paths - 3)) &&
		player->pathDeck[i] != ':') {
	    exit_status(INVALID_PATH);
	}
	if(i % 3 == 0 && i > digits + 3) {
	    si = player->pathDeck[i];
	    te = player->pathDeck[i + 1];
	    siteLimit = player->pathDeck[i + 2];
	    sprintf(site, "%c%c%c", si, te, siteLimit);
	    check_site(site);
	}
    }
}

//Read the path deck and store it to the player struct. Deck variables
//will be checked for validity.
void read_path_deck(Player* player) {
    int paths = 0;
    char* err;
    char buffer[1024];
    if(!fgets(buffer, 1024, stdin)) {
	exit_status(INVALID_PATH);
    }
    for(int i = 0; i < 1024; i++) {
	if(buffer[i] == ';' && paths == 0) {
	    paths = i;
	}
    }
    player->pathsDigits = paths;
    char pathsCopy[player->pathsDigits];
    strncpy(pathsCopy, buffer, player->pathsDigits);
    pathsCopy[player->pathsDigits] = '\0';
    player->paths = strtoul(pathsCopy, &err, 10);
    if(player->paths < 2) {
	exit_status(INVALID_PATH);
    }
    player->pathsDigits = player->pathsDigits + 1;
    player->pathDeck = malloc(sizeof(char) * 
	    (player->paths * 3) + player->pathsDigits);
    for(int i = 0; i < player->paths * 3 + player->pathsDigits; i++) {
	player->pathDeck[i] = buffer[i + player->pathsDigits];
    }
    check_path_deck(player);
    create_board(player);
}

//Create the game board. The board will be created using the path
//file. The population or '-' char of each site or barrier will be 
//replaced with ' '. Players will start on the board represented by their
//id's in decending order.
void create_board(Player* player) {
    player->board = malloc(sizeof(char*) * player->ptotal + 1);
    for(int i = 0; i < player->ptotal + 1; i++) {
	player->board[i] = malloc(sizeof(char) * (player->paths * 3));
    }
    for(int i = 0; i < player->ptotal + 1; i++) {
	for(int j = 0; j < player->paths * 3; j++) {
	    if(i != 0 && j == 0) {
		player->board[i][j] = 
			player->playerIDList[player->ptotal - i] + '0';
	    } else if(i == 0) {
		player->board[i][j] = player->pathDeck[j];
	    } else if(i != 0 && j != 0) {
		player->board[i][j] = ' ';
	    }
	}
    }
}

//Setup the item list for all players. Items are stored in a 2x2
//array as such.
//  list[i][0] = A
//  list[i][1] = B
//  .
//  .
//  .
//  list[i][4] = E
void setup_item_array(Player* player) {
    player->playerItems = malloc(sizeof(int*) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	player->playerItems[i] = malloc(sizeof(int) * 5);
	for(int j = 0; j < 5; j++) {
	    player->playerItems[i][j] = 0;
	}
    }
}

//Return the population of the given site.
int get_site_pop(Player* player, int j) {
    int pop = 0;
    for(int i = 1; i < player->ptotal + 1; i++) {
	if(player->board[i][j] != ' ') {
	    pop++;
	}
    }
    return pop;
}

//Add items to the last.
void add_item_to_list(Player* player, char id, int item) {
    int i = id - '0';
    int j = item - 1;
    player->playerItems[i][j] = player->playerItems[i][j] + 1;
}

//Print the players updated variables to stderr.
void print_player_update(Player* player, int i) {
    int id = player->playerIDList[i];
    int money = player->playerMoney[i];
    int v1 = player->vScores[i][0];
    int v2 = player->vScores[i][1];
    int points = player->playerPoints[i];
    int a = player->playerItems[i][0];
    int b = player->playerItems[i][1];
    int c = player->playerItems[i][2];
    int d = player->playerItems[i][3];
    int e = player->playerItems[i][4];
    fprintf(stderr, "Player %d Money=%d V1=%d V2=%d Points=%d ",
	    id, money, v1, v2, points);
    fprintf(stderr, "A=%d B=%d C=%d D=%d E=%d\n", a, b, c, d, e);
}

//Update the board. The update is based off the HAP message and not
//the site the player lands on.
void update_player_board(Player* player, char rxMsg[]) {
    int n, s, m, c;
    char p;
    if(sscanf(rxMsg, "HAP%c,%d,%d,%d,%d\n", &p, &n, &s, &m, &c) != 5) {
	exit_status(COMM_ERROR);
    }
    if((p - '0') < 0 || (p - '0') >= player->ptotal || n <= 0 || c > 5) {
	exit_status(COMM_ERROR);
    }
    if(c > 5) {
	exit_status(COMM_ERROR);
    } else if(c != 0) {
	add_item_to_list(player, p, c);
    }
    for(int i = 1; i < player->ptotal + 1; i++) {
	for(int j = 0; j < player->paths * 3; j++) {
	    if(player->board[i][j] == p) {
		player->board[i][j] = ' ';
	    }
	}
    }
    if(player->board[0][n * 3] == 'V') {
	if(player->board[0][(n * 3) + 1] == '1') {
	    player->vScores[p - '0'][0] = player->vScores[p - '0'][0] + 1;
	} else if(player->board[0][(n * 3) + 1] == '2') {
	    player->vScores[p - '0'][1] = player->vScores[p - '0'][1] + 1;
	}
    }
    if(m != 0) {
	player->playerMoney[p - '0'] = player->playerMoney[p - '0'] + m;
    }
    if(s != 0) {
	player->playerPoints[p - '0'] = player->playerPoints[p - '0'] + s;
    }
    print_player_update(player, p - '0');
    for(int i = 1; i < player->ptotal + 1; i++) {
	if(player->board[i][n * 3] == ' ') {
	    player->board[i][n * 3] = p;
	    return;
	}
    }
}

//If the player has money, and there is a Do site infront of 
//the player, the player will move there.
int move_one(Player* player) { 
    int col = player->currentCol;
    int paths = player->paths * 3;
    //scan the first row of the board from where the player is
    for(int j = col + 3; j < paths; j++) {
	if(player->board[0][j] == ':') {
	    return 0;
	}
	if(player->board[0][j] == 'D' && 
		player->playerMoney[player->id] != 0) {
	    int siteLimit = player->board[0][j + 2] - '0';
	    int sitePop = get_site_pop(player, j);
	    if(sitePop < siteLimit) {
		int moveToSite = j / 3;
		return moveToSite;
	    }
	}
    }
    return 0;	
}

//If the next site is a Mo, and there is a room, then go there.
int move_two(Player* player) {
    int col = player->currentCol;
    int siteMove = 0;
    int siteLimit = 0;
    if(player->board[0][col + 3] == 'M') {
	siteLimit = player->board[0][col + 5] - '0';
	int sitePop = get_site_pop(player, col + 3);
	if(sitePop < siteLimit) {	
	    siteMove = 1;
	}
    }
    return siteMove;
}

//Pick the closest V1, V2, or :: site and move there.
//This is the default move for player A.
int move_three(Player* player) {
    int col = player->currentCol;
    int paths = player->paths * 3;
    for(int j = col + 3; j < paths; j++) {
	if(player->board[0][j] == 'V' || player->board[0][j] == ':') {
	    int siteLimit = player->board[0][j + 2] - '0';
	    int sitePop = get_site_pop(player, j);
	    if(sitePop < siteLimit && player->board[0][j] != ':') {
		int siteMove = j / 3;
		return siteMove;
	    } else if(player->board[0][j] == ':') {
		int siteMove = j / 3;
		return siteMove;
	    }
	}
    }
    return 0;
}

//Calculate the score from all score sources.
void do_score(Player* player) {
    int* scoreList = malloc(sizeof(int) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	int points = player->playerPoints[i];
	int v1 = player->vScores[i][0];
	int v2 = player->vScores[i][1];
	scoreList[i] = points + v1 + v2;
    }
    fprintf(stderr, "Scores: ");
    fprintf(stderr, "%d", scoreList[0]);
    for(int i = 1; i < player->ptotal; i++) {
	fprintf(stderr, ",%d", scoreList[i]);
    }
    free(scoreList);
}

//Update the players position on the stored board.
void update_player_stats(Player* player, int siteMove, int move) {
    int j = siteMove * 3;
    if(player->board[0][j + 1] == '1') {
	player->sitesVisited[0]++;
    } else if(player->board[0][j + 1] == '2') {
	player->sitesVisited[1]++;
    }
    if(move == 1) {
	player->currentCol = siteMove * 3;
    } else if(move == 2) {
	player->currentCol = player->currentCol + 3;
    } else {
	player->currentCol = siteMove * 3;
    }
}

//Send the move DO message to the hub based on which move is selected
void send_move(Player* player) {
    int siteMove = 0;
    int move = 0;
    if(siteMove == 0 && move_one(player)) {
	siteMove = move_one(player);
	move = 1;
    } 
    if(siteMove == 0 && move_two(player)) {
	siteMove = move_two(player);
	move = 2;
    }
    if(siteMove == 0 && move_three(player)) {
	siteMove = move_three(player);
	move = 3;
    }
    update_player_stats(player, siteMove, move);
    fprintf(stdout, "DO%d\n", player->currentCol / 3);
    fflush(stdout);
}

//Check if the game has ended. The game ends when all plaeyrs
//are located on the final site
int is_game_over(Player* player) {
    int endGame = 1;
    int j = ((player->paths * 3) - 3);
    for(int i = 1; i < player->ptotal + 1; i++) {
	if(player->board[i][j] == ' ') {
	    endGame = 0;
	}
    }
    return endGame;
}

//Read the messages sent from the dealer
void read_message(Player* player) {
    while(1) {
	char rxMsg[256];
	char* ptr = &rxMsg[0];
	if(!fgets(ptr, 256, stdin)) {
	    exit_status(COMM_ERROR); 
	}
	ptr[strlen(ptr) - 1] = '\0';
	if(!strncmp("YT", ptr, 2)) {
	    send_move(player); //need to check if players turn with ID
	} else if(!strncmp("EARLY", ptr, 5)) {
	    exit_status(COMM_ERROR);
	} else if(!strncmp("DONE", ptr, 4)) {
	    if(is_game_over(player) == 1) {
		do_score(player);
		exit_status(NORMAL);
	    } else if(is_game_over(player) == 0) {
		exit_status(COMM_ERROR);
	    }
	} else if(!strncmp("HAP", ptr, 3)) {
	    update_player_board(player, rxMsg);
	    print_board(player);
	} else {
	    exit_status(COMM_ERROR);
	}
    }
}

//Create the score list to store the players score
void create_score_list(Player* player) {
    player->playerScore = malloc(sizeof(int) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	player->playerScore[i] = 0;
    }
}

//Create the vcore list to store v sites that player has landed on
void create_vscore_list(Player* player) {
    player->vScores = malloc(sizeof(int*) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	player->vScores[i] = malloc(sizeof(int) * 2);
	for(int j = 0; j < 2; j++) {
	    player->vScores[i][j] = 0;
	}
    }
}

//Create the point list to store players points from Do sites
void create_points_list(Player* player) {
    player->playerPoints = malloc(sizeof(int) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	player->playerPoints[i] = 0;
    }
}

//Create the money list to store players money 
void create_money_list(Player* player) {
    player->playerMoney = malloc(sizeof(int) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	player->playerMoney[i] = 7;
    }
}

//Create the ID list to store players ID
void create_id_list(Player* player) {
    player->playerIDList = malloc(sizeof(int) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	player->playerIDList[i] = i;
    }
}

//Initialise and validiate player arguements
void initialise_variables(Player* player, int argc, char** argv) {
    char* err1;
    char* err2;
    if(argc != 3) {
	exit_status(INVALID_ARGS);
    }
    player->ptotal = strtoul(argv[1], &err1, 10);
    if(*err1 != '\0') {
	exit_status(INVALID_PCOUNT);
    }
    if(!strlen(argv[2])) {
	exit_status(INVALID_ID);
    }
    for(char* p = argv[2]; *p != '\0'; p++) {
	if(!isdigit(*p)) {
	    exit_status(INVALID_ID);
	}
    }
    player->id = strtoul(argv[2], &err2, 10);
    if(player->id < 0) {
	exit_status(INVALID_ID);
    }
    if(*err2 != '\0') {
	exit_status(INVALID_ID);
    }
    create_score_list(player);
    create_id_list(player);
    create_money_list(player);
    create_points_list(player);
    create_vscore_list(player);
    player->currentRow = 0;
    player->currentCol = 0;
    setup_item_array(player);
}

//Start the player. Initilise variables for the player and create lists
//for other players variables to store.
void start_player(Player* player, int argc, char** argv) {
    initialise_variables(player, argc, argv);
    player->sitesVisited = malloc(sizeof(int) * 2);
    player->sitesVisited[0] = 0;
    player->sitesVisited[1] = 0;
    fprintf(stdout, "^");
    fflush(stdout);
    read_path_deck(player);    
    print_board(player);
    read_message(player);
}
	
int main(int argc, char** argv) {
    Player* player = malloc(sizeof(Player));
    start_player(player, argc, argv);
    return 1;

}
