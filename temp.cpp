
void weird_ptr()
{
    int num;

    int **ptr = &&num;

}


void double_ptr()
{
    int ** ptr1 = nullptr;
    int * const * ptr2 = nullptr;

    int * const *  * ptr3 = &ptr2; 
}

void single_ptr()
{
    int *ptr1 = nullptr;
    const int *ptr2 = nullptr;

    ptr1 = ptr2; // Invalid
    *ptr1 = *ptr2; // Valid
}



int main()
{

    int *ptr1 = nullptr;

    const int *ptr2 = nullptr;

    int * const ptr3 = nullptr;
    ptr3 = nullptr;

    ptr1 = ptr2; // Invalid
    *ptr1 = *ptr2; // Valid

    bool * bptr = &false;

    return 0;
}