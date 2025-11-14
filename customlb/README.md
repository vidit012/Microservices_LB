# Custom C++ Load Balancer

A high-performance, production-ready load balancer written in C++ that replaces Nginx in the Kubernetes microservices architecture.

## Features

### Load Balancing Algorithms
- **Round Robin** - Distributes requests evenly across backends (Order service)
- **Least Connections** - Routes to backend with fewest active connections (Catalog service)
- **IP Hash** - Session persistence using client IP hashing (Customer service)

### Advanced Features
- ✅ **Path-based Routing** - Route `/catalog/`, `/customer/`, `/order/` to different services
- ✅ **Health Checks** - Automatic backend health monitoring every 30 seconds
- ✅ **Failover** - Automatically retry failed requests on different backends (max 3 attempts)
- ✅ **Connection Pooling** - Efficient connection management
- ✅ **HTTP Proxy** - Full HTTP/1.1 proxy with header forwarding
- ✅ **Monitoring** - Real-time statistics dashboard on port 8081
- ✅ **Graceful Degradation** - max_fails=3, fail_timeout=30s per backend
- ✅ **Multi-threaded** - Handle concurrent requests efficiently
- ✅ **Request Logging** - Detailed access logs with timestamps

## Architecture

```
Client Request
     ↓
Ingress Controller (HTTPS termination)
     ↓
Custom C++ Load Balancer (Port 80)
     ↓
Path-based Routing:
  /customer/ → IP Hash      → customer:8080
  /catalog/  → Least Conn   → catalog:8080
  /order/    → Round Robin  → order:8080
```

## Building

### Prerequisites
- CMake 3.10+
- GCC 11+ (C++17 support)
- Docker (for containerization)
- Kubernetes cluster

### Local Build

```bash
mkdir build && cd build
cmake ..
make
./loadbalancer
```

### Docker Build

```bash
docker build -t vidit12/cpp-loadbalancer:latest .
docker push vidit12/cpp-loadbalancer:latest
```

### Quick Deploy

```bash
chmod +x build-and-deploy.sh
./build-and-deploy.sh
```

## Deployment to Kubernetes

### Step 1: Build and Push Image

```bash
cd /home/vidit-pt7945/microservice-kubernetes/customlb
docker build -t vidit12/cpp-loadbalancer:latest .
docker push vidit12/cpp-loadbalancer:latest
```

### Step 2: Deploy to Kubernetes

```bash
kubectl apply -f cpp-loadbalancer-deployment.yaml
```

### Step 3: Update Ingress

Update your `microservice-ingress.yaml` to point to the new load balancer:

```yaml
backend:
  service:
    name: cpp-loadbalancer-service  # Changed from nginx-loadbalancer-service
    port:
      number: 80
```

Apply the change:

```bash
kubectl apply -f microservice-ingress.yaml
```

### Step 4: Verify Deployment

```bash
# Check pod status
kubectl get pods -l app=cpp-loadbalancer

# Check service
kubectl get svc cpp-loadbalancer-service

# View logs
kubectl logs -l app=cpp-loadbalancer -f
```

## Testing

### Test Load Balancing

```bash
# Test catalog service
curl -k https://microservices.local:8443/catalog/

# Test customer service
curl -k https://microservices.local:8443/customer/

# Test order service
curl -k https://microservices.local:8443/order/
```

### View Statistics

```bash
# Port forward stats service
kubectl port-forward svc/cpp-loadbalancer-stats 8081:8081

# Open in browser
open http://localhost:8081/nginx_status
```

### Health Check

```bash
kubectl port-forward svc/cpp-loadbalancer-service 8080:80
curl http://localhost:8080/health
```

## Configuration

The load balancer is configured in `main_new.cpp`:

```cpp
// Customer Service - IP Hash (Session Persistence)
lb->addService("/customer/", LoadBalancingAlgorithm::IP_HASH);
lb->addBackendToService("/customer/", "customer-1", "customer", 8080, 3, 30);

// Catalog Service - Least Connections
lb->addService("/catalog/", LoadBalancingAlgorithm::LEAST_CONNECTIONS);
lb->addBackendToService("/catalog/", "catalog-1", "catalog", 8080, 3, 30);

// Order Service - Round Robin
lb->addService("/order/", LoadBalancingAlgorithm::ROUND_ROBIN);
lb->addBackendToService("/order/", "order-1", "order", 8080, 3, 30);
```

Parameters:
- `maxFails`: Maximum consecutive failures before marking backend as DOWN (default: 3)
- `failTimeout`: Seconds to wait before retrying a failed backend (default: 30)

