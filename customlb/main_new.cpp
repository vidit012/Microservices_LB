#include "LoadBalancer.h"
#include <iostream>
#include <csignal>
#include <memory>

std::unique_ptr<LoadBalancer> lb;

void signalHandler(int signum) {
    std::cout << "\n\nInterrupt signal (" << signum << ") received. Shutting down...\n";
    if (lb) {
        lb->stop();
    }
    exit(signum);
}

int main() {
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "==============================================\n";
    std::cout << "  Custom C++ Load Balancer for Microservices\n";
    std::cout << "  Replacing Nginx with Custom Implementation\n";
    std::cout << "==============================================\n\n";
    
    // Create load balancer (port 80 for main traffic, 8081 for stats)
    lb = std::make_unique<LoadBalancer>(80, 8081);
    
    // Configure services matching nginx.conf
    
    // 1. Customer Service - IP Hash (Session Persistence)
    std::cout << "Configuring Customer service (IP Hash)..." << std::endl;
    lb->addService("/customer/", LoadBalancingAlgorithm::IP_HASH);
    lb->addBackendToService("/customer/", "customer-1", "customer", 8080, 3, 30);
    
    // 2. Catalog Service - Least Connections
    std::cout << "Configuring Catalog service (Least Connections)..." << std::endl;
    lb->addService("/catalog/", LoadBalancingAlgorithm::LEAST_CONNECTIONS);
    lb->addBackendToService("/catalog/", "catalog-1", "catalog", 8080, 3, 30);
    
    // 3. Order Service - Round Robin
    std::cout << "Configuring Order service (Round Robin)..." << std::endl;
    lb->addService("/order/", LoadBalancingAlgorithm::ROUND_ROBIN);
    lb->addBackendToService("/order/", "order-1", "order", 8080, 3, 30);
    
    std::cout << "\nConfiguration complete!\n" << std::endl;
    
    // Start the load balancer
    lb->start();
    
    return 0;
}
