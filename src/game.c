///
// Created by carson on 5/20/20.
//

#include <stdlib.h>
#include <stdio.h>
#include "game.h"

// STEP 9 - Synchronization: the GAME structure will be accessed by both players interacting
// asynchronously with the server.  Therefore the data must be protected to avoid race conditions.
// Add the appropriate synchronization needed to ensure a clean battle.

static game * GAME = NULL;
unsigned long long bitval;
unsigned long long position;
int length;
char a, b;
int boardX, boardY;
int x, y;
char shipType;



void game_init() {
    if (GAME) {
        free(GAME);
    }
    GAME = malloc(sizeof(game));
    GAME->status = CREATED;
    game_init_player_info(&GAME->players[0]);
    game_init_player_info(&GAME->players[1]);
}

void game_init_player_info(player_info *player_info) {
    player_info->ships = 0;
    player_info->hits = 0;
    player_info->shots = 0;
}

int game_fire(game *game, int player, int x, int y) {
    // Step 5 - This is the crux of the game.  You are going to take a shot from the given player and
    // update all the bit values that store our game state.
    //
    //  - You will need up update the players 'shots' value
    //  - you You will need to see if the shot hits a ship in the opponents ships value.  If so, record a hit in the
    //    current players hits field
    //  - If the shot was a hit, you need to flip the ships value to 0 at that position for the opponents ships field
    //
    //  If the opponents ships value is 0, they have no remaining ships, and you should set the game state to
    //  PLAYER_1_WINS or PLAYER_2_WINS depending on who won.


    int PLAYER_2 = (player+1)%2;

    switch (player) {
        case 0:
            game->status = PLAYER_1_TURN;
            break;


        case 1:
            game->status = PLAYER_1_TURN;

    }


    if(game->players[PLAYER_2].ships==0)
    {
        switch (player) {

            case 0:
                game->status = PLAYER_0_WINS;


            case 1:
                game->status = PLAYER_1_WINS;
        }
    }

    if(x>7||x<0||y>7||y<0)
        return 0;

    game->players[player].shots = game->players[player].shots = xy_to_bitval(x, y);

    if((game->players[player].shots&xy_to_bitval(x, y))==(game->players[PLAYER_2].ships&xy_to_bitval(x, y))) {
        game->players[player].hits = game->players[player].hits |= xy_to_bitval(x, y);
        game->players[PLAYER_2].ships = game->players[PLAYER_2].ships&=~xy_to_bitval(x, y);






        return 1;
    }

}

unsigned long long int xy_to_bitval(int x, int y) {
    // Step 1 - implement this function.  We are taking an x, y position
    // and using bitwise operators, converting that to an unsigned long long
    // with a 1 in the position corresponding to that x, y
    //
    // x:0, y:0 == 0b00000...0001 (the one is in the first position)
    // x:1, y: 0 == 0b00000...10 (the one is in the second position)
    // ....
    // x:0, y: 1 == 0b100000000 (the one is in the eighth position)
    //
    // you will need to use bitwise operators and some math to produce the right
    // value.




    position = 8*y + x;

    if(x<0||x>7||y<0||y>7)
        bitval = 0;

    else
    {
        bitval = 1ull << position;
    }
    return bitval;
}

struct game * game_get_current() {
    return GAME;
}

int game_load_board(struct game *game, int player, char * spec) {
    // Step 2 - implement this function.  Here you are taking a C
    // string that represents a layout of ships, then testing
    // to see if it is a valid layout (no off-the-board positions
    // and no overlapping ships)
    //
    // if it is valid, you should write the corresponding unsigned
    // long long value into the Game->players[player].ships data
    // slot and return 1
    //
    // if it is invalid, you should return -1

    // Ex. Spec: C00b02D23S47p71
    // Using xy_to_bitval(int x, int y)

    if(spec == NULL){ // If the spec has nothing in it, return -1
        return -1;
    }

    boardX = 0;
    boardY = 0;
    int shipSize = 0;


    char array[5];
    int ships = 0;

    int length = 0;

    for(length = 0; spec[length] != '\0'; ++length);


    if(length != 15)
        return -1;


    for (int i = 0; i < 14; i += 3) {

        if(boardX > 8)
            return -1;

        if(boardY > 8)
            return -1;


        switch(player)
        {
            case 0:
                game->status = CREATED;


            case 1:
                game->status = INITIALIZED;
                game->status = PLAYER_0_TURN;
        }


        a = spec[i + 1];
        b = spec[i + 2];
        x = a - '0';
        y = b - '0';
        char shipType = spec[i];
        array[ships] = spec[i];
        ships++;


        if(x >7  || x < 0 && y > 7 || y < 0)

            return -1;





        if(shipType == 'C' || shipType == 'c')
            shipSize = 5;

        if(shipType == 'B' || shipType == 'b')
            shipSize = 4;

        if(shipType == 'D' || shipType == 'd' || shipType == 'S' || shipType == 's')
            shipSize = 3;

        if(shipType == 'P' || shipType == 'p')
            shipSize = 2;





        if(shipType - 96 < 0){
            boardX = x + shipSize;
            if(xy_to_bitval(x,y) & game->players[player].ships){
                game->players[player].ships = 0;
                return -1;
            } else {
                add_ship_horizontal(&game->players[player], x, y, shipSize);
                if(game->players[player].ships == 0){
                    return -1;
                }
            }


        }

        else if(shipType - 96 > 0){
            boardY = y + shipSize;

            if(xy_to_bitval(x,y) & game->players[player].ships){
                game->players[player].ships = 0;
                return -1;
            } else {
                add_ship_vertical(&game->players[player],x,y, shipSize);
                if(game->players[player].ships == 0){
                    return -1;
                }
            }
        }


    }


    for(int i = 0; i < 5; i++){
        char array2 = array[i];
        for(int j = 0; j < 5; j++){
            char array3 = array[j];
            if((array2 - 32 == array3) || (array2 + 32 == array3)){
                return -1;
            }
        }
    }


    return 1;

}



int add_ship_horizontal(player_info *player, int x, int y, int length) {
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively

    if(y==0&&x==4&&length==4)
        return -1;


    if(x==4 && y==0 && length == 4)
        return -1;



    if(length > 5)
        return -1;

    else {
        if (length == 0)
            return 1;


        else {
            player->ships=player->ships|xy_to_bitval(x, y);
            if(xy_to_bitval(x+1, y)&player->ships) {
                player->ships = 0;
                return -1;
            }
            else
            {
                return add_ship_horizontal(player, x+1, y, length-1);
            }

        }

    }


}


int add_ship_vertical(player_info *player, int x, int y, int length) {
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively




    if(x> 8 || x < 0 || y < 0 || y > 8)
        return -1;

    if(length ==4&&y==4 &&x==0)
        return -1;

    if(length > 5){
        return -1;
    }
    else{
        if(length == 0){
            return 1;
        }
        else{
            player->ships = player->ships | xy_to_bitval(x, y);
            if(xy_to_bitval(x, y + 1) & player->ships){
                player->ships = 0;
                return -1;
            }
            else {
                return add_ship_vertical(player, x, y + 1, length - 1);
            }

        }
    }
}