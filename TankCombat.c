/* 
    ----------------------------------------------- TankCombat.C -------------------------------------------------------
    Programmers                 : Mehul Joshi, Cong Troung
    Course Number               : BEE 496, Capstone Project
    Facilitator/Sponsor         : Professor Joseph C. Decuir
    Creation Date               : September 29, 2023
    Date of Last Modification   : 7/29/2024
    --------------------------------------------------------------------------------------------------------------------
    Project Details
        Description             : Tank Combat source code developed using C
        Compiler                : CC65
        Goal                    : Porting the original Tank Combat from Atari 2600 to Atari 800
    --------------------------------------------------------------------------------------------------------------------
    NOTE:
        Code Key:
            Player 1 = P0
            Player 2 = P1 (AI)
    --------------------------------------------------------------------------------------------------------------------
*/


/*
    -------------------------------------------- Libraries (Header Files) ----------------------------------------------------
    Libraries used down below are provided by the CC65 Compiler. To download the CC 65 compiler please refer to the
    link provided: https://cc65.github.io/
*/
#include <atari.h>
#include <_antic.h>
#include <_atarios.h>
#include <peekpoke.h>
#include <conio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <joystick.h>

/*
    ----------------------------------------------- IDENTIFIERS -------------------------------------------------------
*/
//Defining the 16 tank rotations
#define NORTH               0
#define NORTH_15            1
#define NORTH_EAST          2
#define NORTH_60            3
#define EAST                4
#define EAST_15             5
#define EAST_SOUTH          6
#define EAST_60             7
#define SOUTH               8
#define SOUTH_15            9
#define SOUTH_WEST          10
#define SOUTH_60            11
#define WEST                12
#define WEST_15             13
#define WEST_NORTH          14
#define WEST_60             15



/*
    // FORWARD, BACKWARD, LEFT TURN, RIGHT TURN, FIRE
    // unsigned char moves[5] = {0x01, 0x02, 0x04, 0x08, 0x10};
*/
#define FORWARD 0x01
#define BACKWARD 0x02
#define LEFT_TURN 0x04
#define RIGHT_TURN 0x08
#define FIRE 0x10


//collision detection definitions, add all registers
#define P1PF                0xD005         //Player 1 to playfield Collision Register    
#define P0PF                0xD004         //Player 0 to Playfield Collision Register
#define M1PF                0xD001         //Missile 1 to Playfield Collision Register
#define M0PF                0xD000         //Missile 0 to Playfield Collision Register
#define M1P                 0xD009         //Missile 1 to Player Collision Register
#define M0P                 0xD008         //Missile 0 to Player Collision Register

#define HITCLR              0xD01E         //Collsion Clear Register: Poking a 1 clears ALL collision registers

/*
    ----------------------------------------------- GLOBAL VARAIABLES -------------------------------------------------------
*/
//Different tank pictures to be printed
unsigned int tankPics[16][8] = {
        {8,8,107,127,127,127,99,99},        //NORTH
        {36,100,121,255,255,78,14,4},       //NORTH_15
        {25,58,124,255,223,14,28,24},       //NORTH_EAST
        {28,120,251,124,28,31,62,24},       //NORTH_60
        {0,252,252,56,63,56,252,252},       //EAST
        {24,62,31,28,124,251,120,28},       //EAST_15
        {25,58,124,255,223,14,28,24},       //SOUTH_EAST
        {4,14,78,255,255,121,36},           //EAST_60
        {99,99,127,127,127,107,8,8},        //SOUTH
        {32,112,114,255,255,158,38,36},     //SOUTH_15
        {152,92,62,255,251,112,56,24},      //SOUTH_WEST
        {24,124,248,56,62,223,30,56},       //SOUTH_60
        {0,63,63,28,252,28,63,63},          //WEST
        {56,30,223,62,56,248,124,24},       //WEST_15
        {152,92,62,255,251,112,56,24},      //NORTH_WEST
        {36,38,158,255,255,114,112,32}      //WEST_60
};

// row, col
// y, x
short deltas[16][2] = {
    {-1, 0},            // NORTH
    {-2, 1},            // NORTH_15
    {-1, 1},            // NORTH_EAST
    {-1, 2},            // NORTH_60
    {0, 1},             // EAST
    {1, 2},             // EAST_15
    {1, 1},             // EAST_SOUTH
    {2, 1},             // EAST_60
    {1, 0},             // SOUTH
    {2, -1},            // SOUTH_15
    {1, -1},            // SOUTH_WEST
    {1, -2},            // SOUTH_60
    {0, -1},            // WEST
    {-1, -2},           // WEST_15
    {-1, -1},           // WEST_NORTH
    {-2, -1}            // WEST_60
};

unsigned char j = 255;
unsigned char m0SoundTracker = 0;
unsigned char m1SoundTracker = 0;
int i;
int frameDelayCounter = 0;

//Adresses
int bitMapAddress;
int charMapAddress;
int PMBaseAddress;
int playerAddress;
int missileAddress;

//Horizontal Positions Registers
int *horizontalRegister_P0 = (int *)0xD000;
int *horizontalRegister_M0 = (int *)0xD004;
int *horizontalRegister_P1 = (int *)0xD001;
int *horizontalRegister_M1 = (int *)0xD005;

//Color-Luminance Registers
int *colLumPM0 = (int *)0x2C0;
int *colLumPM1 = (int *)0x2C1;

//Starting direction of each Players
unsigned int p0Direction = EAST;
unsigned int p1Direction = WEST;
unsigned char p0LastMove;
unsigned char p1LastMove;
unsigned char p0history;
unsigned char p1history;

//Variables to track vertical and horizontal locations of players
int p0VerticalLocation = 131;
int p0HorizontalLocation = 57;
int p1VerticalLocation = 387;
int p1HorizontalLocation = 190;


// variables to track the vertical and horizontal locations of the players in a new reference frame
/* One thing that we found out while trying to run BFS is that there is no set position system for this game
 * and that is just one of the weird things that come with the Atari. We found out that in reference
 * to player 1 the board starts at (52, 55) and ends at (196, 216). Meaning that the board has 144x161
 * playable locations. Using this information we can create a position system through using (52, 55) as the new (0,0)
 * Implying that player0 which is used to start at (57, 131) now starts at (57-52, 131-55) = (5, 76). But since that is
 * very start of the spirte we move it 4 down and 4 across to get the position of the "middle of the sprite" = (9, 80).
 * Doing the very same thing for sprite2 and we see that it's position is at (194-52,135-55) = (142, 80).
 * I think a easier way to compute is just to use (r, c) notation especially in this case because we are going to be rotating
 * the "AI tank" a lot so....
 * (9,80) =(row,col)=> (80, 9)
 * (142, 80) =(row, col)=> (80, 142). 
 * Now where ever we update the p0VerticalLocation etc etc we should update these as well.
 */

int p0_r = 80;
int p0_c = 9;
int p1_r = 80;
int p1_c = 142;
int k = 0;


/* p0VerticalLocation = p0_r
 * p0HorizontalLocation = p0_c
 * p1VerticalLocation = p1_r
 * p1HorizontalLocation = p1_c
 */

//variables for missile tracking
int m0LastHorizontalLocation;
int m0LastVerticalLocation;
int m1LastHorizontalLocation;
int m1LastVerticalLocation;
int m0direction;
int m1direction;

//variables to keep track of tank firing
bool p0Fired = false;
bool p1Fired = false;
bool m0exists = false;
bool m1exists = false;
int p0FireDelayCounter = 0;
int p1FireDelayCounter = 0;
bool p0FireAvailable = true;
bool p1FireAvailable = true;

//tank hit variables
bool p0IsHit = false;
bool p1IsHit = false;
int p0HitDir = 0;
int p1HitDir = 0;
int hitTime[2] = {0, 0};

