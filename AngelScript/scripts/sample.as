int fib(int n)
{
    if (n <= 1)
        return n;

    return fib(n - 1) + fib(n - 2);
}

int fib_test()
{
    int total = 0;
    for (int i = 0; i < 6; ++i)
        total += fib(10) + i;

    return total;
}

int main()
{
    fib_test();

    float angle = 1.57079637f;
    float value = math::sin(angle);
    print("sin=" + value + "\n");
    return int(value * 1000.0f);
}

