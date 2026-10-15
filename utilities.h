#ifndef UTILITIES_H
#define UTILITIES_H

#include "splashkit.h"

/**
 * Print prompts and read user input
 * 
 * @param prompt The prompt printed to the terminal.
 * 
 * @return Information entered by the user.
 */
string read_string(string prompt);

/**
 * Prompt user to enter an integer. If the input is not an integer, prompt the user continuously until an integer is entered.
 * 
 * @param prompt The prompt printed to the terminal.
 * 
 * @return The integer entered by the user.
 */
int read_integer(string prompt);

/**
 * Prompts user to enter an integer within a specified range. If the input is outside the range, prompt continuously until the correct input is entered.
 * 
 * @param prompt The prompt printed to the terminal.
 * @param min The minimum allowed input value.
 * @param max The maximum allowed input value.
 * 
 * @return The Integer within a specified range entered by the user.
 */
int read_integer(string prompt, int min, int max);

/**
 * Prompt user to enter a Double-precision floating-point number. If the input is not a Double-precision floating-point number, prompt the user continuously until a Double-precision floating-point is entered.
 * 
 * @param prompt The prompt printed to the terminal.
 * 
 * @return The Double-precision floating-point number entered by the user.
 */
double read_double(string prompt);

/**
 * Prompts user to enter a Double-precision floating-point number within a specified range. If the input is outside the range, prompt continuously until the correct input is entered.
 * 
 * @param prompt The prompt printed to the terminal.
 * @param min The minimum allowed input value.
 * @param max The maximum allowed input value.
 * 
 * @return The Double-precision floating-point number within a specified range entered by the user.
 */
double read_double(string prompt, double min, double max);

/**
 * Swap the values ​​of two integer variables.
 * 
 * @param x The first integer passed by reference.
 * @param y The second integer passed by reference.
 */
void swap(int &x, int &y);

#endif