//variables to delay tank diagonal movement
bool p0FirstDiag = false;
bool p1FirstDiag = false;

//scores
//functions to turn and update tank positions
int *characterSetP0[8] = {
        (int*)0x0030,
        (int*)0x0011,
        (int*)0x0000,
        (int*)0x0037,
        (int*)0x0029,
        (int*)0x002E,
        (int*)0x0033,
        (int*)0x0001
};

int *characterSetP1[8] = {
        (int*)0x0030,
        (int*)0x0012,
        (int*)0x0000,
        (int*)0x0037,
        (int*)0x0029,
        (int*)0x002E,
        (int*)0x0033,
        (int*)0x0001
};

int p0Score = 16;
int p1Score = 16;

//variable to run the game, if it is false a user has won
bool gameOn = true;

/*
    ----------------------------------------------- FUNCTION DECLARATIONS -------------------------------------------------------
*/
void rearrangingDisplayList();
void initializeScore();
void createBitMap();
void enablePMGraphics();
void setUpTankDisplay();
void spinTank(int tank);
void movePlayers();
void fire(int tank);
void missileLocationHelper(unsigned int tankDirection, int pLastHorizontalLocation, int pLastVerticalLocation, int tank);
void traverseMissile(unsigned int missileDirection, int mHorizontalLocation, int mVerticalLocation, int tank);
void moveForward(int tank);
void moveBackward(int tank);
void checkCollision();
void turnplayer(unsigned char turn, int player);
void tankExplosion();
void updateplayerDir(int player);

/*
    ----------------------------------------------- MAIN DRIVER -------------------------------------------------------
*/
int main() {
    //Loading and installing joystick driver
    joy_load_driver(joy_stddrv);
    joy_install(joy_static_stddrv);     

    //Set Up Display Screen
    _graphics(18);                      //Set default display to graphics 3 + 16 (+16 displays mode with graphics, eliminating the text window)
    rearrangingDisplayList();           //rearranging graphics 3 display list
    initializeScore();
    createBitMap();                     //Create bit map
    enablePMGraphics();                 //Enable Player Missile Graphics
    setUpTankDisplay();                 //Set up PLayer 1 and 2 Tank display

    //First while loop to prevent program carshing in native hardware
    while (true) {
        while (gameOn) {
            //Slows down character movement e.g. (60fps/5) = 12moves/second (it is actually slower than this for some reason)
            if (frameDelayCounter == 5)
            {
                movePlayers();
                frameDelayCounter = 0;

                //if either of the players are hit, spin and move them, rather than letting them fire or move
                if (p0IsHit && hitTime[0] > 0) spinTank(0);
                if (p1IsHit && hitTime[1] > 0) spinTank(1);
                
                if(j < 12)
                {
                    _sound(0, j , 8, 8);
                    j++;
                }

                if(j >= 12) _sound(0, 0, 0, 0); //Turn off sound register
            } else {
                frameDelayCounter++;
            }


            //Makes a firing sound when P1 presses the fire button
            if (p0Fired == true) {
                m0SoundTracker++;

                if (m0SoundTracker < 15) {
                    _sound(0, m0SoundTracker, 8, 2);
                } else if (m0SoundTracker == 15) {
                    m0SoundTracker = 0;
                    _sound(0, 0, 0, 0); //Turns off sound register for Audio Channel 0
                    p0Fired = false;
                }
            }

            //Makes a firing sound when P1 presses the fire button
            if (p1Fired == true) {
                m1SoundTracker++;

                if (m1SoundTracker < 15) {
                    _sound(1, m1SoundTracker, 8, 2);
                } else if (m1SoundTracker == 15) {
                    m1SoundTracker = 0;
                    _sound(1, 0, 0, 0); //Turns off sound register for Audio Channel 1
                    p1Fired = false;
                }
            }

            if (p0FireAvailable == false) { //start counter to limit p0 fire inputs
                p0FireDelayCounter++;
            }
            if (p0FireDelayCounter >= 60) {
                p0FireAvailable = true;
                p0FireDelayCounter = 0;
            }
            if (p1FireAvailable == false) { //start counter to limit p0 fire inputs
                p1FireDelayCounter++;
            }
            if (p1FireDelayCounter >= 60) {
                p1FireAvailable = true;
                p1FireDelayCounter = 0;
            }


            if (m0exists == true) {
                traverseMissile(m0direction, m0LastHorizontalLocation, m0LastVerticalLocation, 0);
            }
            if (m1exists == true) {
                traverseMissile(m1direction, m1LastHorizontalLocation, m1LastVerticalLocation, 1);
            }

            //Checking Collision every single frame
            checkCollision();
            p1history = p1LastMove; //helps to fix collision bug
            p0history = p0LastMove; //helps to fix collision bug


            //This condition will only be met when either player 1 or player 2 reaches the score of 9,
            //or charcater 9 (which is 0x0019 in HEX located at Character Memory)
            if (p0Score == 25 || p1Score == 25) {
                int tracker = 0;

                for (i = 0; i < 20; i++) {
                    POKE(charMapAddress + i, 0);

                    if (i >= 6 && i <= 13) {
                        if (p0Score == 25) {
                            POKE(charMapAddress + i, characterSetP0[tracker]);
                        } else if (p1Score == 25) {
                            POKE(charMapAddress + i, characterSetP1[tracker]);
                        }
                        tracker++;
                    }
                }

                gameOn = false;
            }

            waitvsync();
        }
    }

    return 0;
}

/*
    ----------------------------------------------- FUNCTION IMPLEMENTATIONS -------------------------------------------------------
*/

//------------------------------ rearrangingDisplayList ------------------------------
// Purpose: Reconfigure the display list for graphics mode to ensure proper rendering.
//          This function sets up the necessary parameters in the display list.
//          It includes blank lines, character and screen memory configurations,
//          and a jump instruction for vertical blank synchronization.
//          The resulting display list must have a total of 192 scan lines.
// Parameters: None
// Preconditions: The OS object must be initialized with appropriate values.
// Postconditions: The display list is modified as per the graphics mode requirements.
void rearrangingDisplayList() {
    unsigned int *DLIST_ADDRESS  = (OS.sdlstl + OS.sdlsth*256);
    unsigned char *DLIST = (unsigned char *)DLIST_ADDRESS;

    // Write the Display List for Graphics Mode
    // Must contain a total of 192 Scan Lines for display list to function properly
    unsigned char displayList[] = {
            DL_BLK8,  // 8 blank lines
            DL_BLK8,
            DL_BLK8,
            DL_LMS(DL_CHR20x16x2),
            0xE0, 0x9C,  // Charcater Memory (Low Byte and High Byte)
            DL_LMS(DL_MAP40x8x4),
            0xF4,0x9C,  //Screen memory (Low Byte and High Byte)
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_MAP40x8x4,
            DL_JVB,  // Jump and vertical blank
            0x4E, 0x9E  // Jump address (in this case, loop back to the start)
    };

    for (i = 0; i < 30; i++) {
        POKE(DLIST + i, displayList[i]);
    }

    //Setting bit and character map address based on the modified display list 
    bitMapAddress = PEEK(DLIST+7) + PEEK(DLIST+8)*256;
    charMapAddress = PEEK(DLIST+4) + PEEK(DLIST+5)*256;
}

//------------------------------ initializeScore ------------------------------
// Purpose: Creating a scoreboard that sets both player's score
// Parameters: None
// Preconditions: None
// Postconditions: The display list will have a score board that sets both player's
//                 score to 0.
void initializeScore() {
    //Temp code
    POKE(charMapAddress + 5, 16);
    POKE(charMapAddress + 14, 16);
}

