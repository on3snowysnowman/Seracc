

#include <iostream>

int c = 1;



struct Thing
{   
    int x;

    struct Nested
    {
        int x;
    };
};

int main()
{
    int c = 3;

    {
        int c = 4;
        std::cout << c << '\n';
    }

    std::cout << c << '\n';
}