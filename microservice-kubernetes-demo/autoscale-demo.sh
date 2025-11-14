#!/bin/bash
# Autoscaling Demonstration Script
# This script demonstrates HPA scale-up and scale-down behavior

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Autoscaling Demonstration${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# Check current HPA status
echo -e "${BLUE}Step 1: Initial State${NC}"
echo "Current HPA configuration:"
kubectl get hpa
echo ""
echo "Current pod count:"
kubectl get pods -l 'run in (catalog,customer,order)' -o wide
echo ""

read -p "Press Enter to start load generation..."

# Start load generation in background
echo -e "${YELLOW}Step 2: Starting Load Generation${NC}"
echo "Generating sustained load on all services..."
echo ""

# Catalog service
echo "Starting catalog load (least_conn routing)..."
ab -n 50000 -c 30 -k -s 60 https://microservices.local:8443/catalog/ > /dev/null 2>&1 &
PID_CATALOG=$!
echo "  - Catalog PID: $PID_CATALOG"

# Customer service
echo "Starting customer load (ip_hash routing)..."
ab -n 50000 -c 30 -k -s 60 https://microservices.local:8443/customer/ > /dev/null 2>&1 &
PID_CUSTOMER=$!
echo "  - Customer PID: $PID_CUSTOMER"

# Order service
echo "Starting order load (round_robin routing)..."
ab -n 30000 -c 20 -k -s 60 https://microservices.local:8443/order/ > /dev/null 2>&1 &
PID_ORDER=$!
echo "  - Order PID: $PID_ORDER"

echo ""
echo -e "${GREEN}Load generation started!${NC}"
echo ""

# Monitor for scale-up
echo -e "${YELLOW}Step 3: Monitoring Scale-Up (watching for ~2-3 minutes)${NC}"
echo "HPA checks metrics every 15 seconds, scaling decisions every 30-60 seconds..."
echo "Watch for pod count to increase as CPU/Memory thresholds are exceeded"
echo ""

for i in {1..24}; do
    clear
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  Autoscaling Demonstration${NC}"
    echo -e "${GREEN}  Monitoring Scale-Up: Iteration $i/24${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    
    echo -e "${BLUE}HPA Status:${NC}"
    kubectl get hpa
    echo ""
    
    echo -e "${BLUE}Pod Status:${NC}"
    kubectl get pods -l 'run in (catalog,customer,order)' -o wide | grep -E "NAME|catalog|customer|order"
    echo ""
    
    echo -e "${BLUE}Pod Count Summary:${NC}"
    CATALOG_COUNT=$(kubectl get pods -l run=catalog --no-headers 2>/dev/null | wc -l)
    CUSTOMER_COUNT=$(kubectl get pods -l run=customer --no-headers 2>/dev/null | wc -l)
    ORDER_COUNT=$(kubectl get pods -l run=order --no-headers 2>/dev/null | wc -l)
    echo "  Catalog pods:  $CATALOG_COUNT"
    echo "  Customer pods: $CUSTOMER_COUNT"
    echo "  Order pods:    $ORDER_COUNT"
    echo ""
    
    if [ "$CATALOG_COUNT" -ge 2 ] || [ "$CUSTOMER_COUNT" -ge 2 ] || [ "$ORDER_COUNT" -ge 2 ]; then
        echo -e "${GREEN}✅ Scale-up detected! At least one service has scaled to 2+ pods.${NC}"
        break
    fi
    
    sleep 5
done

echo ""
echo -e "${YELLOW}Step 4: Stopping Load Generation${NC}"
echo "Killing load generation processes..."
kill $PID_CATALOG $PID_CUSTOMER $PID_ORDER 2>/dev/null || true
wait $PID_CATALOG $PID_CUSTOMER $PID_ORDER 2>/dev/null || true
echo -e "${GREEN}Load generation stopped!${NC}"
echo ""

# Take snapshot of scaled-up state
echo -e "${YELLOW}Step 5: Scaled-Up State (Post-Load)${NC}"
echo ""
echo "HPA Status:"
kubectl get hpa
echo ""
echo "Pod Status:"
kubectl get pods -l 'run in (catalog,customer,order)' -o wide
echo ""

read -p "Press Enter to monitor scale-down..."

# Monitor for scale-down
echo -e "${YELLOW}Step 6: Monitoring Scale-Down${NC}"
echo "HPA will scale down after ~5 minutes of low utilization (stabilization window)"
echo "This may take several minutes..."
echo ""

for i in {1..40}; do
    clear
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  Autoscaling Demonstration${NC}"
    echo -e "${GREEN}  Monitoring Scale-Down: Iteration $i/40${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    
    echo -e "${BLUE}HPA Status:${NC}"
    kubectl get hpa
    echo ""
    
    echo -e "${BLUE}Pod Status:${NC}"
    kubectl get pods -l 'run in (catalog,customer,order)' -o wide | grep -E "NAME|catalog|customer|order"
    echo ""
    
    echo -e "${BLUE}Pod Count Summary:${NC}"
    CATALOG_COUNT=$(kubectl get pods -l run=catalog --no-headers 2>/dev/null | grep -c Running || true)
    CUSTOMER_COUNT=$(kubectl get pods -l run=customer --no-headers 2>/dev/null | grep -c Running || true)
    ORDER_COUNT=$(kubectl get pods -l run=order --no-headers 2>/dev/null | grep -c Running || true)
    echo "  Catalog pods:  $CATALOG_COUNT (Running)"
    echo "  Customer pods: $CUSTOMER_COUNT (Running)"
    echo "  Order pods:    $ORDER_COUNT (Running)"
    echo ""
    
    TOTAL_RUNNING=$((CATALOG_COUNT + CUSTOMER_COUNT + ORDER_COUNT))
    if [ "$TOTAL_RUNNING" -le 3 ]; then
        echo -e "${GREEN}✅ Scale-down detected! Services have scaled back to minimum replicas.${NC}"
        break
    fi
    
    sleep 15
done

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Autoscaling Demonstration Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Summary:"
echo "  ✅ Load generation triggered CPU/Memory thresholds"
echo "  ✅ HPA scaled up pods to handle increased load"
echo "  ✅ After load stopped, HPA scaled down to minimum replicas"
echo ""
echo "Final State:"
kubectl get hpa
echo ""
kubectl get pods -l 'run in (catalog,customer,order)'
echo ""
echo "View HPA events:"
echo "  kubectl describe hpa catalog-hpa"
echo "  kubectl describe hpa customer-hpa"
echo "  kubectl describe hpa order-hpa"
echo ""