//------------------------------ updatePlayerScore ------------------------------
// Purpose: Update player's score based on collisions
// Parameters: None
// Preconditions: Tank to missile collision must be true
// Postconditions: The scoreboard will be updated
void updatePlayerScore() {
    POKE(charMapAddress + 5, p0Score);
    POKE(charMapAddress + 14, p1Score);
}

//------------------------------ createBitMap ------------------------------
// Purpose: Create bit map (Sprites and Borders)
// Parameters: None
// Preconditions: None
// Postconditions: Bit Map will be created
void createBitMap() {
    //Making the top and bottom border
    for (i = 0; i < 10; i++)
    {
        POKE(bitMapAddress+i, 170);
        POKE(bitMapAddress+210+i, 170);
    }

    //Making the left border
    for (i = 10; i <= 200; i += 10)
    {
        POKE(bitMapAddress+i, 128);
    }

    //Making the right border
    for (i = 19; i <= 209; i += 10)
    {
        POKE(bitMapAddress+i, 2);
    }

    POKE(0x2C5, 26);    //Sets bitmap color to yellow
}

//------------------------------ enablePMGraphics ------------------------------
// Purpose: Turning on and initializing Player Missile Graphics with the correct
//          addresses
// Parameters: None
// Preconditions: None
// Postconditions: player and missile base address will be intialized
void enablePMGraphics() {
    POKE(0x22F, 62);                    //Enable Player-Missile DMA single line
    PMBaseAddress = 0x2800;             //the player-missile base address
    POKE(0xD407, PMBaseAddress);        //Store Player-Missile base address in base register
    POKE(0xD01D, 3);                    //Enable Player-Missile DMA

    playerAddress = (PMBaseAddress * 256) + 1024;
    missileAddress = (PMBaseAddress * 256) + 768;

    //Clear up default built-in characters in Player's address
    for (i = 0; i <= 512; i++) {
        POKE(playerAddress + i, 0);

        //Clear up built in characters in Missile's address
        if (i <= 256)
        {
            POKE(missileAddress + i, 0);
        }
    }
}

//------------------------------ setUpTankDisplay ------------------------------
// Purpose: Setting up tank displays for both players
// Parameters: None
// Preconditions: None
// Postconditions: Tank for Player 1 will be created and set to point North East
//                 behind sprite. Tank for Player 2 will be created and set to
//                 point South West behind sprite.
void setUpTankDisplay() {
    int counter = 0;

    //Set up player 0 tank
    POKE(horizontalRegister_P0, 57);
    POKE(colLumPM0, 202);

    for (i = 131; i < 131+8; i++) {
        POKE(playerAddress+i, tankPics[EAST][counter]);
        counter++;
    }
    counter = 0;

    //Set up player 1 tank
    POKE(horizontalRegister_P1, 190);
    POKE(colLumPM1, 40);

    for (i = 387; i < 395; i++) {
        POKE(playerAddress+i, tankPics[WEST][counter]);
        counter++;
    }
    counter = 0;
}

// direction is one of NORTH...
bool isOnLine(int direction) {
    return deltas[direction][1] * (p1_r - p0_r) == deltas[direction][0] * (p1_c - p0_c);
}

unsigned char rotateOrFire(int direction) {
    if (direction < 8) {
        // it's opposite direction is at direction + 8
        if (p1Direction != direction + 8) {
            if (direction <= p1Direction && p1Direction < direction + 8) {
                return RIGHT_TURN;
            } else {
                return LEFT_TURN;
            }
        }
    } else {
        // it's opposite is direction - 8
        if (p1Direction != direction - 8) {
            if (direction - 8 <= p1Direction && p1Direction < direction) {
                return LEFT_TURN;
            } else {
                return RIGHT_TURN;
            }
        }
    }
    return FIRE;
}

unsigned char attack() {
    // FORWARD, BACKWARD, LEFT TURN, RIGHT TURN, FIRE
    // unsigned char moves[5] = {0x01, 0x02, 0x04, 0x08, 0x10};

    // Goal 1: Let's map out the position of the AI tank in reference to p0 [DONE]
    // Goal 2: Implement all the directions. There are 12 more but should go by pretty fast
    // Goal 2.5: clean up the code lots of spegetti going on rn.
    // Goal 3: Move the rotating to outside of each direction specific code. 
    // Goal 4: add the moving to a different spot logic as well if you can't reach the player from here.

    // Here are the design decisions that I have made:
    // Using the normal coordinate system
    // N-E-S-W
    // I - includes N discludes E
    // II - includes E disculdes S
    // III - includes S disculdes W
    // IV - includes W discludes N

    // p0_r, p0_c, p1_r, p1_c
    if (p0_r > p1_r && p0_c <= p1_c) {
        // AI is in quadrant I of p0
        // NORTH_EAST disculding EAST
        // there are 4 directions in this case that I have to consider
        // NORTH, NORTH_15, NORTH_EAST, NORTH_60 = [0, 3]
        for (i = NORTH; i < NORTH_EAST; i++) {
            if (isOnLine(i)) {
                return rotateOrFire(i);
            }
        }
        return 0x00; 
    } else if (p0_r <= p1_r && p0_c < p1_c) {
        // AI is in quadrant II of p0
        // SOUTH_EAST discluding SOUTH
        for (i = EAST; i < SOUTH; i++) {
            if (isOnLine(i)) {
                return rotateOrFire(i);
            }
        }
        return 0x00;
    } else if (p0_r < p1_r && p0_c >= p1_c) {
        // AI is in quadrant III of p0
        // SOUTH_WEST disculding WEST
        for (i = SOUTH; i < WEST; i++) {
            if (isOnLine(i)) {
                return rotateOrFire(i);
            }
        }
        return 0x00;
    } else if (p0_r >= p1_r && p0_c > p1_c) {
        // AI is in quadrant IV of p0
        // NORTH_WEST disculding NORTH
        for (i = WEST; i < WEST + 4; i++) {
            if (isOnLine(i)) {
                return rotateOrFire(i);
            }
        }
        return 0x00;
    }
    return 0x00;
}

unsigned char hide() {

    return 0x08;
}

unsigned char getAIPlayersNextMove() {
    // let's start with the basics let's just move the AI Player to always be at the same row as the 
    // other player. 
    // we can do that by calculating the difference between the two
    // column positions
    // p0_c and p1_c
    // FORWARD, BACKWARD, LEFT TURN, RIGHT TURN, FIRE
    // unsigned char moves[5] = {0x01, 0x02, 0x04, 0x08, 0x10};

    
    while (k < 50) {
        k++;
        return FORWARD;
    }

    if (p0Score <= p1Score) {
        // we are ahead or tied try to make it hard for the opponent to score
        // use variables like p0Direction to make it difficult for p0 to score on us
        return attack();
    } else {
        // keep the game close and try to score.
        // p1Direction will be helpful here to understand what way I have to point to try and score. 
        return attack();
    }
}

