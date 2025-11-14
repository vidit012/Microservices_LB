#ifndef LOADBALANCER_H
#define LOADBALANCER_H

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include <chrono>
#include <memory>
#include <thread>
using namespace std;

// Load balancing algorithms
enum class LoadBalancingAlgorithm {
    ROUND_ROBIN,
    LEAST_CONNECTIONS,
    IP_HASH
};

// Backend server state
struct Backend {
    string name;
    string host;
    int port;
    atomic<int> activeConnections;
    atomic<bool> isHealthy;
    atomic<int> consecutiveFailures;
    chrono::time_point<chrono::steady_clock> lastFailTime;
    
    // Health check settings
    int maxFails;
    int failTimeout; // seconds
    
    Backend(const string& n, const string& h, int p, int maxF = 3, int timeout = 30);
    
    bool checkHealth();
    void recordFailure();
    void recordSuccess();
    bool shouldRetry();
};

// Service configuration for path-based routing
struct ServiceConfig {
    string path;
    LoadBalancingAlgorithm algorithm;
    vector<shared_ptr<Backend>> backends;
    atomic<size_t> roundRobinIndex;
    
    ServiceConfig(const string& p, LoadBalancingAlgorithm algo);
    
    shared_ptr<Backend> selectBackend(const string& clientIP);
    shared_ptr<Backend> selectRoundRobin();
    shared_ptr<Backend> selectLeastConnections();
    shared_ptr<Backend> selectIPHash(const string& clientIP);
};

// HTTP Request parser
struct HttpRequest {
    string method;
    string path;
    string version;
    map<string, string> headers;
    string body;
    
    static HttpRequest parse(const string& rawRequest);
    string toString() const;
};

// HTTP Response parser
struct HttpResponse {
    int statusCode;
    string statusMessage;
    string version;
    map<string, string> headers;
    string body;
    
    static HttpResponse parse(const string& rawResponse);
    string toString() const;
};

// Health checker (runs in background thread)
class HealthChecker {
private:
    vector<shared_ptr<Backend>> allBackends;
    atomic<bool> running;
    thread healthCheckThread;
    int checkIntervalSeconds;
    
    void healthCheckLoop();
    
public:
    HealthChecker(int intervalSeconds = 10);
    ~HealthChecker();
    
    void addBackend(shared_ptr<Backend> backend);
    void start();
    void stop();
};

// Main Load Balancer class
class LoadBalancer {
private:
    int listenPort;
    int statsPort;
    map<string, shared_ptr<ServiceConfig>> services;
    atomic<bool> running;
    unique_ptr<HealthChecker> healthChecker;
    
    // Statistics
    atomic<uint64_t> totalRequests;
    atomic<uint64_t> failedRequests;
    atomic<uint64_t> totalBytesReceived;
    atomic<uint64_t> totalBytesSent;
    
    mutex logMutex;
    
    void handleClient(int clientSocket, const string& clientIP);
    void handleStatsRequest(int clientSocket);
    string generateStatsHTML();
    string generateHealthCheckResponse();
    
    shared_ptr<ServiceConfig> matchService(const string& path);
    bool forwardRequest(int clientSocket, HttpRequest& request, 
                       shared_ptr<Backend> backend,
                       const string& clientIP);
    
    void logRequest(const string& clientIP, const string& method,
                   const string& path, int statusCode,
                   const string& backendName);
    
public:
    LoadBalancer(int port = 80, int stats = 8081);
    ~LoadBalancer();
    
    void addService(const string& path, LoadBalancingAlgorithm algo);
    void addBackendToService(const string& path, const string& name,
                            const string& host, int port,
                            int maxFails = 3, int failTimeout = 30);
    
    void start();
    void stop();
};

#endif // LOADBALANCER_H
