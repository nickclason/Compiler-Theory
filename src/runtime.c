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


    int max_length = 256;
    char *string = malloc(max_length * sizeof(char));
    fgets(string, max_length, stdin);
    // remove trailing new line if there is one
    if ((strlen(string) > 0) && (string[strlen(string) - 1] == '\n'))
    {
        string[strlen(string) - 1] = '\0';
    }
    return string;

//    char *string = "a"; // TODO: WHY DOES DOING THIS WORK
//    return string;


//    char *buffer;
//    size_t buf_size = 32;
//    size_t characters;
//
//    buffer = (char *)malloc(buf_size * sizeof(char));
//    if( buffer == NULL)
//    {
//        perror("Unable to allocate buffer");
//        exit(1);
//    }
//
//    characters = getline(&buffer,&buf_size,stdin);
//
//    return buffer;

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