//------------------------------ movePlayers ------------------------------
// Purpose: Do actions based on player's inputs such as moving and firing.
// Parameters: None
// Preconditions: None
// Postconditions: Both tank will do actions based on the user's inputs.
void movePlayers(){
    //joystick code
    unsigned char player0move = joy_read(JOY_1);
    // unsigned char player1move = joy_read(JOY_2);
    unsigned char player1move = getAIPlayersNextMove();
    p0LastMove = player0move;
    p1LastMove = player1move;

    //moving player 1, only if they are not hit
    if(JOY_BTN_1(player0move) && p0FireAvailable == true && !p0IsHit) {fire(0); p0Fired = true;}
    else if(JOY_UP(player0move) && !p0IsHit) {
        if (!(p0Direction % 2) || ((p0Direction % 2) && p0FirstDiag == true)) moveForward(0);
        else p0FirstDiag = true;
    }
    else if(JOY_DOWN(player0move) && !p0IsHit) {
        if(!(p0Direction % 2) || ((p0Direction % 2) && p0FirstDiag == true)) moveBackward(0);
        else p0FirstDiag = true;
    }
    else if(JOY_LEFT(player0move) || JOY_RIGHT(player0move) && !p0IsHit) turnplayer(player0move, 0);

    //moving player 2, only if they are not hit
    if(JOY_BTN_1(player1move) && p1FireAvailable == true && !p1IsHit) {fire(1); p1Fired = true;}
    else if(JOY_UP(player1move) && !p1IsHit) {
        if (!(p1Direction % 2) || ((p1Direction % 2) && p1FirstDiag == true)) moveForward(1);
        else p1FirstDiag = true;
    }
    else if(JOY_DOWN(player1move) && !p1IsHit) {
        if(!(p1Direction % 2) || ((p1Direction % 2) && p1FirstDiag == true)) moveBackward(1);
        else p1FirstDiag = true;
    }
    else if(JOY_LEFT(player1move) || JOY_RIGHT(player1move) && !p1IsHit) turnplayer(player1move, 1);
}

//------------------------------ turnPlayer ------------------------------
// Purpose: Changes the direction of the specified player's tank based on joystick input.
//          This function handles tank direction changes, taking into account joystick input
//          and special cases for smooth and intuitive tank movement.
// Parameters:
//   turn - The joystick input indicating the desired direction change.
//   player - The player identifier (0 or 1) indicating which tank's direction to change.
// Preconditions: The player's current direction and joystick input must be correctly set.
// Postconditions: The player's tank direction is updated according to the joystick input.
void turnplayer(unsigned char turn, int player){
    //for player 1
    if (player == 0) {
        //handling edge cases
        if (p0Direction == WEST_60 && JOY_RIGHT(turn)) {
            p0Direction = NORTH;
        } else if (p0Direction == NORTH && JOY_LEFT(turn)) {
            p0Direction = WEST_60;
        }

        //if the joystick is left,
        else if(JOY_LEFT(turn)){
            p0Direction = p0Direction - 1;
        }
        
        //if the joystick is right
        else if(JOY_RIGHT(turn)){
            p0Direction = p0Direction + 1;
        }

        updateplayerDir(0);
    }
    
    //for player 2
    else if (player == 1) {
        if (p1Direction == WEST_60 && JOY_RIGHT(turn)) {
            p1Direction = NORTH;
        }
        else if (p1Direction == NORTH && JOY_LEFT(turn)) {
            p1Direction = WEST_60;
        }
        
        //if the joystick is left,
        else if (JOY_LEFT(turn)) {
            p1Direction = p1Direction - 1;
        }

        //if the joystick is right
        else if(JOY_RIGHT(turn)){
            p1Direction = p1Direction + 1;
        }

        updateplayerDir(1);
    }
}

//------------------------------ updatePlayerDir ------------------------------
// Purpose: Updates the visual representation of the specified player's tank.
//          This function refreshes the display of the tank sprite based on its
//          current direction, ensuring smooth animation.
// Parameters:
//   player - The player identifier (0 or 1) indicating which tank's sprite to update.
// Preconditions: The player's current direction, position, and tank sprite array (tankPics)
//                must be correctly set.
// Postconditions: The tank sprite on the screen reflects the updated direction.
void updateplayerDir(int player){
    //updating player 1
    if (player == 0) {
        if (p0Direction == SOUTH_WEST || p0Direction == EAST_SOUTH) {
            int counter = 7;
            for (i = p0VerticalLocation; i < p0VerticalLocation + 8; i++) {
                POKE(playerAddress + i, tankPics[p0Direction][counter]);
                counter--;
            }
        } else {
            int counter = 0;
            for (i = p0VerticalLocation; i < p0VerticalLocation + 8; i++) {
                POKE(playerAddress + i, tankPics[p0Direction][counter]);
                counter++;
            }
        }
    }

    //updating player 2
    else if (player == 1) {
        if (p1Direction == SOUTH_WEST || p1Direction == EAST_SOUTH) {
            int counter = 7;
            for (i = p1VerticalLocation; i < p1VerticalLocation + 8; i++) {
                POKE(playerAddress + i, tankPics[p1Direction][counter]);
                counter--;
            }
        } else {
            int counter = 0;
            for (i = p1VerticalLocation; i < p1VerticalLocation + 8; i++) {
                POKE(playerAddress + i, tankPics[p1Direction][counter]);
                counter++;
            }
        }
    }
}