## Monitoring

### Statistics Dashboard

The load balancer exposes a statistics dashboard at `/nginx_status` on port 8081:

**Metrics Available:**
- Total requests processed
- Failed requests
- Success rate percentage
- Bytes received/sent
- Backend health status
- Active connections per backend
- Consecutive failures per backend

### Access Stats Dashboard

```bash
# Via NodePort (if available)
curl http://$(minikube ip):30081/nginx_status

# Via Port Forward
kubectl port-forward svc/cpp-loadbalancer-stats 8081:8081
open http://localhost:8081/nginx_status
```

### Request Logs

```bash
kubectl logs -l app=cpp-loadbalancer -f
```

Example log output:
```
[2025-10-14 10:30:45] 192.168.1.100 "GET /catalog/" 200 backend=catalog-1
[2025-10-14 10:30:46] 192.168.1.101 "GET /customer/" 200 backend=customer-1
[2025-10-14 10:30:47] 192.168.1.100 "POST /order/" 200 backend=order-1
```

## Performance

### Resource Usage
- **CPU**: 200m request, 500m limit
- **Memory**: 256Mi request, 512Mi limit
- **Threads**: One per concurrent connection
- **Health Check**: Every 30 seconds (low overhead)

### Benchmarking

```bash
# Use Apache Bench
ab -n 1000 -c 10 -k https://microservices.local:8443/catalog/

# Use the existing benchmark script
cd /home/vidit-pt7945/microservice-kubernetes/microservice-kubernetes-demo
./benchmark.sh
```

## Comparison with Nginx

| Feature | Nginx | Custom C++ LB |
|---------|-------|---------------|
| Load Balancing Algorithms | ✅ | ✅ |
| Health Checks | ✅ | ✅ |
| Path-based Routing | ✅ | ✅ |
| Session Persistence | ✅ | ✅ |
| Request Retries | ✅ | ✅ |
| Real-time Stats | ✅ | ✅ |
| **Customization** | Limited | **Full Control** |
| **Code Understanding** | Config only | **Complete Source** |
| **Binary Size** | ~30MB | **~500KB** |
| **Custom Logic** | Complex | **Easy to Add** |

## Advantages of C++ Implementation

1. **Full Control** - Complete control over load balancing logic
2. **Lightweight** - Smaller binary size and memory footprint
3. **Learning** - Understand exactly how load balancing works
4. **Customizable** - Easy to add custom features (authentication, rate limiting, etc.)
5. **Performance** - Native C++ performance with minimal overhead
6. **No External Dependencies** - Only standard C++ library

## Troubleshooting

### Pod Not Starting

```bash
# Check pod events
kubectl describe pod -l app=cpp-loadbalancer

# Check logs
kubectl logs -l app=cpp-loadbalancer
```

### Backend Connection Failures

```bash
# Check backend service names resolve
kubectl exec -it <cpp-loadbalancer-pod> -- nslookup customer
kubectl exec -it <cpp-loadbalancer-pod> -- nslookup catalog
kubectl exec -it <cpp-loadbalancer-pod> -- nslookup order
```

### Health Check Failing

The load balancer performs TCP health checks on backends. Ensure:
- Backend services are running
- Backend services accept connections on port 8080
- Network policies allow traffic from load balancer to backends

## Future Enhancements

Potential features to add:
- [ ] TLS/SSL support for backend connections
- [ ] Rate limiting per client IP
- [ ] Request/response caching
- [ ] WebSocket support
- [ ] HTTP/2 support
- [ ] Prometheus metrics export
- [ ] Configuration file support (YAML/JSON)
- [ ] Dynamic backend discovery (Kubernetes API)
- [ ] Circuit breaker pattern
- [ ] Request/response transformation

## Files

```
customlb/
├── LoadBalancer.h                      # Header file with class definitions
├── LoadBalancer.cpp                    # Implementation
├── main_new.cpp                        # Entry point with configuration
├── CMakeLists.txt                      # Build configuration
├── Dockerfile                          # Multi-stage Docker build
├── cpp-loadbalancer-deployment.yaml    # Kubernetes deployment
├── build-and-deploy.sh                 # Automated build/deploy script
└── README.md                           # This file
```

## License

MIT License

## Author

Vidit - Custom load balancer implementation for microservices architecture

## Acknowledgments

- Replaces Nginx with custom C++ implementation
- Designed for Kubernetes microservices environment
- Implements production-ready load balancing patterns
