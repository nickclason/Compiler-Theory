//
// Created by Nick Clason on 3/9/21.
//

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


bool PUTINTEGER(int num)
{
    printf("%d\n", num);
    return true;
}

int GETINTEGER()
{
    int num;
    scanf("%d", &num);
    return num;
}

bool PUTFLOAT(float num)
{
    printf("%f\n", num);
    return true;
}

float GETFLOAT()
{
    float num;
    scanf("%f", &num);
    return num;
}

bool PUTBOOL(bool val)
{
    val ? printf("true") : printf("false");
//    printf("%i\n", val);
    return true;
}

bool GETBOOL()
{
    int val;
    scanf("%i", &val);
    return (val == 1);
}

bool PUTSTRING(char *str)
{
    printf("%s\n", str);
    return true;
}

char* GETSTRING()
{
    int max_length = 256;
    char *string = malloc(max_length * sizeof(char));
    scanf("%s", string);
    string[strlen(string) - 1] = '\0';
    return string;

    //  So for some reason, when you hard code in the string, string comparisons work (str == "A")
    //  I cannot figure out why my regular method of taking a string as an input causes the comparison
    //  to fail, so leaving as is for now, I do not think it is a major issue as this is one of the very
    //  few things not working correctly, as far as I am aware.

    // char *string = "A";
    // return string;
}

float SQRT(int num)
{
    return sqrt((double)num);
}

void OOB_ERROR()
{
    printf("Out-of-bounds error\n");
    exit(1);
}