//------------------------------ moveForward ------------------------------
// Purpose: Move the tank forward in the specified direction.
//          This function updates the tank's position based on its current
//          direction and ensures smooth movement, including diagonal cases.
// Parameters:
//   tank - The tank identifier (0 or 1) indicating which tank to move.
// Preconditions: The tank's direction, position, and related variables must be set.
// Postconditions: The tank's position is updated to move it forward in the specified direction.
void moveForward(int tank){
    //moving forward tank 1--------------------------------
    if (tank == 0) {
        p0FirstDiag = false;

        //movement for north
        if (p0Direction == NORTH) {
            POKE(playerAddress+(p0VerticalLocation+7), 0);
            p0VerticalLocation--;
            p0_r--;
        }

        //movement for south
        if (p0Direction == SOUTH) {
            POKE(playerAddress+p0VerticalLocation, 0);
            p0VerticalLocation++;
            p0_r++;
        }

        //movement north-ish cases
        if (p0Direction == NORTH_15 || p0Direction == NORTH_60 || p0Direction == NORTH_EAST || p0Direction == WEST_15 || p0Direction == WEST_NORTH || p0Direction == WEST_60) {
            int x = 0;
            int y = 0;

            //X ifs
            if (p0Direction == NORTH_EAST || p0Direction == WEST_NORTH || p0Direction == NORTH_15 || p0Direction == WEST_60) x = 1;
            if (p0Direction == NORTH_60 || p0Direction == WEST_15) x = 2;

            //Y ifs
            if (p0Direction == NORTH_EAST || p0Direction == WEST_NORTH || p0Direction == NORTH_60 || p0Direction == WEST_15) y = 1;
            if (p0Direction == NORTH_15 || p0Direction == WEST_60) y = 2;
            if (p0Direction < 4) {
                p0HorizontalLocation = p0HorizontalLocation + x;
                p0_c = p0_c + x;
            } else {
                p0HorizontalLocation = p0HorizontalLocation - x;
                p0_c = p0_c - x;
            }
            POKE(playerAddress+(p0VerticalLocation +7), 0);
            POKE(playerAddress+(p0VerticalLocation +6), 0);
            p0VerticalLocation = p0VerticalLocation - y;
            p0_r = p0_r - y;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }

        //movement south-ish cases
        if (p0Direction == SOUTH_15 || p0Direction == SOUTH_60 || p0Direction == SOUTH_WEST || p0Direction == EAST_15 || p0Direction == EAST_SOUTH || p0Direction == EAST_60) {
            int x = 0;
            int y = 0;

            //X ifs
            if (p0Direction == SOUTH_WEST || p0Direction == EAST_SOUTH || p0Direction == SOUTH_15 || p0Direction == EAST_60) x = 1;
            if (p0Direction == SOUTH_60 || p0Direction == EAST_15) x = 2;

            //Y ifs
            if (p0Direction == SOUTH_WEST || p0Direction == EAST_SOUTH || p0Direction == SOUTH_60 || p0Direction == EAST_15) y = 1;
            if (p0Direction == SOUTH_15 || p0Direction == EAST_60) y = 2;
            if (p0Direction < 8) {
                p0HorizontalLocation = p0HorizontalLocation + x;
                p0_c = p0_c + x;
            }
            else {
                p0HorizontalLocation = p0HorizontalLocation - x;
                p0_c = p0_c - x;
            }
            POKE(playerAddress+(p0VerticalLocation), 0);
            POKE(playerAddress+(p0VerticalLocation +1), 0);
            p0VerticalLocation = p0VerticalLocation + y;
            p0_r = p0_r + y;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }

        //movement west
        if (p0Direction == WEST) {
            p0HorizontalLocation--;
            p0_c--;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }

        //movement east
        if (p0Direction == EAST) {
            p0HorizontalLocation++;
            p0_c++;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }

        updateplayerDir(0);
    }

    //moving forward tank 2------------------------------
    else if (tank == 1) {
        p1FirstDiag = false;

        //movement for north
        if (p1Direction == NORTH) {
            POKE(playerAddress+(p1VerticalLocation+7), 0);
            p1VerticalLocation--;
            p1_r--;
        }

        //movement for south
        if (p1Direction == SOUTH) {
            POKE(playerAddress+p1VerticalLocation, 0);
            p1VerticalLocation++;
            p1_r++;
        }

        //movement north-ish cases
        if (p1Direction == NORTH_15 || p1Direction == NORTH_60 || p1Direction == NORTH_EAST || p1Direction == WEST_15 || p1Direction == WEST_NORTH || p1Direction == WEST_60) {
            int x = 0;
            int y = 0;

            //X ifs
            if (p1Direction == NORTH_EAST || p1Direction == WEST_NORTH || p1Direction == NORTH_15 || p1Direction == WEST_60) x = 1;
            if (p1Direction == NORTH_60 || p1Direction == WEST_15) x = 2;

            //Y ifs
            if (p1Direction == NORTH_EAST || p1Direction == WEST_NORTH || p1Direction == NORTH_60 || p1Direction == WEST_15) y = 1;
            if (p1Direction == NORTH_15 || p1Direction == WEST_60) y = 2;
            if (p1Direction < 4) {
                p1HorizontalLocation = p1HorizontalLocation + x;
                p1_c = p1_c + x;
            } else {
                p1HorizontalLocation = p1HorizontalLocation - x;
                p1_c = p1_c - x;
            }

            POKE(playerAddress+(p1VerticalLocation +7), 0);
            POKE(playerAddress+(p1VerticalLocation +6), 0);
            p1VerticalLocation = p1VerticalLocation - y;
            p1_r = p1_r - y;
            POKE(horizontalRegister_P1, p1HorizontalLocation);
        }

        //movement south-ish cases
        if (p1Direction == SOUTH_15 || p1Direction == SOUTH_60 || p1Direction == SOUTH_WEST || p1Direction == EAST_15 || p1Direction == EAST_SOUTH || p1Direction == EAST_60) {
            int x = 0;
            int y = 0;

            //X ifs
            if (p1Direction == SOUTH_WEST || p1Direction == EAST_SOUTH || p1Direction == SOUTH_15 || p1Direction == EAST_60) x = 1;
            if (p1Direction == SOUTH_60 || p1Direction == EAST_15) x = 2;

            //Y ifs
            if (p1Direction == SOUTH_WEST || p1Direction == EAST_SOUTH || p1Direction == SOUTH_60 || p1Direction == EAST_15) y = 1;
            if (p1Direction == SOUTH_15 || p1Direction == EAST_60) y = 2;
            if (p1Direction < 8) {
                p1HorizontalLocation = p1HorizontalLocation + x;
                p1_c = p1_c + x;
            } else {
                p1HorizontalLocation = p1HorizontalLocation - x;
                p1_c = p1_c - x;
            }
            POKE(playerAddress+(p1VerticalLocation), 0);
            POKE(playerAddress+(p1VerticalLocation +1), 0);
            p1VerticalLocation = p1VerticalLocation + y;
            p1_r = p1_r + y;
            POKE(horizontalRegister_P1, p1HorizontalLocation);
        }

        //movement west
        if (p1Direction == WEST) {
            p1HorizontalLocation--;
            p1_c--;
            POKE(horizontalRegister_P1, p1HorizontalLocation);
        }

        //movement east
        if (p1Direction == EAST) {
            p1HorizontalLocation++;
            p1_c++;
            POKE(horizontalRegister_P1, p1HorizontalLocation);
        }

        updateplayerDir(1);
    }
}

