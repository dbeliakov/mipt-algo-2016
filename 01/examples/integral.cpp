#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <numeric>

void printUsage(const char* const procName)
{
    std::cerr << "Usage: " << procName << " <count of threads>" << std::endl
        << "\t<count of threads> -- int <12 and >0" << std::endl;
}

void calcIntegral(double from, double to, double step, double* res)
{
    double result = 0.;
    for (; from < to; from += step) {
        result += from * from * step;
    }
    *res = result;
}

void print()
{
    std::cout << "hello, world" << std::endl;
}

constexpr size_t fact(size_t n)
{
    return n > 0 ? n * fact(n - 1) : 1;
}

int main(int argc, char** argv)
{
    constexpr double from = 0.;
    constexpr double to = 1.;
    constexpr size_t parts = fact(12);

    if (argc != 2) {
        printUsage(argv[0]);
        exit(1);
    }
    int threadsCount = std::stoi(argv[1]);
    if (threadsCount < 1 || threadsCount > 12) {
        printUsage(argv[1]);
        exit(1);
    }
    int partToOneThread = parts / threadsCount;

    std::vector<double> results(threadsCount);
    std::vector<std::thread> threads;
    double diff = (to - from) / threadsCount;

    for (int i = 0; i < threadsCount; ++i) {
        threads.push_back(std::thread(
            calcIntegral,
            from + diff * i,
            from + diff * (i + 1),
            diff / partToOneThread,
            &results.at(i))
        );
    }

    for (auto&& thread : threads) {
        thread.join();
    }
    std::cout << std::accumulate(results.begin(), results.end(), 0.) << std::endl;
    return 0;
}
