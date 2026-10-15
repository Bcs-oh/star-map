#include "splashkit.h"

string read_string(string prompt)
{
    write(prompt);
    return read_line();
}

int read_integer(string prompt)
{
    string input = read_string(prompt);
    //Check if the input is an integer.
    while (!is_integer(input))
    {
        write_line("Please enter a whole number.");
        input = read_string(prompt);
    }
    //Convert the input string to an integer and return it.
    return to_integer(input);
}

int read_integer(string prompt, int min, int max)
{
    int result;
    //Check if the input is within range.
    do
    {
        result = read_integer(prompt);
        if (result < min || result > max)
        {
            write_line("Please enter a number between " + to_string(min) + " and " + to_string(max) + ".");
        }
    } while (result < min || result > max);
    return result;
}

double read_double(string prompt)
{
    string line;
    line = read_string(prompt);
    //Check if the input is a Double-precision floating-point number.
    while (!is_double(line))
    {
        write_line("Please enter number");
        line = read_string(prompt);
    }
    //Convert the input string to a Double-precision floating-point number and return it.
    return to_double(line);
}

double read_double(string prompt, double min, double max)
{
    //Ensure that the minimum input value is less than the maximum input value. If not, throw the data..
    if (min >= max)
    {
        string message = "Min must be smaller than Max";
        throw message;
    }

    double result;
    result = read_double(prompt);
    //When the input value is outside the range, prompt user  prompt the user continuously until the input value is within range.
    while (result < min or result > max)
    {
        write_line("Enter a value between " + to_string(min) + " and " + to_string(max));
        result = read_double(prompt);
    }
    return result;
}

void swap(int &x, int &y)
{
    //Use a temp variable to swap the values ​​of the two variables x and y.
    int temp;
    temp = x;
    x = y;
    y = temp;
}