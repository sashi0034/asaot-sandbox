int fib(int n)
{
    if (n <= 1)
        return n;

    return fib(n - 1) + fib(n - 2);
}

int main()
{
    print("sample.as: begin\n");

    int total = 0;
    for (int i = 0; i < 6; ++i)
        total += fib(10) + i;

    print("sample.as: end\n");
    return total;
}
