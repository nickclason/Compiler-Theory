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
    printf("%d\n", val);
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

    return string;
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