#!/bin/bash
# Performance Benchmarking Script for Microservices Load Balancer
# This script runs comprehensive load tests on all microservices

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
BASE_URL="https://microservices.local:8443"
RESULTS_DIR="./benchmark-results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Create results directory
mkdir -p "$RESULTS_DIR"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Microservices Load Balancer Benchmark${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Timestamp: $TIMESTAMP"
echo "Base URL: $BASE_URL"
echo "Results Directory: $RESULTS_DIR"
echo ""

# Function to run benchmark test
run_benchmark() {
    local service_name=$1
    local endpoint=$2
    local requests=$3
    local concurrency=$4
    local output_file="${RESULTS_DIR}/${service_name}_${TIMESTAMP}.txt"
    
    echo -e "${YELLOW}Testing: $service_name${NC}"
    echo "  Endpoint: $endpoint"
    echo "  Requests: $requests"
    echo "  Concurrency: $concurrency"
    echo ""
    
    ab -n "$requests" -c "$concurrency" -k -s 30 "$endpoint" > "$output_file" 2>&1
    
    # Extract key metrics
    echo "  Results:"
    grep "Requests per second:" "$output_file" | awk '{print "  - Throughput: " $4 " req/s"}'
    grep "Time per request:" "$output_file" | head -1 | awk '{print "  - Mean Latency: " $4 " ms"}'
    grep "Failed requests:" "$output_file" | awk '{print "  - Failed: " $3}'
    grep "50%" "$output_file" | awk '{print "  - p50: " $2 " ms"}'
    grep "95%" "$output_file" | awk '{print "  - p95: " $2 " ms"}'
    grep "99%" "$output_file" | awk '{print "  - p99: " $2 " ms"}'
    echo ""
}

# Test 1: Catalog Service (Least Connection Load Balancing)
echo -e "${GREEN}=== Test 1: Catalog Service ===${NC}"
run_benchmark "catalog" "${BASE_URL}/catalog/" 5000 50

# Test 2: Customer Service (IP Hash / Sticky Sessions)
echo -e "${GREEN}=== Test 2: Customer Service ===${NC}"
run_benchmark "customer" "${BASE_URL}/customer/" 5000 50

# Test 3: Order Service (Round Robin)
echo -e "${GREEN}=== Test 3: Order Service ===${NC}"
run_benchmark "order" "${BASE_URL}/order/" 5000 50

# Test 4: High Concurrency Test (Catalog)
echo -e "${GREEN}=== Test 4: High Concurrency (Catalog) ===${NC}"
run_benchmark "catalog_high_concurrency" "${BASE_URL}/catalog/" 10000 100

# Test 5: Sustained Load Test (5 minutes)
echo -e "${GREEN}=== Test 5: Sustained Load Test ===${NC}"
echo "Running 5-minute sustained load test on all services..."
echo "This will help test autoscaling behavior"
echo ""

# Start background load generators
ab -n 30000 -c 20 -k -s 30 "${BASE_URL}/catalog/" > "${RESULTS_DIR}/sustained_catalog_${TIMESTAMP}.txt" 2>&1 &
PID_CATALOG=$!

ab -n 30000 -c 20 -k -s 30 "${BASE_URL}/customer/" > "${RESULTS_DIR}/sustained_customer_${TIMESTAMP}.txt" 2>&1 &
PID_CUSTOMER=$!

ab -n 30000 -c 20 -k -s 30 "${BASE_URL}/order/" > "${RESULTS_DIR}/sustained_order_${TIMESTAMP}.txt" 2>&1 &
PID_ORDER=$!

echo "Load tests running in background..."
echo "PIDs: Catalog=$PID_CATALOG, Customer=$PID_CUSTOMER, Order=$PID_ORDER"
echo ""
echo "Monitor with:"
echo "  kubectl get hpa -w"
echo "  kubectl get pods"
echo ""
echo "Waiting for tests to complete..."

# Wait for all background jobs
wait $PID_CATALOG $PID_CUSTOMER $PID_ORDER

echo -e "${GREEN}Sustained load test completed!${NC}"
echo ""

# Generate summary report
SUMMARY_FILE="${RESULTS_DIR}/summary_${TIMESTAMP}.txt"

echo "========================================" > "$SUMMARY_FILE"
echo "  Performance Benchmark Summary" >> "$SUMMARY_FILE"
echo "========================================" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"
echo "Date: $(date)" >> "$SUMMARY_FILE"
echo "Base URL: $BASE_URL" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"

for service in catalog customer order catalog_high_concurrency; do
    file="${RESULTS_DIR}/${service}_${TIMESTAMP}.txt"
    if [ -f "$file" ]; then
        echo "--- $service ---" >> "$SUMMARY_FILE"
        grep "Requests per second:" "$file" >> "$SUMMARY_FILE"
        grep "Time per request:" "$file" | head -1 >> "$SUMMARY_FILE"
        grep "Failed requests:" "$file" >> "$SUMMARY_FILE"
        grep "50%\|95%\|99%" "$file" >> "$SUMMARY_FILE"
        echo "" >> "$SUMMARY_FILE"
    fi
done

echo "" >> "$SUMMARY_FILE"
echo "Current Pod Status:" >> "$SUMMARY_FILE"
kubectl get pods -o wide >> "$SUMMARY_FILE" 2>&1

echo "" >> "$SUMMARY_FILE"
echo "HPA Status:" >> "$SUMMARY_FILE"
kubectl get hpa >> "$SUMMARY_FILE" 2>&1

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Benchmark Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Results saved to: $RESULTS_DIR"
echo "Summary: $SUMMARY_FILE"
echo ""
echo "View results:"
echo "  cat $SUMMARY_FILE"
echo ""
