#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

long long getNanoTime() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
}

class TokenBucket {
private:
    long long m_maxBucketSize;
    long long m_refillRate;
    double m_currentBucketSize;
    long long m_lastRefillTimestamp;
    std::mutex mtx;

    void refill() {
        auto now = getNanoTime();
        double tokensToAdd = (now - m_lastRefillTimestamp) * m_refillRate / 1e9;
        m_currentBucketSize =
            std::min(static_cast<long long>(m_currentBucketSize + tokensToAdd), m_maxBucketSize);
        m_lastRefillTimestamp = now;
    }
public:
    TokenBucket(long long maxBucketSize, long long refillRate)
        : m_maxBucketSize(maxBucketSize), m_refillRate(refillRate) {
        this->m_currentBucketSize = m_maxBucketSize;
        this->m_lastRefillTimestamp = getNanoTime();
    }

    bool allowRequest(int tokens) {
        std::lock_guard<std::mutex> lock(mtx);
        refill();
        bool allowed = true;

        if (m_currentBucketSize >= tokens) {
            m_currentBucketSize -= tokens;
        } else {
            allowed = false;
        }

        return allowed;
    }
};

void makeRequests(int id, TokenBucket &tokenBucket, int tokens, int requestCount) {
    for (int i = 1; i <= requestCount; ++i) {
        if (tokenBucket.allowRequest(tokens)) {
            std::cout << "Thread " << id << " - Request " << i << ": Allowed\n";
        } else {
            std::cout << "Thread " << id << " - Request " << i << ": Denied\n";
        }

        // Sleep for 1 second between requests
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    // Create a token bucket with a maximum size of 10 tokens and a refill rate of 4 tokens per second
    TokenBucket tokenBucket(10, 4);

    // Number of threads and requests per thread
    const int numThreads = 3;
    const int requestsPerThread = 5;

    // Vector to store thread objects
    std::vector<std::thread> threads;

    // Create threads
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(makeRequests, i + 1, std::ref(tokenBucket), 4, requestsPerThread);
    }

    // Join threads
    for (auto &thread : threads) {
        thread.join();
    }

    return 0;
}
