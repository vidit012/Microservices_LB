#include "LoadBalancer.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <climits>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <iomanip>

using namespace std;

// ==================== Backend Implementation ====================

Backend::Backend(const string& n, const string& h, int p, int maxF, int timeout)
    : name(n), host(h), port(p), activeConnections(0), isHealthy(true),
      consecutiveFailures(0), maxFails(maxF), failTimeout(timeout) {
}

bool Backend::checkHealth() {
    // Simple TCP health check
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    
    // Set timeout
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    struct hostent* server = gethostbyname(host.c_str());
    if (!server) {
        close(sock);
        return false;
    }
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
    serverAddr.sin_port = htons(port);
    
    int result = connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    close(sock);
    
    return result == 0;
}

void Backend::recordFailure() {
    consecutiveFailures++;
    lastFailTime = chrono::steady_clock::now();
    
    if (consecutiveFailures >= maxFails) {
        isHealthy = false;
        cout << "[HEALTH] Backend " << name << " marked as DOWN ("
             << consecutiveFailures << " failures)" << endl;
    }
}

void Backend::recordSuccess() {
    if (consecutiveFailures > 0) {
        consecutiveFailures = 0;
        if (!isHealthy) {
            isHealthy = true;
            cout << "[HEALTH] Backend " << name << " marked as UP" << endl;
        }
    }
}

bool Backend::shouldRetry() {
    if (isHealthy) return true;
    
    auto now = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::seconds>(now - lastFailTime).count();
    
    if (elapsed >= failTimeout) {
        cout << "[HEALTH] Retry timeout expired for " << name << ", attempting recovery" << endl;
        consecutiveFailures = 0; // Reset for retry
        return true;
    }
    
    return false;
}

// ==================== ServiceConfig Implementation ====================

ServiceConfig::ServiceConfig(const string& p, LoadBalancingAlgorithm algo)
    : path(p), algorithm(algo), roundRobinIndex(0) {
}

shared_ptr<Backend> ServiceConfig::selectBackend(const string& clientIP) {
    switch (algorithm) {
        case LoadBalancingAlgorithm::ROUND_ROBIN:
            return selectRoundRobin();
        case LoadBalancingAlgorithm::LEAST_CONNECTIONS:
            return selectLeastConnections();
        case LoadBalancingAlgorithm::IP_HASH:
            return selectIPHash(clientIP);
    }
    return nullptr;
}

shared_ptr<Backend> ServiceConfig::selectRoundRobin() {
    vector<shared_ptr<Backend>> healthy;
    for (auto& backend : backends) {
        if (backend->isHealthy || backend->shouldRetry()) {
            healthy.push_back(backend);
        }
    }
    
    if (healthy.empty()) return nullptr;
    
    size_t index = roundRobinIndex.fetch_add(1) % healthy.size();
    return healthy[index];
}

shared_ptr<Backend> ServiceConfig::selectLeastConnections() {
    shared_ptr<Backend> selected = nullptr;
    int minConnections = INT_MAX;
    
    for (auto& backend : backends) {
        if (backend->isHealthy || backend->shouldRetry()) {
            int conns = backend->activeConnections.load();
            if (conns < minConnections) {
                minConnections = conns;
                selected = backend;
            }
        }
    }
    
    return selected;
}

shared_ptr<Backend> ServiceConfig::selectIPHash(const string& clientIP) {
    vector<shared_ptr<Backend>> healthy;
    for (auto& backend : backends) {
        if (backend->isHealthy || backend->shouldRetry()) {
            healthy.push_back(backend);
        }
    }
    
    if (healthy.empty()) return nullptr;
    
    hash<string> hasher;
    size_t hashValue = hasher(clientIP);
    size_t index = hashValue % healthy.size();
    
    return healthy[index];
}

// ==================== HttpRequest Implementation ====================

