#ifndef TEST_APP_H
#define TEST_APP_H

static char PROGRAM_0[] = "10 HOME\r20 PRINT \"HACK THE PLANET\"\r30 PRINT \"WIZIO 2023\"\r";

static char PROGRAM_1[] =
    "0 LET H$ = \"0123456789ABCDEF\"\r"
    "1 HOME : VTAB 3 : HTAB 13\r"
    "2 PRINT H$\r"
    "3 FOR V = 1 TO 16 : PRINT\r"
    "4   HTAB 11 : A = PEEK (41) * 256\r"
    "5   LET A = A + PEEK(40)\r"
    "6   PRINT MID$(H$, V, 1);\r"
    "7   FOR H = 1 TO 16\r"
    "8     POKE A + H + 11, C : C = C + 1\r"
    "9 NEXT H, V\r"
    "LIST\r"
    "RUN\r";

static char PROGRAM_10[] =
    "1 GR:HOME:PRINT:FOR X=4 TO 35:COLOR=C:H=C>9:POKE 1616+128*U+X,176+K*(C-11*H)+H:VLIN 0,39 AT X:C=C+K:K=K=0:U=U=K:NEXT\r"
    "LIST\r"
    "RUN\r";

static char PROGRAM_11[] =
    "10 GR\r"
    "20 FOR X = 0 TO 39\r"
    "30 FOR Y = 0 TO 39\r"
    "40 COLOR = X+Y\r"
    "50 PLOT X, Y\r"
    "60 NEXT\r"
    "70 NEXT\r"
    "LIST\r"
    "LIST\r"
    "LIST\r"
    "RUN\r";

static char PROGRAM_20[] =
    "10 HGR:HCOLOR=3\r"
    "20 FOR R = 4 TO 90 STEP 4\r"
    "30 FOR I = 0 TO 6.28 STEP 0.1\r"
    "40 X = 140 + R * SIN(I)\r"
    "50 Y =  95 + R * COS(I)\r"
    "60 HPLOT X,Y\r"
    "70 NEXT\r"
    "80 NEXT\r"
    "LIST\r"
    "LIST\r"
    "LIST\r"
    "RUN\r";

static char PROGRAM_21[] =
    "10 HGR2\r"
    "20 FOR C = 0 TO 7\r"
    "30 FOR I = 0 TO 191 STEP 3\r"
    "40 HCOLOR = C\r"
    "50 HPLOT 0,I TO 0,I TO I,I TO I,0 TO 0,0\r"
    "60 NEXT\r"
    "70 NEXT\r"
    "80 GOTO 20\r"
    "RUN\r";

static char PROGRAM_SABOTAGE[] = "CALL -151\r1D00G\rN"; // run game
static char PROGRAM_MARIO[] = "CALL -151\r0803G\rN";    // run game

#define PROGRAM PROGRAM_1
//#define RUN_GAME

#endif