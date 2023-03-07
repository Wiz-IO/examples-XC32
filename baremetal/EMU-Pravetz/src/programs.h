#ifndef TEST_APP_H
#define TEST_APP_H

static char PROGRAM0[] = "10 HOME\r20 PRINT \"HACK THE PLANET\"\r30 PRINT \"WIZIO 2023\"\r";

/*
1 GR:HOME:PRINT:FOR X=4 TO 35:COLOR=C:H=C>9:POKE 1616+128*U+X,176+K*(C-11*H)+H:VLIN 0,39 AT X:C=C+K:K=K=0:U=U=K:NEXT
*/

static char PROGRAM1[] =
    "10 GR\r" // mode is not ready
    "20 FOR C = 0 TO 39\r"
    "30 FOR R = 0 TO 39\r"
    "40 COLOR = C\r"
    "50 PLOT C,R\r"
    "60 NEXT\r"
    "70 NEXT\r"
    "LIST\r"
    "RUN\r";

static char PROGRAM2[] =
    "10 HGR:HCOLOR=3\r"
    "20 FOR R = 0 TO 90 STEP 1\r"
    "30 FOR I = 0 TO 6.28 STEP 0.1\r"
    "40 X = 140 + R * SIN(I)\r"
    "50 Y =  95 + R * COS(I)\r"
    "60 HPLOT X,Y\r"
    "70 NEXT\r"
    "80 NEXT\r"
    "RUN\r";

static char PROGRAM3[] =
    "10 HGR\r"
    "20 FOR C = 0 TO 7\r"
    "30 FOR I = 0 TO 191 STEP 3\r"
    "40 HCOLOR = C\r"
    "50 HPLOT 0,I TO 0,I TO I,I TO I,0 TO 0,0\r"
    "60 NEXT\r"
    "70 NEXT\r"
    "80 GOTO 20\r"
    "RUN\r";

static char PROGRAM[] = "CALL -151\r1D00G\rN"; // run game

#endif