HttpRequest HttpRequest::parse(const string& rawRequest) {
    HttpRequest req;
    istringstream stream(rawRequest);
    string line;
    
    // Parse request line
    if (getline(stream, line)) {
        istringstream requestLine(line);
        requestLine >> req.method >> req.path >> req.version;
    }
    
    // Parse headers
    while (getline(stream, line) && line != "\r" && !line.empty()) {
        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            string key = line.substr(0, colonPos);
            string value = line.substr(colonPos + 1);
            // Trim whitespace
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            req.headers[key] = value;
        }
    }
    
    // Parse body (if any)
    string bodyPart;
    while (getline(stream, line)) {
        bodyPart += line + "\n";
    }
    req.body = bodyPart;
    
    return req;
}

string HttpRequest::toString() const {
    ostringstream oss;
    oss << method << " " << path << " " << version << "\r\n";
    for (const auto& header : headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    oss << "\r\n";
    if (!body.empty()) {
        oss << body;
    }
    return oss.str();
}

// ==================== HttpResponse Implementation ====================

HttpResponse HttpResponse::parse(const string& rawResponse) {
    HttpResponse resp;
    istringstream stream(rawResponse);
    string line;
    
    // Parse status line
    if (getline(stream, line)) {
        istringstream statusLine(line);
        statusLine >> resp.version >> resp.statusCode;
        getline(statusLine, resp.statusMessage);
        // Trim whitespace
        resp.statusMessage.erase(0, resp.statusMessage.find_first_not_of(" \t\r\n"));
    }
    
    // Parse headers
    while (getline(stream, line) && line != "\r" && !line.empty()) {
        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            string key = line.substr(0, colonPos);
            string value = line.substr(colonPos + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            resp.headers[key] = value;
        }
    }
    
    // Parse body
    string bodyPart;
    while (getline(stream, line)) {
        bodyPart += line + "\n";
    }
    resp.body = bodyPart;
    
    return resp;
}

string HttpResponse::toString() const {
    ostringstream oss;
    oss << version << " " << statusCode << " " << statusMessage << "\r\n";
    for (const auto& header : headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    oss << "\r\n";
    if (!body.empty()) {
        oss << body;
    }
    return oss.str();
}

// ==================== HealthChecker Implementation ====================

HealthChecker::HealthChecker(int intervalSeconds)
    : running(false), checkIntervalSeconds(intervalSeconds) {
}

HealthChecker::~HealthChecker() {
    stop();
}

void HealthChecker::addBackend(shared_ptr<Backend> backend) {
    allBackends.push_back(backend);
}

void HealthChecker::start() {
    running = true;
    healthCheckThread = thread(&HealthChecker::healthCheckLoop, this);
}

void HealthChecker::stop() {
    running = false;
    if (healthCheckThread.joinable()) {
        healthCheckThread.join();
    }
}

void HealthChecker::healthCheckLoop() {
    while (running) {
        for (auto& backend : allBackends) {
            bool healthy = backend->checkHealth();
            if (healthy) {
                backend->recordSuccess();
            } else {
                backend->recordFailure();
            }
        }
        
        this_thread::sleep_for(chrono::seconds(checkIntervalSeconds));
    }
}

// ==================== LoadBalancer Implementation ====================

LoadBalancer::LoadBalancer(int port, int stats)
    : listenPort(port), statsPort(stats), running(false),
      totalRequests(0), failedRequests(0),
      totalBytesReceived(0), totalBytesSent(0) {
    healthChecker = make_unique<HealthChecker>(30); // Check every 30 seconds
}

LoadBalancer::~LoadBalancer() {
    stop();
}

void LoadBalancer::addService(const string& path, LoadBalancingAlgorithm algo) {
    services[path] = make_shared<ServiceConfig>(path, algo);
}

void LoadBalancer::addBackendToService(const string& path, const string& name,
                                      const string& host, int port,
                                      int maxFails, int failTimeout) {
    auto it = services.find(path);
    if (it != services.end()) {
        auto backend = make_shared<Backend>(name, host, port, maxFails, failTimeout);
        it->second->backends.push_back(backend);
        healthChecker->addBackend(backend);
    }
}

shared_ptr<ServiceConfig> LoadBalancer::matchService(const string& path) {
    // Find longest matching prefix
    shared_ptr<ServiceConfig> matched = nullptr;
    size_t maxLen = 0;
    
    for (auto& [servicePath, config] : services) {
        if (path.find(servicePath) == 0 && servicePath.length() > maxLen) {
            matched = config;
            maxLen = servicePath.length();
        }
    }
    
    return matched;
}

bool LoadBalancer::forwardRequest(int clientSocket, HttpRequest& request,
                                  shared_ptr<Backend> backend,
                                  const string& clientIP) {
    // Connect to backend
    int backendSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (backendSocket < 0) {
        return false;
    }
    
    // Set timeouts
    struct timeval tv;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    setsockopt(backendSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(backendSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    struct hostent* server = gethostbyname(backend->host.c_str());
    if (!server) {
        close(backendSocket);
        return false;
    }
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
    serverAddr.sin_port = htons(backend->port);
    
    if (connect(backendSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(backendSocket);
        return false;
    }
    
    // Add/modify headers for proxying
    request.headers["X-Real-IP"] = clientIP;
    request.headers["X-Forwarded-For"] = clientIP;
    request.headers["X-Forwarded-Proto"] = "http";
    request.headers["Connection"] = "close";
    
    // Send request to backend
    string requestStr = request.toString();
    ssize_t sent = send(backendSocket, requestStr.c_str(), requestStr.length(), 0);
    if (sent < 0) {
        close(backendSocket);
        return false;
    }
    
    totalBytesSent += sent;
    
    // Receive response from backend
    char buffer[8192];
    string response;
    ssize_t bytesRead;
    
    while ((bytesRead = recv(backendSocket, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, bytesRead);
        totalBytesReceived += bytesRead;
    }
    
    close(backendSocket);
    
    // Forward response to client
    if (!response.empty()) {
        send(clientSocket, response.c_str(), response.length(), 0);
        return true;
    }
    
    return false;
}

void LoadBalancer::handleClient(int clientSocket, const string& clientIP) {
    totalRequests++;
    
    char buffer[8192] = {0};
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead <= 0) {
        close(clientSocket);
        return;
    }
    
    string rawRequest(buffer, bytesRead);
    HttpRequest request = HttpRequest::parse(rawRequest);
    
    // Check for health endpoint
    if (request.path == "/health") {
        string response = generateHealthCheckResponse();
        send(clientSocket, response.c_str(), response.length(), 0);
        close(clientSocket);
        logRequest(clientIP, request.method, request.path, 200, "health-check");
        return;
    }
    
    // Serve index.html for root path
    if (request.path == "/" || request.path == "/index.html") {
        string indexHTML = R"(<!DOCTYPE html>
<html>
<head>
<title>Order Processing</title>
<link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.2.0/css/bootstrap.min.css" />
<link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.2.0/css/bootstrap-theme.min.css" />
<script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.2.0/js/bootstrap.min.js"></script>
</head>
<body>
<h1>Order Processing</h1>
<div class="container">
<div class="row">
<div class="col-md-4"><a href="/customer/list.html">Customer</a></div>
<div class="col-md-4">List / add / remove customers</div>
</div>
<div class="row">
<div class="col-md-4"><a href="/catalog/list.html">Catalog</a></div>
<div class="col-md-4">List / add / remove items</div>
</div>
<div class="row">
<div class="col-md-4"><a href="/catalog/searchForm.html">Catalog</a></div>
<div class="col-md-4">Search Items</div>
</div>
<div class="row">
<div class="col-md-4"><a href="/order/">Order</a></div>
<div class="col-md-4">Create an order</div>
</div>
</div>
</body>
</html>)";
        
        ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Content-Length: " << indexHTML.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << indexHTML;
        
        string responseStr = response.str();
        send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
        close(clientSocket);
        logRequest(clientIP, request.method, request.path, 200, "static-index");
        return;
    }
    
    // Match service by path
    auto service = matchService(request.path);
    if (!service) {
        string response = "HTTP/1.1 404 Not Found\r\n\r\nService not found";
        send(clientSocket, response.c_str(), response.length(), 0);
        close(clientSocket);
        failedRequests++;
        logRequest(clientIP, request.method, request.path, 404, "no-service");
        return;
    }
    
    // Strip the service prefix from the path (like Nginx proxy_pass with trailing /)
    // E.g., /catalog/list.html -> /list.html
    string originalPath = request.path;
    if (request.path.find(service->path) == 0) {
        request.path = "/" + request.path.substr(service->path.length());
    }
    
    // Select backend with retry logic
    const int maxRetries = 3;
    bool success = false;
    
    for (int attempt = 0; attempt < maxRetries && !success; attempt++) {
        auto backend = service->selectBackend(clientIP);
        
        if (!backend) {
            string response = "HTTP/1.1 503 Service Unavailable\r\n\r\nNo healthy backends";
            send(clientSocket, response.c_str(), response.length(), 0);
            failedRequests++;
            logRequest(clientIP, request.method, originalPath, 503, "no-backend");
            break;
        }
        
        backend->activeConnections++;
        
        success = forwardRequest(clientSocket, request, backend, clientIP);
        
        if (success) {
            backend->recordSuccess();
            logRequest(clientIP, request.method, originalPath, 200, backend->name);
        } else {
            backend->recordFailure();
            failedRequests++;
            logRequest(clientIP, request.method, originalPath, 502, backend->name + "-failed");
        }
        
        backend->activeConnections--;
    }
    
    if (!success) {
        string response = "HTTP/1.1 502 Bad Gateway\r\n\r\nBackend error";
        send(clientSocket, response.c_str(), response.length(), 0);
    }
    
    close(clientSocket);
}

void LoadBalancer::handleStatsRequest(int clientSocket) {
    string response = generateStatsHTML();
    send(clientSocket, response.c_str(), response.length(), 0);
    close(clientSocket);
}

string LoadBalancer::generateStatsHTML() {
    ostringstream html;
    auto now = chrono::system_clock::now();
    auto timeT = chrono::system_clock::to_time_t(now);
    
    html << "HTTP/1.1 200 OK\r\n";
    html << "Content-Type: text/html\r\n";
    html << "Connection: close\r\n\r\n";
    
    html << "<!DOCTYPE html><html><head><title>Load Balancer Stats</title>";
    html << "<style>body{font-family:Arial;margin:20px;}table{border-collapse:collapse;width:100%;margin:20px 0;}";
    html << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}th{background-color:#4CAF50;color:white;}";
    html << ".healthy{color:green;}.unhealthy{color:red;}</style></head><body>";
    
    html << "<h1>Custom C++ Load Balancer Statistics</h1>";
    html << "<p><strong>Status:</strong> RUNNING</p>";
    html << "<p><strong>Last Updated:</strong> " << ctime(&timeT) << "</p>";
    
    html << "<h2>Overall Statistics</h2>";
    html << "<table><tr><th>Metric</th><th>Value</th></tr>";
    html << "<tr><td>Total Requests</td><td>" << totalRequests.load() << "</td></tr>";
    html << "<tr><td>Failed Requests</td><td>" << failedRequests.load() << "</td></tr>";
    html << "<tr><td>Success Rate</td><td>";
    if (totalRequests > 0) {
        double successRate = (double)(totalRequests - failedRequests) / totalRequests * 100;
        html << fixed << setprecision(2) << successRate << "%";
    } else {
        html << "N/A";
    }
    html << "</td></tr>";
    html << "<tr><td>Bytes Received</td><td>" << totalBytesReceived.load() << "</td></tr>";
    html << "<tr><td>Bytes Sent</td><td>" << totalBytesSent.load() << "</td></tr>";
    html << "</table>";
    
    html << "<h2>Services and Backends</h2>";
    for (const auto& [path, service] : services) {
        string algoName;
        switch (service->algorithm) {
            case LoadBalancingAlgorithm::ROUND_ROBIN: algoName = "Round Robin"; break;
            case LoadBalancingAlgorithm::LEAST_CONNECTIONS: algoName = "Least Connections"; break;
            case LoadBalancingAlgorithm::IP_HASH: algoName = "IP Hash"; break;
        }
        
        html << "<h3>Service: " << path << " (Algorithm: " << algoName << ")</h3>";
        html << "<table><tr><th>Name</th><th>Host:Port</th><th>Status</th><th>Active Connections</th><th>Failures</th></tr>";
        
        for (const auto& backend : service->backends) {
            html << "<tr>";
            html << "<td>" << backend->name << "</td>";
            html << "<td>" << backend->host << ":" << backend->port << "</td>";
            html << "<td class='" << (backend->isHealthy ? "healthy" : "unhealthy") << "'>";
            html << (backend->isHealthy ? "UP" : "DOWN") << "</td>";
            html << "<td>" << backend->activeConnections.load() << "</td>";
            html << "<td>" << backend->consecutiveFailures.load() << "</td>";
            html << "</tr>";
        }
        html << "</table>";
    }
    
    html << "<br><p><a href='/nginx_status'>Refresh</a></p>";
    html << "</body></html>";
    
    return html.str();
}

string LoadBalancer::generateHealthCheckResponse() {
    ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/plain\r\n";
    response << "Connection: close\r\n\r\n";
    response << "healthy\n";
    return response.str();
}

void LoadBalancer::logRequest(const string& clientIP, const string& method,
                              const string& path, int statusCode,
                              const string& backendName) {
    lock_guard<mutex> lock(logMutex);
    auto now = chrono::system_clock::now();
    auto timeT = chrono::system_clock::to_time_t(now);
    
    char timeBuffer[100];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localtime(&timeT));
    
    cout << "[" << timeBuffer << "] "
         << clientIP << " \"" << method << " " << path << "\" "
         << statusCode << " backend=" << backendName << endl;
}

void LoadBalancer::start() {
    cout << "\n=== Custom C++ Load Balancer ===" << endl;
    cout << "Starting health checker..." << endl;
    healthChecker->start();
    
    cout << "Configured services:" << endl;
    for (const auto& [path, service] : services) {
        cout << "  " << path << " -> " << service->backends.size() << " backends" << endl;
    }
    
    running = true;
    
    // Start stats server in separate thread
    thread statsThread([this]() {
        int statsSocket = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(statsSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in statsAddr;
        memset(&statsAddr, 0, sizeof(statsAddr));
        statsAddr.sin_family = AF_INET;
        statsAddr.sin_addr.s_addr = INADDR_ANY;
        statsAddr.sin_port = htons(statsPort);
        
        bind(statsSocket, (struct sockaddr*)&statsAddr, sizeof(statsAddr));
        listen(statsSocket, 10);
        
        cout << "Stats server listening on port " << statsPort << endl;
        
        while (running) {
            int clientSocket = accept(statsSocket, nullptr, nullptr);
            if (clientSocket >= 0) {
                thread([this, clientSocket]() {
                    handleStatsRequest(clientSocket);
                }).detach();
            }
        }
        
        close(statsSocket);
    });
    statsThread.detach();
    
    // Start main load balancer server
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(listenPort);
    
    if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Bind failed on port " << listenPort << endl;
        return;
    }
    
    if (listen(serverSocket, 100) < 0) {
        cerr << "Listen failed" << endl;
        return;
    }
    
    cout << "Load balancer listening on port " << listenPort << endl;
    cout << "Stats available at http://localhost:" << statsPort << "/nginx_status" << endl;
    cout << "Health check at http://localhost:" << listenPort << "/health" << endl;
    cout << "Press Ctrl+C to stop\n" << endl;
    
    while (running) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket >= 0) {
            string clientIP = inet_ntoa(clientAddr.sin_addr);
            
            thread([this, clientSocket, clientIP]() {
                handleClient(clientSocket, clientIP);
            }).detach();
        }
    }
    
    close(serverSocket);
}

void LoadBalancer::stop() {
    running = false;
    healthChecker->stop();
}
