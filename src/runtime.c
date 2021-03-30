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
    printf("%i\n", num);
    return true;
}

int GETINTEGER()
{
    int num;
    scanf("%i", &num);
    getchar();
    return num;
}

bool PUTBOOL(bool val)
{
    printf("%i\n", val);
    return true;
}

bool GETBOOL()
{
    int val;
    scanf("%i", &val);
    getchar();
    return (val == 1);
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
    getchar();
    return num;
}

bool PUTSTRING(char *str)
{
    printf("%s\n", str);
    return true;
}

char* GETSTRING()
{
    int max = 256;
    char *str = malloc(max * sizeof(char));
    fgets(str, max, stdin);
    if ((strlen(str) > 0) && (str[strlen(str) - 1] == '\n')) {
        str[strlen(str) - 1] = '\0';
    }
    return str;
}
//TODO: SQRT
//void BOUNDSERROR() {
//    printf("Error - out of bounds exception!\n");
//    exit(1);
//}