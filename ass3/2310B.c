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
//sites or player id's store on them. char_on_row() will
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

//Check the validity of the sites. If a site is invalid 
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
void setup_item_list(Player* player) {
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

//Update the players variables. The players variables will update
//based of the HAP message recived. IF HAP message is incorrect
//but variables are still valid the message will go though.
//This was specified in criteria.
void update_player_board(Player* player, char rxMsg[]) {
    int n, s, m, c;
    char p;
    if(sscanf(rxMsg, "HAP%c,%d,%d,%d,%d\n", &p, &n, &s, &m, &c) != 5) {
	exit_status(COMM_ERROR);
    }
    if((p - '0') < 0 || (p - '0') >= player->ptotal || n <= 0 || c > 5) {
	exit_status(COMM_ERROR);
    }
    if(c > 5 && c >= 0) {
	exit_status(COMM_ERROR);
    } else if(c != 0) {
	add_item_to_list(player, p, c);
    }
    if(n > player->paths || n <= 0) {
	exit_status(COMM_ERROR);
    }
    for(int i = 1; i < player->ptotal + 1; i++) {
	for(int j = 0; j < player->paths * 3; j++) {
	    if(player->board[i][j] == p) {
		player->board[i][j] = ' ';
	    }
	}
    }
    if (player->board[0][n * 3] == 'V') {
	if (player->board[0][(n * 3) + 1] == '1') {
	    player->vScores[p - '0'][0] = player->vScores[p - '0'][0] + 1;
	} else if (player->board[0][(n * 3) + 1] == '2') {
	    player->vScores[p - '0'][1] = player->vScores[p - '0'][1] + 1;
	}
    }
    if (m != 0) {
	player->playerMoney[p - '0'] = player->playerMoney[p - '0'] + m;
    }
    if (s != 0) {
	player->playerPoints[p - '0'] = player->playerPoints[p - '0'] + s;
    }
    print_player_update(player, p - '0');
    for(int i = 1; i < player->ptotal + 1; i++) {
	if (player->board[i][n * 3] == ' ') {
	    player->board[i][n * 3] = p;
	    return;
	}
    }
}

//Check if the player is last on the board. Last meaning there is no
//other players in the same column or in pervious columns.
int is_player_last(Player* player) {
    int isLast = 1;
    int col = player->currentCol;
    for(int i = 1; i < player->ptotal; i++) {
	for(int j = 0; j <= col; j++) {
	    if(player->board[i][j] != ' ' && 
		    player->board[i][j] != player->id + '0') {
		isLast = 0;
	    }
	}
    }
    return isLast;
}

//Count the total amount of cards the player has.
int total_cards(Player* player, int id) {
    int cards = 0;
    for(int j = 0; j < 5; j++) {
	cards = cards + player->playerItems[id][j];
    }
    return cards;
}

//Move to the next site if coming last and site is not full.
int move_one(Player* player) {
    int col = player->currentCol;
    int siteMove = 0;
    int siteLimit = player->board[0][col + 5] - '0';
    int sitePop = get_site_pop(player, col + 3);
    if(sitePop < siteLimit) {	
	if(is_player_last(player) == 1) {
	    siteMove = 1;
	}
    }
    return siteMove;
}

//If the player has an odd amount of money and there is 
//a Mo between us and the next barrier player will move there.
int move_two(Player* player) {
    int col = player->currentCol;
    int paths = player->paths * 3;
    int mSiteAt = 0;
    int barrierAt = 0;
    if(player->playerMoney[player->id] % 2 != 1) {
	return 0;
    }
    for(int j = col + 1; j < paths; j++) {
	if(player->board[0][j] == 'M' && mSiteAt == 0 && 
		get_site_pop(player, j) < (player->board[0][j + 2] - '0')) {
	    mSiteAt = j / 3;
	}
	if(player->board[0][j] == ':' && barrierAt == 0 && j > col + 1) {
	    barrierAt = j / 3;
	}
    }
    if(mSiteAt != 0 && mSiteAt < barrierAt) {
	return mSiteAt;
    }
    return 0;
}

//If the player has the most card or all players have no cards, 
//move to the next Ri site if there is a barrier between the 
//player and the site
int move_three(Player* player) {
    int thisPlayersCards = total_cards(player, player->id);
    int totalCards = 0;
    int rSiteAt = 0;
    int barrierAt = 0;
    int paths = player->paths * 3;
    int col = player->currentCol;
    int count = 0;
    int zeroCount = 1;
    for(int i = 0; i < player->ptotal; i++) {
	if(i == player->id) {
	    thisPlayersCards = total_cards(player, i);
	} else {
	    totalCards = total_cards(player, i);
	}
	if(totalCards <= thisPlayersCards) {
	    count++;
	} else if(totalCards == 0 && thisPlayersCards == 0) {
	    zeroCount++;
	}
	if(count != 0 || zeroCount != player->ptotal) {
	    if(totalCards != 0 && thisPlayersCards != 0) {
		return 0;
	    } else {
		continue;
	    }
	}
    }
    for(int j = col + 1; j < paths; j++) {                              
	if(player->board[0][j] == 'R' && rSiteAt == 0 &&              
		get_site_pop(player, j) < (player->board[0][j + 2] - '0')) {   
            rSiteAt = j / 3;
        }
        if(player->board[0][j] == ':' && barrierAt == 0 && j > col + 1) {
            barrierAt = j / 3;
        }
    }
    if(rSiteAt != 0 && rSiteAt < barrierAt) {
        return rSiteAt;
    }
    return 0;
} 

//If a V2 site is between us and a barrier, player will move their.
int move_four(Player* player) {
    int col = player->currentCol;
    int paths = player->paths * 3;
    int vSiteAt = 0;
    int barrierAt = 0;
    for(int j = col + 1; j < paths; j++) {                                
        if(player->board[0][j] == 'V' && player->board[0][j + 1] == '2' && 
		vSiteAt == 0 && get_site_pop(player, j) < 
		(player->board[0][j + 2] - '0')) {   
            vSiteAt = j / 3;                                            
        }                                                             
        if(player->board[0][j] == ':' && barrierAt == 0 && j > col + 1) {
            barrierAt = j / 3;                                          
        }                                                             
    }                                                                 
    if(vSiteAt != 0 && vSiteAt < barrierAt) {                         
        return vSiteAt;                                               
    }                                                                 
    return 0;                                                         
}

//Move forward to the first site that has room.
int move_five(Player* player) {
    int col = player->currentCol;
    int paths = player->paths * 3;
    int siteMove = 0;
    for(int j = col + 2; j < paths; j++) {
	char site = player->board[0][j];
	if(site == ':' && j > col && siteMove == 0) {
	    siteMove = j / 3;
	    return siteMove;
	}
	if(site == 'M' || site == 'D' || site == 'V' || site == 'R') {
	    int sitePop = get_site_pop(player, j);
	    int siteLimit = player->board[0][j + 2] - '0';
	    if(sitePop < siteLimit) {
		siteMove = j / 3;
		return siteMove;
	    }
	}
    }
    return siteMove;
}

//Based on the move the player makes, update their currnet
//position on the board.
void update_player_stats(Player* player, int siteMove, int move) {
    int j = siteMove * 3;
    if(player->board[0][j + 1] == '1') {
	player->sitesVisited[0]++;
    } else if(player->board[0][j + 1] == '2') {
	player->sitesVisited[1]++;
    }
    if(move == 1) {
	player->currentCol = player->currentCol + 3;
    } else if(move == 2) {
	player->currentCol = siteMove * 3;
    } else if(move == 3) {
	player->currentCol = siteMove * 3;		
    } else if(move == 4) {
	player->currentCol = siteMove * 3;
    } else if(move == 5) {
	player->currentCol = siteMove * 3;
    }
}

//Determines which move will be made. Moves are prioritised from
//move 1, move 2, ..., move 5.
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
    if(siteMove == 0 && move_four(player)) {
	siteMove = move_four(player);
	move = 4;
    }
    if(siteMove == 0 && move_five(player)) {
	siteMove = move_five(player);
	move = 5;
    }
    update_player_stats(player, siteMove, move);
    fprintf(stdout, "DO%d\n", player->currentCol / 3);
    fflush(stdout);
}