//------------------------------ moveBackward ------------------------------
// Purpose: Move the tank backward in the specified direction.
//          This function updates the tank's position based on its current
//          direction and ensures smooth movement, including diagonal cases.
// Parameters:
//   tank - The tank identifier (0 or 1) indicating which tank to move.
// Preconditions: The tank's direction, position, and related variables must be set.
// Postconditions: The tank's position is updated to move it backward in the specified direction.
void moveBackward(int tank) {
    //moving backward tank 1-------------------------------
    if (tank == 0) {
        p0FirstDiag = false;

        //movement for north
        if (p0Direction == NORTH) {
            POKE(playerAddress+p0VerticalLocation, 0);
            p0VerticalLocation++;
            p0_r++;
        }

        //movement for south
        if(p0Direction == SOUTH){
            POKE(playerAddress+(p0VerticalLocation+7), 0);
            p0VerticalLocation--;
            p0_r--;
        }

        //movement north-ish cases
        if(p0Direction == NORTH_15 || p0Direction == NORTH_60 || p0Direction == NORTH_EAST || p0Direction == WEST_15 || p0Direction == WEST_NORTH || p0Direction == WEST_60){
            int x = 0;
            int y = 0;

            //X ifs
            if (p0Direction == NORTH_EAST || p0Direction == WEST_NORTH || p0Direction == NORTH_15 || p0Direction == WEST_60) x = 1;
            if (p0Direction == NORTH_60 || p0Direction == WEST_15) x = 2;

            //Y ifs
            if (p0Direction == NORTH_EAST || p0Direction == WEST_NORTH || p0Direction == NORTH_60 || p0Direction == WEST_15) y = 1;
            if (p0Direction == NORTH_15 || p0Direction == WEST_60) y = 2;
            if (p0Direction > 4) {
                p0HorizontalLocation = p0HorizontalLocation + x;
                p0_c = p0_c + x;
            } else {
                p0HorizontalLocation = p0HorizontalLocation - x;
                p0_c = p0_c - x;
            }
            POKE(playerAddress+(p0VerticalLocation), 0);
            POKE(playerAddress+(p0VerticalLocation + 1), 0);
            p0VerticalLocation = p0VerticalLocation + y;
            p0_r = p0_r + y;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }
        
        //movement south-ish cases
        if (p0Direction == SOUTH_15 || p0Direction == SOUTH_60 || p0Direction == SOUTH_WEST || p0Direction == EAST_15 || p0Direction == EAST_SOUTH || p0Direction == EAST_60){
            int x = 0;
            int y = 0;

            //X ifs
            if (p0Direction == SOUTH_WEST || p0Direction == EAST_SOUTH || p0Direction == SOUTH_15 || p0Direction == EAST_60) x = 1;
            if (p0Direction == SOUTH_60 || p0Direction == EAST_15) x = 2;

            //Y ifs
            if (p0Direction == SOUTH_WEST || p0Direction == EAST_SOUTH || p0Direction == SOUTH_60 || p0Direction == EAST_15) y = 1;
            if (p0Direction == SOUTH_15 || p0Direction == EAST_60) y = 2;
            if (p0Direction < 8) {
                p0HorizontalLocation = p0HorizontalLocation - x;
                p0_c = p0_c - x;
            } else {
                p0HorizontalLocation = p0HorizontalLocation + x;
                p0_c = p0_c + x;
            }
            POKE(playerAddress+(p0VerticalLocation + 7), 0);
            POKE(playerAddress+(p0VerticalLocation +6), 0);
            p0VerticalLocation = p0VerticalLocation - y;
            p0_r = p0_r - y;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }

        //movement west
        if (p0Direction == WEST){
            p0HorizontalLocation++;
            p0_c++;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }

        //movement east
        if (p0Direction == EAST){
            p0HorizontalLocation--;
            p0_c--;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }

        updateplayerDir(0);
    }


    //moving backward tank 2-------------------------------
    if (tank == 1) {
        p1FirstDiag = false;

        //movement for north
        if (p1Direction == NORTH) {
            POKE(playerAddress+p1VerticalLocation, 0);
            p1VerticalLocation++;
            p1_r++;
        }

        //movement for south
        if (p1Direction == SOUTH) {
            POKE(playerAddress+(p1VerticalLocation+7), 0);
            p1VerticalLocation--;
            p1_r--;
        }

        //movement north-ish cases
        if (p1Direction == NORTH_15 || p1Direction == NORTH_60 || p1Direction == NORTH_EAST || p1Direction == WEST_15 || p1Direction == WEST_NORTH || p1Direction == WEST_60) {
            int x = 0;
            int y = 0;

            //X ifs
            if (p1Direction == NORTH_EAST || p1Direction == WEST_NORTH || p1Direction == NORTH_15 || p1Direction == WEST_60) x = 1;
            if (p1Direction == NORTH_60 || p1Direction == WEST_15) x = 2;

            //Y ifs
            if (p1Direction == NORTH_EAST || p1Direction == WEST_NORTH || p1Direction == NORTH_60 || p1Direction == WEST_15) y = 1;
            if (p1Direction == NORTH_15 || p1Direction == WEST_60) y = 2;
            if (p1Direction > 4) {
                p1HorizontalLocation = p1HorizontalLocation + x;
                p1_c = p1_c + x;
            } else {
                p1HorizontalLocation = p1HorizontalLocation - x;
                p1_c = p1_c - x;
            }
            POKE(playerAddress+(p1VerticalLocation), 0);
            POKE(playerAddress+(p1VerticalLocation + 1), 0);
            p1VerticalLocation = p1VerticalLocation + y;
            p1_r = p1_r + y;
            POKE(horizontalRegister_P1, p1HorizontalLocation);
        }

        //movement south-ish cases
        if(p1Direction == SOUTH_15 || p1Direction == SOUTH_60 || p1Direction == SOUTH_WEST || p1Direction == EAST_15 || p1Direction == EAST_SOUTH || p1Direction == EAST_60){
            int x = 0;
            int y = 0;

            //X ifs
            if(p1Direction == SOUTH_WEST || p1Direction == EAST_SOUTH || p1Direction == SOUTH_15 || p1Direction == EAST_60) x = 1;
            if(p1Direction == SOUTH_60 || p1Direction == EAST_15) x = 2;

            //Y ifs
            if(p1Direction == SOUTH_WEST || p1Direction == EAST_SOUTH || p1Direction == SOUTH_60 || p1Direction == EAST_15) y = 1;
            if(p1Direction == SOUTH_15 || p1Direction == EAST_60) y = 2;
            if(p1Direction < 8) {
                p1HorizontalLocation = p1HorizontalLocation - x;
                p1_c = p1_c - x;
            }
            else {
                p1HorizontalLocation = p1HorizontalLocation + x;
                p1_c = p1_c + x;
            }
            POKE(playerAddress+(p1VerticalLocation + 7), 0);
            POKE(playerAddress+(p1VerticalLocation +6), 0);
            p1VerticalLocation = p1VerticalLocation - y;
            p1_r = p1_r - y;
            POKE(horizontalRegister_P1, p1HorizontalLocation);
        }

        //movement west
        if(p1Direction == WEST){
            p1HorizontalLocation++;
            p1_c++;
            POKE(horizontalRegister_P1, p1HorizontalLocation);
        }
        //movement east
        if(p1Direction == EAST){
            p1HorizontalLocation--;
            p1_c--;
            POKE(horizontalRegister_P1, p1HorizontalLocation);
        }

        updateplayerDir(1);
    }
}

//-------------------------------check borders------------------------------
//purpose: during a collision, check to see if the tank is going to spin
//         out-of-bounds, and correct it by sending it to the opposing
//         side of the screen
//parameters: none
//preconditions: tank location must be set
//post conditions: tank location may be changed
//--------------------------------------------------------------------------
void checkBorders() {
    //variables to make sure that they don't just jump back across the screen (doesn't trigger as move up right after move down)
    bool movedLeft0 = false;
    bool movedLeft1 = false;
    bool movedup0 = false;
    bool movedup1 = false;

    //if they are too far to the left
    if(p0HorizontalLocation <= 50 && p0IsHit){
        movedLeft0 = true;
        p0HorizontalLocation = 195;
        // p0_c = 144;
        POKE(horizontalRegister_P0, p0HorizontalLocation);
    }

    if(p1HorizontalLocation <= 50 && p1IsHit){
        movedLeft1 = true;
        p1HorizontalLocation = 195;
        // p1_c = 144;
        POKE(horizontalRegister_P1, p1HorizontalLocation);
    }

    //if they're too far to the right
    if(p0HorizontalLocation >= 195 && p0IsHit && !movedLeft0){
        p0HorizontalLocation = 50;

        POKE(horizontalRegister_P0, p0HorizontalLocation);
    }

    if(p1HorizontalLocation >= 195 && p1IsHit && !movedLeft1){
        p1HorizontalLocation = 50;
        POKE(horizontalRegister_P1, p1HorizontalLocation);
    }

    //if they're too far up
    if(p0VerticalLocation <= 57 && p0IsHit){
        //delete player from upper location
        for (i = 0; i < 8; i++) {
            POKE(playerAddress+(p0VerticalLocation + i), 0);
        }

        //add them to lower location
        p0VerticalLocation = 207;
        movedup0 = true;
    }
    if(p1VerticalLocation <= 312 && p1IsHit){
        //delete player from upper location
        for (i = 0; i < 8; i++) {
            POKE(playerAddress+(p1VerticalLocation + i), 0);
        }

        POKE(playerAddress+(p1VerticalLocation + 7), 0);
        //add them to lower location
        p1VerticalLocation = 464;
        movedup1 = true;
    }

    //if they're too far down
    if(p0VerticalLocation >= 207 && p0IsHit && !movedup0){
        //delete player at lower location
        for (i = 0; i < 8; i++) {
            POKE(playerAddress+(p0VerticalLocation + i), 0);
        }

        //add them to upper location
        p0VerticalLocation = 57;
    }
    if(p1VerticalLocation >= 464 && p1IsHit && !movedup1){
        //delete player at lower location
        for (i = 0; i < 8; i++) {
            POKE(playerAddress+(p1VerticalLocation + i), 0);
        }

        //add them to upper location
        p1VerticalLocation = 312;
    }
}

