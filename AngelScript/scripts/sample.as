uint mix32(uint x)
{
    x = x ^ (x >> 16);
    x = x * 0x7feb352d;
    x = x ^ (x >> 15);
    x = x * 0x846ca68b;
    x = x ^ (x >> 16);
    return x;
}

uint mix_checksum(uint acc, uint value)
{
    return acc ^ (value + uint(0x9e3779b9) + (acc << 6) + (acc >> 2));
}

int fib_recursive_impl(int n)
{
    if (n < 2)
        return n;
    return fib_recursive_impl(n - 1) + fib_recursive_impl(n - 2);
}

int solve_queen_impl(int row, int n, int cols, int diag1, int diag2)
{
    if (row == n)
        return 1;

    int available = ((1 << n) - 1) & ~(cols | diag1 | diag2);
    int solutions = 0;
    while (available != 0)
    {
        int bit = available & -available;
        available -= bit;
        solutions += solve_queen_impl(row + 1, n, cols | bit, (diag1 | bit) << 1, (diag2 | bit) >> 1);
    }
    return solutions;
}

uint work_native_loop(int seed)
{
    uint acc = uint(seed);
    for (uint i = 0; i <= 4000000; ++i)
        acc = acc + ((i * 3) ^ (i >> 2));
    return acc;
}

uint benchmark_native_loop(int repeat_count)
{
    uint acc = 0;
    for (int i = 0; i < repeat_count; ++i)
        acc = mix_checksum(acc, work_native_loop(i));
    return acc;
}

uint work_fibonacci_loop(int seed)
{
    uint a = uint(seed);
    uint b = uint(seed + 1);
    uint acc = 0;
    for (int i = 0; i < 250000; ++i)
    {
        uint c = a + b;
        a = b;
        b = c;
        acc = acc ^ c;
    }
    return mix_checksum(acc, a ^ b);
}

uint benchmark_fibonacci_loop(int repeat_count)
{
    uint acc = 0;
    for (int i = 0; i < repeat_count; ++i)
        acc = mix_checksum(acc, work_fibonacci_loop(i));
    return acc;
}

uint work_fibonacci_recursive(int seed)
{
    return uint(fib_recursive_impl(31 + (seed & 1)));
}

uint benchmark_fibonacci_recursive(int repeat_count)
{
    uint acc = 0;
    for (int i = 0; i < repeat_count; ++i)
        acc = mix_checksum(acc, work_fibonacci_recursive(i));
    return acc;
}

uint work_mandelbrot(int seed)
{
    int size = 96;
    double offset = double(seed & 7) * 0.0001;
    uint acc = 0;
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            double cr = (double(x) / double(size)) * 3.0 - 2.0 + offset;
            double ci = (double(y) / double(size)) * 2.0 - 1.0 - offset;
            double zr = 0.0;
            double zi = 0.0;
            int iter = 0;
            while ((zr * zr + zi * zi) <= 4.0 && iter < 80)
            {
                double next_r = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = next_r;
                ++iter;
            }
            acc += uint(iter);
        }
    }
    return acc;
}

uint benchmark_mandelbrot(int repeat_count)
{
    uint acc = 0;
    for (int i = 0; i < repeat_count; ++i)
        acc = mix_checksum(acc, work_mandelbrot(i));
    return acc;
}

uint work_queen(int seed)
{
    return uint(solve_queen_impl(0, 11 + (seed & 1), 0, 0, 0));
}

uint benchmark_queen(int repeat_count)
{
    uint acc = 0;
    for (int i = 0; i < repeat_count; ++i)
        acc = mix_checksum(acc, work_queen(i));
    return acc;
}

int main()
{
    return int(benchmark_fibonacci_recursive(1) & 0x7fffffff);
}