//Score items collected by players. Score is based off how
//many sets of items a player has.
int score_items(Player* player, int i) {
    int points = 0;
    int scores[5];
    scores[0] = player->playerItems[i][0];
    scores[1] = player->playerItems[i][1];
    scores[2] = player->playerItems[i][2];
    scores[3] = player->playerItems[i][3];
    scores[4] = player->playerItems[i][4];
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

//Calculate the score from all score sources.
void do_score(Player* player) {
    int* scoreList = malloc(sizeof(int) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	int points = player->playerPoints[i];
	int v1 = player->vScores[i][0];
	int v2 = player->vScores[i][1];
	int score = score_items(player, i);
	scoreList[i] = points + v1 + v2 + score;
    }

    fprintf(stderr, "Scores: ");
    fprintf(stderr, "%d", scoreList[0]);
    for(int i = 1; i < player->ptotal; i++) {
	fprintf(stderr, ",%d", scoreList[i]);
    }
    free(scoreList);
}

//Check if the game is over. The is considered over when all players
//are on the final site.
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

//Start reading messages from the dealer and take 
//actions based on the message
void read_message(Player* player) {
    while(1) {
	char rxMsg[256];
	char* rxScan = &rxMsg[0];
	if(!fgets(rxScan, 256, stdin)) {
	    exit_status(COMM_ERROR); 
	}
	rxScan[strlen(rxScan) - 1] = '\0';
	if(!strncmp("YT", rxScan, 2)) {
	    if(strlen(rxScan) != 2) {
		exit_status(COMM_ERROR);
	    }
	    send_move(player); //need to check if players turn with ID
	} else if(!strncmp("EARLY", rxScan, 5)) {
	    exit_status(COMM_ERROR);
	} else if(!strncmp("DONE", rxScan, 4)) {
	    if(strlen(rxScan) != 4) {
		exit_status(COMM_ERROR);
	    }
	    if(is_game_over(player) == 1) {
		do_score(player);
		exit_status(NORMAL);
	    } else if(is_game_over(player) == 0) {
		exit_status(COMM_ERROR);
	    }
	} else if(!strncmp("HAP", rxScan, 3)) {
	    update_player_board(player, rxMsg);
	    print_board(player);
	} else {
	    exit_status(COMM_ERROR);
	}
    }
}

//Create score list used to calculate final scores.
void create_score_list(Player* player) {
    player->playerScore = malloc(sizeof(int) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	player->playerScore[i] = 0;
    }
}

//Create list to store player V1 and V2 sites.
void create_vscore_list(Player* player) {
    player->vScores = malloc(sizeof(int*) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	player->vScores[i] = malloc(sizeof(int) * 2);
	for(int j = 0; j < 2; j++) {
	    player->vScores[i][j] = 0;
	}
    }
}

//Create list to store player points. These will be points from Do sites.
void create_points_list(Player* player) {
    player->playerPoints = malloc(sizeof(int) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	player->playerPoints[i] = 0;
    }
}

//Create list to store player money. Players start with 7 money.
void create_money_list(Player* player) {
    player->playerMoney = malloc(sizeof(int) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	player->playerMoney[i] = 7;
    }
}

//Create list to store player id's.
void create_id_list(Player* player) {
    player->playerIDList = malloc(sizeof(int) * player->ptotal);
    for(int i = 0; i < player->ptotal; i++) {
	player->playerIDList[i] = i;
    }
}

//Initialise and varify the validity of player variables.
void initialise_variables(Player* player, int argc, char** argv) {
    char* err1;
    char* err2;
    if(argc != 3) {
	exit_status(INVALID_ARGS);
    }
    player->ptotal = strtoul(argv[1], &err1, 10);
    if(*err1 != '\0' || player->ptotal < 1) {
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
    if(player->id < 0 || player->id >= player->ptotal) {
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
    setup_item_list(player);
    player->currentRow = 0;
    player->currentCol = 0;
}

//Start the player. Initialise variables for the player and create lists
//for for other players variables to store.
void start_player(Player* player, int argc, char** argv) {
    initialise_variables(player, argc, argv);
    player->sitesVisited = malloc(sizeof(int) * 2);
    player->sitesVisited[0] = 0;
    player->sitesVisited[1] = 0;
    fprintf(stdout, "^");   //send acknowledgment char
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
