/*
 * input.h
 *
 *  Created on: 15-Apr-2022
 *      Author: ASUS
 */

#ifndef INPUT_H_
#define INPUT_H_



typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;
int power(int,int);

char* i_to_a(int);
int number_of_digits(int);
uint8_t strCompare(char*, const char*);
void processInputCommands();
void displayConnectionInfo();
uint8_t strLength(char*);


#endif /* INPUT_H_ */