//-----------------------spin tank------------------------
//purpose: spin the tank if it is hit, but in different
//         directions depending on what direction it
//         is hit from
//parameters: tank, either 0 for tank 1 or 1 for tank 2
//preconditions: tank direction, location, and player hit
//               direction must be set (set in collision)
//post conditions: tank direction and location are changed
//--------------------------------------------------------
void spinTank(int tank){

    //if player 1 is hit
    if(tank == 0){
        //if the tank is hit from the north
        if(p0HitDir == NORTH || p0HitDir == NORTH_EAST || p0HitDir == EAST_60 || p0HitDir == NORTH_15){
            //move left and spin
            p0HorizontalLocation++;
            p0_c++;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
            if(p0Direction == WEST_NORTH || p0Direction == WEST_60) p0Direction = NORTH;
            else p0Direction = p0Direction + 2;
            updateplayerDir(0);
        }
        //if the tank is hit from the south
        if(p0HitDir == SOUTH || p0HitDir == SOUTH_15 || p0HitDir == SOUTH_WEST || p0HitDir == WEST_60){
            //move right and spin
            p0HorizontalLocation--;
            p0_c--;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
            if(p0Direction == NORTH_15 || p0Direction == NORTH) p0Direction = WEST_60;
            else p0Direction = p0Direction - 2;
            updateplayerDir(0);
        }
        //if the tank is hit from the west
        if(p0HitDir == WEST || p0HitDir == WEST_15 || p0HitDir == WEST_NORTH || p0HitDir == SOUTH_60){
            //move down and spin
            if(p0Direction == NORTH_15 || p0Direction == NORTH) p0Direction = WEST_60;
            else p0Direction = p0Direction - 2;
            POKE(playerAddress+p0VerticalLocation, 0);
            p0VerticalLocation++;
            p0_r++;
            updateplayerDir(0);
        }
        if(p0HitDir == EAST || p0HitDir == EAST_15 || p0HitDir == EAST_SOUTH || p0HitDir == NORTH_60){
            //move up and spin
            if(p0Direction == NORTH_15 || p0Direction == NORTH) p0Direction = WEST_60;
            else p0Direction = p0Direction - 2;
            POKE(playerAddress+p0VerticalLocation+7, 0);
            p0VerticalLocation--;
            p0_r--;
            updateplayerDir(0);
        }
        //lower the time that the tank is stuck in "hit" state
        hitTime[0] = hitTime[0] - 1;
        if(hitTime[0] == 0) p0IsHit = false;
    }

    //if player 2 is hit
    if(tank == 1){
        //if the tank is hit from the north
        if(p1HitDir == NORTH || p1HitDir == NORTH_15 || p1HitDir == NORTH_EAST || p1HitDir == EAST_60){
            //move left and spin
            p1HorizontalLocation++;
            p1_c++;
            POKE(horizontalRegister_P1, p1HorizontalLocation);
            if(p1Direction == WEST_NORTH || p1Direction == WEST_60) p1Direction = NORTH;
            else p1Direction = p1Direction + 2;
            updateplayerDir(1);
        }
        //if the tank is hit from the south
        if(p1HitDir == SOUTH || p1HitDir == SOUTH_15 || p1HitDir == SOUTH_WEST || p1HitDir == WEST_60){
            //move right and spin
            p1HorizontalLocation--;
            p1_c--;
            POKE(horizontalRegister_P1, p1HorizontalLocation);
            if(p1Direction == NORTH_15 || p1Direction == NORTH) p1Direction = WEST_60;
            else p1Direction = p1Direction - 2;
            updateplayerDir(1);
        }
        //if the tank is hit from the west
        if(p1HitDir == WEST || p1HitDir == WEST_15 || p1HitDir == WEST_NORTH || p1HitDir == SOUTH_60){
            //move down and spin
            POKE(playerAddress+p1VerticalLocation, 0);
            p1VerticalLocation++;
            p1_r++;
            if(p1Direction == NORTH_15 || p1Direction == NORTH) p1Direction = WEST_60;
            else p1Direction = p1Direction - 2;
            updateplayerDir(1);
        }
        //if the tank is hit from the east
        if(p1HitDir == EAST || p1HitDir == EAST_15 || p1HitDir == EAST_SOUTH || p1HitDir == NORTH_60){
            //move up and spin
            POKE(playerAddress+p1VerticalLocation+7, 0);
            p1VerticalLocation--;
            p1_r++;
            if(p1Direction == NORTH_15 || p1Direction == NORTH) p1Direction = WEST_60;
            else p1Direction = p1Direction - 2;
            updateplayerDir(1);
        }
        //lower the time that the tank is stuck in the "hit" state
        hitTime[1] = hitTime[1] - 1;
        if(hitTime[1] == 0) p1IsHit = false;
    }
    //check to see if a tank hit a border wall
    checkBorders();
}

//------------------------------ checkCollision ------------------------------
// Purpose: Move the tank backward in the specified direction.
//          This function updates the tank's position based on its current
//          direction and ensures smooth movement, including diagonal cases.
// Parameters: None
// Preconditions: None
// Postconditions: Reading into collision registers to check if there are any
//                 collisions. If there are collisions do the respective actions
//                 and ensure to clear collision register after execution (writing
//                 0's to the collision registers)
void checkCollision(){
    //checking for player 1 to playfield collision 
    if(PEEK(P1PF) != 0x0000){
        if(JOY_UP(p1history)){
            moveBackward(1);
            moveBackward(1);
            moveBackward(1);
            moveBackward(1);
        }
        else if(JOY_DOWN(p1history)){
            moveForward(1);
            moveForward(1);
            moveForward(1);
            moveForward(1);
        }
    }

    //checking for player 0 to playfield collision
    if(PEEK(P0PF) != 0x0000){
        if(JOY_UP(p0history)){
            moveBackward(0);
            moveBackward(0);
            moveBackward(0);
            moveBackward(0);
        }
        if(JOY_DOWN(p0history)){
            moveForward(0);
            moveForward(0);
            moveForward(0);
            moveForward(0);
        }
    }
    //checking for missile (player 1) to playfield collision
    if(PEEK(M1PF) != 0x0000){
        m1exists = false;
        POKE(missileAddress+m1LastVerticalLocation, 0);
    }
    //checking for missile (player 0) to playfield collision
    if(PEEK(M0PF) != 0x0000){
        m0exists = false;
        POKE(missileAddress+m0LastVerticalLocation, 0);
    }
    //checking for missile1 to player collision
    if(PEEK(M1P) != 0x0000){
        p0HitDir = m1direction;
        m1exists = false;
        POKE(missileAddress+m1LastVerticalLocation, 0);
        p1Score += 1;
        updatePlayerScore();
        p0IsHit = true;
        hitTime[0] = 12;
        j = 0;
    }
    //checking for missile0 to player collision
    if(PEEK(M0P) != 0x0000){
        p1HitDir = m0direction;
        m0exists = false;
        POKE(missileAddress+m0LastVerticalLocation, 0);
        p0Score += 1;
        updatePlayerScore();
        p1IsHit = true;
        hitTime[1] = 12;
        j = 0;
    }

    POKE(HITCLR, 1); // Clear ALL of the Collision Registers
}

//------------------------------ fire ------------------------------
// Purpose: Launches a projectile from the specified tank.
//          This function sets up and fires a projectile in the direction of the tank.
// Parameters:
//   tank - The tank identifier (0 or 1) indicating which tank is firing.
// Preconditions: The tank's direction, position, missile existence, and fire availability
//                must be appropriately configured.
// Postconditions: A projectile is launched from the tank, and its existence is marked
//                 until a collision occurs. Fire availability is temporarily disabled to
//                 prevent rapid firing.
void fire(int tank) {
    if (tank == 0)
    {
        POKE(missileAddress+m0LastVerticalLocation, 0);
        missileLocationHelper(p0Direction, p0HorizontalLocation, p0VerticalLocation, tank);
        m0exists = true; //missile exists until colliding
        p0FireAvailable = false; //prevents missile spamming, starts a counter in the main loop
    }
    else if (tank == 1)
    {
        POKE(missileAddress+m1LastVerticalLocation, 0);
        missileLocationHelper(p1Direction, p1HorizontalLocation, p1VerticalLocation, tank);
        m1exists = true; //missile exists until colliding
        p1FireAvailable = false; //prevents missile spamming, starts a counter in the main loop
    }
}

