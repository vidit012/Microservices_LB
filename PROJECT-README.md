# Custom Load Balancer for Kubernetes Microservices

## 📋 Project Overview

This project started with the [ewolff/microservice-kubernetes](https://github.com/ewolff/microservice-kubernetes) repository, which originally used Apache as a reverse proxy. I developed a **custom C++ load balancer from scratch** to implement advanced load balancing algorithms, health monitoring, and metrics collection. The project also includes autoscaling capabilities and comprehensive monitoring across distributed services.

### Key Features

- ✅ Custom C++ Load Balancer: Built from scratch with multiple load balancing algorithms
- ✅ Multiple Load Balancing Strategies: Least Connection, Round Robin, Weighted Round Robin
- ✅ Content-Aware Routing: Path-based routing to different microservices
- ✅ Session Persistence: Connection tracking for stateful operations
- ✅ Health Checks & Failover: Active health monitoring and automatic failover
- ✅ TLS Termination: HTTPS with TLS 1.3 encryption
- ✅ Horizontal Pod Autoscaling: CPU and memory-based automatic scaling
- ✅ Comprehensive Monitoring: Prometheus metrics + Grafana dashboards
- ✅ Multi-Database Integration: PostgreSQL (SQL) + MongoDB (NoSQL) + MinIO (Object Storage)

## 🛤️ Project Journeyvidit-pt7945@Vidit-pt7945:~/Documents/microservices & LB$ 

### Phase 1: Initial Setup and Deployment
1. Cloned Original Repository: Started with [ewolff/microservice-kubernetes](https://github.com/ewolff/microservice-kubernetes)
2. Installed Minikube: Set up local Kubernetes cluster environment
3. Created Kubernetes Cluster: Deployed Minikube cluster with Docker driver
4. Deployed Original Application: Hosted the microservices on Kubernetes
5. Tested with Apache: Verified the application worked with the original Apache reverse proxy

### Phase 2: Custom Load Balancer Implementation
1. Designed C++ Load Balancer: Built a custom load balancer from scratch using C++
2. Implemented Core Algorithms: Developed three load balancing strategies:
   - Least Connection for Catalog service (optimal for variable processing times)
   - Round Robin for Customer service (even distribution)
   - Weighted Round Robin for Order service (priority-based routing)
3. Added Health Monitoring: Implemented active health checks with automatic backend failure detection
4. Built Request Router: Created path-based routing logic to direct traffic to appropriate microservices:
   - `/catalog/*` → Catalog service
   - `/customer/*` → Customer service
   - `/order/*` → Order service
5. Containerized Load Balancer: Created Dockerfile and built container image
6. Setup Ingress Controller: Configured nginx-ingress-controller as the entry point to the cluster
7. Deployed to Kubernetes: Replaced Apache with custom C++ load balancer as a Kubernetes service
8. Implemented Connection Pooling: Optimized backend connections for better performance
9. Added Metrics Endpoint: Exposed metrics for Prometheus monitoring
10. Tested Load Balancing: Verified routing, failover, and strategy effectiveness

### Phase 3: TLS/HTTPS Security Setup
1. Downloaded mkcert: Installed mkcert tool for local certificate authority
2. Created Self-Signed Certificate: Generated TLS certificate for `microservices.local` domain
3. Installed Local CA: Added mkcert's root certificate to system trust store
4. Created Kubernetes Secret: Stored TLS certificate and key as Kubernetes secret
5. Configured Ingress for HTTPS: Updated Ingress resource to use TLS certificate
6. Enabled SSL/TLS Termination: Ingress controller now terminates HTTPS connections
7. Tested HTTPS Access: Verified secure HTTPS requests to `https://microservices.local:8443`

### Phase 4: Monitoring and Observability Setup
1. Downloaded Prometheus via Helm: Installed Prometheus using Helm chart from prometheus-community
2. Downloaded Grafana via Helm: Installed Grafana using Helm chart from grafana repository
3. Port Forwarded Services: Set up port forwarding for Prometheus (9090) and Grafana (3000) for local access
4. Added Metrics Endpoints: Configured checkpoints and request points throughout the project:
   - Added Spring Boot Actuator to all microservices
   - Added Micrometer Prometheus dependency for `/actuator/prometheus` endpoints
   - Configured Prometheus annotations on Services for scraping
5. Configured Prometheus Scraping: Set up Prometheus to scrape metrics from:
   - All three microservices (Catalog, Customer, Order)
   - Nginx load balancer metrics exporter
   - Kubernetes pod and node metrics
6. Created Grafana Dashboards: Built dashboards to visualize metrics on Grafana
7. Tested Monitoring: Verified metrics collection and visualization in both Prometheus and Grafana

### Phase 5: Horizontal Pod Autoscaling (HPA)
1. Installed Metrics Server: Deployed metrics-server for HPA to collect CPU and memory metrics
2. Created HPA Configuration: Set up HPA resources for all three microservices:
   - CPU target: 75%
   - Memory target: 180% (adjusted for Spring Boot memory patterns)
   - Min replicas: 1
   - Max replicas: 3
3. Added Resource Requests/Limits: Configured resource specifications for each pod:
   - CPU: 200m request, 500m limit
   - Memory: 512Mi request, 1Gi limit
4. Tested Autoscaling with Load: Increased load on Catalog service to trigger scaling
5. Observed Scale-Up: HPA successfully scaled Catalog service from 1 to 3 replicas based on CPU/memory usage
6. Tested Scale-Down: Removed traffic load and observed behavior
7. Verified Cooldown Period: After 5-minute stabilization period, pods scaled back down to 1 replica

### Phase 6: Database Integration and Persistence
1. Deployed Persistent Volume Databases: Set up all databases as pods running locally inside the Kubernetes cluster
2. Configured Customer Microservice with MongoDB:
   - Deployed MongoDB pod with persistent volume
   - Connected Customer microservice to MongoDB
   - Database: `microservicesdb`, Collection: `customer`
   - CRUD Operations: Add customer → entry pushed to database, Delete customer → entry removed from database
3. Configured Catalog Microservice with PostgreSQL:
   - Deployed PostgreSQL pod with persistent volume
   - Connected Catalog microservice to PostgreSQL
   - Database: `microservicesdb`, Table: `item`
   - CRUD Operations: Add item → new entry in database, Delete item → entry removed from database
4. Configured Order Microservice with PostgreSQL:
   - Used separate PostgreSQL database for Order service
   - Database: `ordersdb`, Table: `simple_order`
   - CRUD Operations: Create order → entry pushed to database, Delete order → entry removed from database
5. Integrated Catalog Microservice with MinIO:
   - Deployed MinIO pod for object storage
   - Connected Catalog microservice to MinIO
   - Bucket: `catalog-images` for storing product images
   - Image Operations: Upload images for catalog items, retrieve images on demand
6. Tested All CRUD Operations: Verified complete data persistence across all microservices:
   - Catalog: Add/Delete items with image storage
   - Customer: Add/Delete customer records
   - Order: Create/Delete orders
7. Verified Data Persistence: Confirmed all data persists across pod restarts using persistent volumes

### Phase 7: Additional Enhancements
(Additional phases documented in subsequent sections)

## 🏗️ Architecture

```
┌─────────────┐
│   Client    │
│ (Browser)   │
└──────┬──────┘
       │ HTTPS (TLS 1.3)
       │ Port 8443
       ▼
┌─────────────────────────┐
│  Ingress Controller     │
│  (nginx-ingress)        │
│  - TLS Termination      │
│  - Certificate: mkcert  │
└─────────┬───────────────┘
          │
          ▼
┌────────────────────────────────────┐
│   Custom C++ Load Balancer         │
│   - Least Connection Algorithm     │
│   - Round Robin Algorithm          │
│   - Weighted Round Robin           │
│   - Health Checks (every 30s)      │
│   - Automatic Failover             │
│   - Metrics Export (port 8081)     │
└────┬───────────┬──────────┬────────┘
     │           │          │
     │ /catalog  │ /customer│ /order
     │ least_conn│ round_rob│ weighted
     │           │ in       │ round_rob
     │           │          │
     ▼           ▼          ▼
┌─────────┐ ┌──────────┐ ┌─────────┐
│ Catalog │ │ Customer │ │  Order  │
│ Service │ │ Service  │ │ Service │
│ (1-3    │ │ (1-3     │ │ (1-3    │
│  pods)  │ │  pods)   │ │  pods)  │
└────┬────┘ └────┬─────┘ └────┬────┘
     │           │            │
     │           │            ├──────────┐
     ▼           ▼            ▼          ▼
┌──────────┐ ┌────────┐ ┌──────────┐ ┌────────┐
│PostgreSQL│ │ MongoDB│ │PostgreSQL│ │ MinIO  │
│microserv…│ │microser│ │ ordersdb │ │catalog-│
│  icesdb  │ │vicesdb │ │          │ │ images │
└──────────┘ └────────┘ └──────────┘ └────────┘

         ┌────────────────┐
         │   Monitoring   │
         │ ┌───────────┐  │
         │ │Prometheus │  │
         │ └─────┬─────┘  │
         │       │        │
         │ ┌─────▼─────┐  │
         │ │  Grafana  │  │
         │ └───────────┘  │
         └────────────────┘

         ┌────────────────┐
         │      HPA       │
         │ CPU: 70%       │
         │ Memory: 80%    │
         │ Min: 1, Max: 3 │
         └────────────────┘
```

## 🛠️ Technology Stack

| Component | Technology | Version |
|-----------|-----------|---------|
| Orchestration | Kubernetes (Minikube) | v1.31+ |
| Load Balancer | Custom C++ Implementation | 1.0 |
| Microservices | Spring Boot | 2.7+ |
| SQL Database | PostgreSQL | 15+ |
| NoSQL Database | MongoDB | 6.0+ |
| Object Storage | MinIO | RELEASE.2023-09-30T07-02-29Z |
| Metrics | Prometheus | 2.40+ |
| Dashboards | Grafana | 9.0+ |
| Ingress | nginx-ingress-controller | 1.10+ |
| Load Testing | Apache Bench | 2.3+ |

## 🚀 Load Balancing Features

### 1. Multiple Routing Strategies

#### Least Connection (Catalog Service)
```cpp
// Custom C++ implementation
// Routes requests to backend with fewest active connections
Backend* selectBackend() {
    Backend* selected = nullptr;
    int minConnections = INT_MAX;
    for (auto& backend : backends) {
        if (backend.isHealthy() && backend.activeConnections < minConnections) {
            minConnections = backend.activeConnections;
            selected = &backend;
        }
    }
    return selected;
}
```
- Use Case: Best for services with variable request processing times
- Benefit: Prevents overloading any single backend
- Result: 86.80 req/s throughput, 576ms mean latency

#### Round Robin (Customer Service)
```cpp
// Custom C++ implementation
// Distributes requests evenly across all healthy backends
Backend* selectBackend() {
    int attempts = 0;
    while (attempts < backends.size()) {
        currentIndex = (currentIndex + 1) % backends.size();
        if (backends[currentIndex].isHealthy()) {
            return &backends[currentIndex];
        }
        attempts++;
    }
    return nullptr;
}
```
- Use Case: Even distribution for general-purpose operations
- Benefit: Simple and predictable load distribution
- Result: 70.82 req/s throughput, 706ms mean latency

#### Weighted Round Robin (Order Service)
```cpp
// Custom C++ implementation
// Routes based on backend weight/priority
Backend* selectBackend() {
    for (auto& backend : backends) {
        if (backend.isHealthy()) {
            backend.currentWeight += backend.effectiveWeight;
            if (backend.currentWeight > maxWeight) {
                maxWeight = backend.currentWeight;
                selected = &backend;
            }
        }
    }
    if (selected) selected->currentWeight -= totalWeight;
    return selected;
}
```
- Use Case: Prioritized routing for transactional operations
- Benefit: Fine-grained control over traffic distribution
- Result: 15.52 req/s throughput (database-bound)

### 2. Health Checks & Failover

```cpp
// Custom C++ health check implementation
void healthCheck() {
    for (auto& backend : backends) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        struct timeval timeout = {5, 0};  // 5 second timeout
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        if (connect(sockfd, &backend.addr, sizeof(backend.addr)) == 0) {
            backend.failCount = 0;
            backend.healthy = true;
        } else {
            backend.failCount++;
            if (backend.failCount >= 3) {
                backend.healthy = false;
                backend.nextRetry = time(nullptr) + 30;  // retry after 30s
            }
        }
        close(sockfd);
    }
}
```

- Health check interval: 30 seconds
- Failure threshold: 3 consecutive failures
- Retry timeout: 30 seconds after marking unhealthy
- Automatic failover to healthy backends

Result: 0% failure rate across 60,000+ test requests

### 3. Content-Aware Routing

- Path-based routing: `/catalog/*` → Catalog service
- Host-based routing: `microservices.local` → Nginx LB
- Method-based routing: All HTTP methods supported (GET, POST, PUT, DELETE)

### 4. Observability

```cpp
// Custom metrics endpoint for Prometheus
void handleMetricsRequest(int clientSocket) {
    std::stringstream metrics;
    metrics << "# HELP cpp_lb_active_connections Currently active connections\n";
    metrics << "# TYPE cpp_lb_active_connections gauge\n";
    metrics << "cpp_lb_active_connections " << activeConnections << "\n\n";
    
    metrics << "# HELP cpp_lb_total_requests Total requests processed\n";
    metrics << "# TYPE cpp_lb_total_requests counter\n";
    metrics << "cpp_lb_total_requests " << totalRequests << "\n\n";
    
    metrics << "# HELP cpp_lb_backend_health Backend health status (1=healthy, 0=unhealthy)\n";
    metrics << "# TYPE cpp_lb_backend_health gauge\n";
    for (const auto& backend : backends) {
        metrics << "cpp_lb_backend_health{backend=\"" << backend.name 
                << "\"} " << (backend.healthy ? 1 : 0) << "\n";
    }
    
    send(clientSocket, metrics.str().c_str(), metrics.str().length(), 0);
}
```

Metrics Exposed:
- Active connections
- Total requests processed
- Backend health status per service
- Request latency percentiles
- Failed requests count

## 📊 Performance Benchmarks

### Test Environment
- Platform: Kubernetes on Minikube (Docker driver)
- Resource Limits: 100m CPU request, 500m limit | 128Mi memory request, 512Mi limit
- Tool: Apache Bench (ab)
- Total Requests Tested: 60,000+

### Results Summary

| Service | Strategy | Throughput | Mean Latency | p95 | p99 | Error Rate |
|---------|----------|------------|--------------|-----|-----|------------|
| Catalog | least_conn | 86.80 req/s | 576 ms | 1,499 ms | 2,298 ms | 0% |
| Customer | ip_hash | 70.82 req/s | 706 ms | 2,102 ms | 3,750 ms | 0% |
| Order | round_robin | 15.52 req/s | 3,221 ms | 7,996 ms | 10,602 ms | 0% |
| Catalog (High Concurrency) | least_conn | 290.99 req/s | 344 ms | 901 ms | 1,594 ms | 0% |

### Key Findings

✅ 100% Reliability: Zero failed requests across all tests  
✅ Efficient Scaling: 3.35x throughput increase with doubled concurrency  
✅ Strategy Effectiveness: Each routing strategy performs optimally for its use case  
✅ TLS Overhead: Minimal latency impact from HTTPS encryption  

📄 Detailed Report: [PERFORMANCE-BENCHMARKS.md](PERFORMANCE-BENCHMARKS.md)

## 🔄 Autoscaling

### Horizontal Pod Autoscaler (HPA) Configuration

```yaml
metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70    # Scale up if CPU > 70%
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80    # Scale up if Memory > 80%

minReplicas: 1
maxReplicas: 3
```

### Observed Behavior

Under Load (5,000 requests @ 50 concurrency):
- Catalog: CPU 250% → Scaled to 3 pods
- Customer: CPU 137% → Scaled to 2 pods
- Order: CPU 249% → Scaled to 3 pods

After Load (5 minute stabilization):
- All services scaled back to 1 pod

Scale-up Time: ~30-60 seconds  
Scale-down Time: ~5-7 minutes (stabilization window)

## 📈 Monitoring & Observability

### Prometheus Metrics

Custom C++ Load Balancer:
- `cpp_lb_active_connections` - Current active connections
- `cpp_lb_total_requests` - Total HTTP requests processed
- `cpp_lb_backend_health` - Backend health status (1=healthy, 0=unhealthy)
- `cpp_lb_request_duration_seconds` - Request latency histogram
- `cpp_lb_failed_requests_total` - Total failed requests

Kubernetes:
- `kube_pod_status_phase` - Pod status
- `kube_horizontalpodautoscaler_status_current_replicas` - HPA replicas
- `container_cpu_usage_seconds_total` - Container CPU usage
- `container_memory_usage_bytes` - Container memory usage

### Grafana Dashboards

1. C++ Load Balancer Dashboard
   - Active connections counter
   - Requests/second gauge
   - Backend health status panel
   - Request latency percentiles (p50, p95, p99)
   - Failed requests timeseries

2. Microservices Overview Dashboard
   - Running pods per service
   - CPU usage by service
   - Memory usage by service
   - HPA scaling activity

📄 Setup Guide: [monitoring/GRAFANA-SETUP.md](microservice-kubernetes-demo/monitoring/GRAFANA-SETUP.md)

## 🗄️ Database & Storage Integration

### PostgreSQL (SQL)

Catalog Database (`microservicesdb`):
```sql
-- 9 items in catalog (iPod, iPad, Apple TV, etc.)
SELECT * FROM catalog;
```

Order Database (`ordersdb`):
```sql
-- Tables: orders, order_line, ordertable_order_line
CREATE TABLE orders (
    id BIGINT PRIMARY KEY,
    customer_id BIGINT,
    order_date TIMESTAMP
);
```

### MongoDB (NoSQL)

Customer Database (`microservicesdb`):
```javascript
// 2 customers with ObjectID strings
db.customer.find()
// { _id: ObjectId("..."), name: "Eberhard Wolff", ... }
// { _id: ObjectId("..."), name: "Rod Johnson", ... }
```

### MinIO (Object Storage)

Catalog Images Bucket:
- Bucket: `catalog-images`
- Objects: `item-1.jpg`, `item-2.jpg`, ...
- API: http://minio:9000
- Console: http://minio:9001

Image Upload/Download:
```bash
# Upload via Catalog service
curl -X POST -F "file=@image.jpg" https://microservices.local:8443/catalog/items/1/image

# Download
curl https://microservices.local:8443/catalog/items/1/image -o downloaded.jpg
```

## 🚀 Deployment Guide

### Prerequisites

- Docker installed
- Kubernetes (Minikube) installed
- kubectl configured
- Helm (for Prometheus/Grafana)
- mkcert (for TLS certificates)

### Step 1: Start Kubernetes Cluster

```bash
minikube start --driver=docker --memory=8192 --cpus=4
minikube addons enable ingress
```

### Step 2: Deploy Databases

```bash
# PostgreSQL
kubectl apply -f database/postgres-deployment.yaml

# MongoDB
kubectl apply -f database/mongodb-deployment.yaml

# MinIO
kubectl apply -f storage/minio-deployment.yaml
```

### Step 3: Deploy Microservices

```bash
# Build the custom C++ load balancer
cd customlb
./build-and-deploy.sh

# Deploy microservices
kubectl apply -f microservices.yaml
```

### Step 4: Setup TLS Certificate

```bash
# Install mkcert
brew install mkcert  # macOS
# or
sudo apt install mkcert  # Linux

# Install local CA
mkcert -install

# Generate certificate
mkcert microservices.local

# Create Kubernetes secret
kubectl create secret tls microservice-tls \
    --cert=microservices.local.pem \
    --key=microservices.local-key.pem
```

### Step 5: Deploy Ingress

```bash
kubectl apply -f apache/microservice-ingress.yaml
```

### Step 6: Deploy Monitoring

```bash
# Add Helm repos
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo add grafana https://grafana.github.io/helm-charts
helm repo update

# Install Prometheus
helm install prometheus prometheus-community/prometheus

# Install Grafana
helm install grafana grafana/grafana

# C++ Load Balancer already exposes metrics on port 8081
# Prometheus will automatically scrape the /metrics endpoint
```

### Step 7: Deploy Autoscaling

```bash
kubectl apply -f autoscaling/hpa.yaml
```

### Step 8: Configure Access

```bash
# Add to /etc/hosts (Linux/Mac) or C:\Windows\System32\drivers\etc\hosts (Windows)
echo "127.0.0.1 microservices.local" | sudo tee -a /etc/hosts

# Port forward ingress controller
kubectl port-forward --namespace ingress-nginx \
    service/ingress-nginx-controller 8443:443
```

### Step 9: Verify Deployment

```bash
# Check all pods are running
kubectl get pods

# Check HPA status
kubectl get hpa

# Access application
curl -k https://microservices.local:8443/catalog/

# Access Grafana (get password first)
kubectl get secret grafana -o jsonpath="{.data.admin-password}" | base64 --decode
kubectl port-forward service/grafana 3000:80
# Open http://localhost:3000
```

## 🧪 Testing

### Manual Testing

```bash
# Test catalog service
curl -k https://microservices.local:8443/catalog/

# Test customer service
curl -k https://microservices.local:8443/customer/

# Test order service
curl -k https://microservices.local:8443/order/

# Test image upload/download
curl -X POST -F "file=@test.jpg" https://microservices.local:8443/catalog/items/1/image
curl https://microservices.local:8443/catalog/items/1/image -o downloaded.jpg
```

### Performance Benchmarking

```bash
cd microservice-kubernetes-demo

# Run full benchmark suite
./benchmark.sh

# Quick test
ab -n 1000 -c 10 -k https://microservices.local:8443/catalog/
```

### Autoscaling Demonstration

```bash
cd microservice-kubernetes-demo

# Run interactive autoscaling demo
./autoscale-demo.sh

# Or manually monitor
kubectl get hpa -w  # Watch HPA in real-time
kubectl get pods -w  # Watch pods scaling
```

### Failover Testing

```bash
# Delete a pod while under load
kubectl delete pod <catalog-pod-name>

# Verify no requests fail
ab -n 1000 -c 10 https://microservices.local:8443/catalog/
# Should see 0 failed requests
```
