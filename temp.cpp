

int main()
{

    int *ptr1 = nullptr;

    const int *ptr2 = nullptr;

    ptr1 = ptr2; // Invalid
    *ptr1 = *ptr2; // Valid

    bool * bptr = &false;

    return 0;
}