//------------------------------ missileLocationHelper ------------------------------
// Purpose: This function determines the target location for a missile launch. It tracks
//          the current position of the tank and sets up the missile's position to be at
//          the tip of the tank's barrel.
// 
// Parameters:
//   tankDirection - The direction in which the tank is facing.
//   pHorizontalLocation - The horizontal position of the tank being passed in.
//   pVerticalLocation - The vertical position of the tank being passed in.
//   tank - The tank identifier (0 or 1).
//
// Preconditions: None
// Postconditions: The missile's launch position is set according to the tank's
//                 orientation.
void missileLocationHelper(unsigned int tankDirection, int pHorizontalLocation, int pVerticalLocation, int tank) {
    int mdirection;
    int mLastHorizontalLocation = 0;
    int mLastVerticalLocation = 0;

    if (tankDirection == NORTH) {
        mLastHorizontalLocation = pHorizontalLocation+4;
        mLastVerticalLocation = pVerticalLocation;
        mdirection = NORTH;
    } else if (tankDirection == NORTH_15) {
        mLastHorizontalLocation = pHorizontalLocation+5;
        mLastVerticalLocation = pVerticalLocation;
        mdirection = NORTH_15;
    } else if (tankDirection == NORTH_EAST) {
        mLastHorizontalLocation = pHorizontalLocation+7;
        mLastVerticalLocation = pVerticalLocation;
        mdirection = NORTH_EAST;
    } else if (tankDirection == NORTH_60) {
        mLastHorizontalLocation = pHorizontalLocation+7;
        mLastVerticalLocation = pVerticalLocation+2;
        mdirection = NORTH_60;
    } else if (tankDirection == EAST) {
        mLastHorizontalLocation = pHorizontalLocation+7;
        mLastVerticalLocation = pVerticalLocation+4;
        mdirection = EAST;
    } else if (tankDirection == EAST_15) {
        mLastHorizontalLocation = pHorizontalLocation+7;
        mLastVerticalLocation = pVerticalLocation+5;
        mdirection = EAST_15;
    } else if (tankDirection == EAST_SOUTH) {
        mLastHorizontalLocation = pHorizontalLocation+7;
        mLastVerticalLocation = pVerticalLocation+7;
        mdirection = EAST_SOUTH;
    } else if (tankDirection == EAST_60) {
        mLastHorizontalLocation = pHorizontalLocation+5;
        mLastVerticalLocation = pVerticalLocation+7;
        mdirection = EAST_60;
    } else if (tankDirection == SOUTH) {
        mLastHorizontalLocation = pHorizontalLocation+4;
        mLastVerticalLocation = pVerticalLocation+7;
        mdirection = SOUTH;
    } else if (tankDirection == SOUTH_15) {
        mLastHorizontalLocation = pHorizontalLocation+2;
        mLastVerticalLocation = pVerticalLocation+7;
        mdirection = SOUTH_15;
    } else if (tankDirection == SOUTH_WEST) {
        mLastHorizontalLocation = pHorizontalLocation;
        mLastVerticalLocation = pVerticalLocation+7;
        mdirection = SOUTH_WEST;
    } else if (tankDirection == SOUTH_60) {
        mLastHorizontalLocation = pHorizontalLocation;
        mLastVerticalLocation = pVerticalLocation+5;
        mdirection = SOUTH_60;
    } else if (tankDirection == WEST) {
        mLastHorizontalLocation = pHorizontalLocation;
        mLastVerticalLocation = pVerticalLocation+4;
        mdirection = WEST;
    } else if (tankDirection == WEST_15) {
        mLastHorizontalLocation = pHorizontalLocation;
        mLastVerticalLocation = pVerticalLocation+2;
        mdirection = WEST_15;
    } else if (tankDirection == WEST_NORTH) {
        mLastHorizontalLocation = pHorizontalLocation;
        mLastVerticalLocation = pVerticalLocation;
        mdirection = WEST_NORTH;
    } else if (tankDirection == WEST_60) {
        mLastHorizontalLocation = pHorizontalLocation+2;
        mLastVerticalLocation = pVerticalLocation+2;
        mdirection = WEST_60;
    }

    //Update missile location based on which player is being passed in
    if (tank == 0) {
        m0LastHorizontalLocation = mLastHorizontalLocation;
        m0LastVerticalLocation = mLastVerticalLocation;
        m0direction = mdirection;
    } else if (tank == 1) {
        m1LastHorizontalLocation = mLastHorizontalLocation;
        m1LastVerticalLocation = mLastVerticalLocation - 256; //-256 because in RAM, the vertical position of player 1 ranges from 257 - 512
        m1direction = mdirection;
    }
}

//------------------------------ traverseMissile ------------------------------
// Purpose: Initiates and animates missile movement, advancing it until a collision
//          occurs. The function continuously updates the missile's position while
//          allowing for simultaneous tank movement.
//
// Parameters:
//   missileDirection - The direction in which the missile is fired.
//   mHorizontalLocation - The horizontal position of the missile.
//   mVerticalLocation - The vertical position of the missile.
//   tank - The tank identifier.
// Preconditions: None
// Postconditions: The missile animation progresses, considering tank movement.
void traverseMissile(unsigned int missileDirection, int mHorizontalLocation, int mVerticalLocation, int tank)
{
    POKE(missileAddress+mVerticalLocation, 0);

    if (missileDirection == NORTH)
    {
        mVerticalLocation--;
    }
    else if (missileDirection == NORTH_15)
    {
        mVerticalLocation -= 2;
        mHorizontalLocation++;
    }
    else if (missileDirection == NORTH_EAST)
    {
        mVerticalLocation--;
        mHorizontalLocation++;
    }
    else if (missileDirection == NORTH_60)
    {
        mVerticalLocation--;
        mHorizontalLocation += 2;
    }
    else if (missileDirection == EAST)
    {
        mHorizontalLocation++;
    }
    else if (missileDirection == EAST_15)
    {
        mVerticalLocation++;
        mHorizontalLocation += 2;
    }
    else if (missileDirection == EAST_SOUTH)
    {
        mVerticalLocation++;
        mHorizontalLocation++;
    }
    else if (missileDirection == EAST_60)
    {
        mVerticalLocation += 2;
        mHorizontalLocation++;
    }
    else if (missileDirection == SOUTH)
    {
        mVerticalLocation++;
    }
    else if (missileDirection == SOUTH_15)
    {
        mVerticalLocation += 2;
        mHorizontalLocation--;
    }
    else if (missileDirection == SOUTH_WEST)
    {
        mVerticalLocation++;
        mHorizontalLocation--;
    }
    else if (missileDirection == SOUTH_60)
    {
        mVerticalLocation++;
        mHorizontalLocation -= 2;
    }
    else if (missileDirection == WEST)
    {
        mHorizontalLocation--;
    }
    else if (missileDirection == WEST_15)
    {
        mVerticalLocation--;
        mHorizontalLocation -= 2;
    }
    else if (missileDirection == WEST_NORTH)
    {
        mVerticalLocation--;
        mHorizontalLocation--;
    }
    else if (missileDirection == WEST_60)
    {
        mVerticalLocation -= 2;
        mHorizontalLocation--;
    }


    if (tank == 0)
    {
        m0LastHorizontalLocation = mHorizontalLocation; //saving new location to global variables
        m0LastVerticalLocation = mVerticalLocation; //saving new location to global variables

        POKE(horizontalRegister_M0, mHorizontalLocation);

        if (m0LastVerticalLocation == m1LastVerticalLocation && m1exists == true)
        {
            POKE(missileAddress+mVerticalLocation, 10);
        }
        else
        {
            POKE(missileAddress+mVerticalLocation, 2);
        }
    }
    else if (tank == 1)
    {
        m1LastHorizontalLocation = mHorizontalLocation; //saving new location to global variables
        m1LastVerticalLocation = mVerticalLocation; //saving new location to global variables

        POKE(horizontalRegister_M1, mHorizontalLocation);

        if (m1LastVerticalLocation == m0LastVerticalLocation && m0exists == true)
        {
            POKE(missileAddress+mVerticalLocation, 10);
        }
        else
        {
            POKE(missileAddress+mVerticalLocation, 8);
        }
    }
}

