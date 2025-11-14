# Multi-stage build for C++ Load Balancer

# Stage 1: Build
FROM gcc:11 AS builder

# Install cmake
RUN apt-get update && apt-get install -y cmake

# Set working directory
WORKDIR /build

# Copy source files
COPY LoadBalancer.h LoadBalancer.cpp main_new.cpp CMakeLists.txt ./

# Build the application
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Stage 2: Runtime
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && \
    apt-get install -y libstdc++6 && \
    rm -rf /var/lib/apt/lists/*

# Copy binary from builder
COPY --from=builder /build/build/loadbalancer /usr/local/bin/loadbalancer

# Create non-root user
RUN useradd -m -u 1000 loadbalancer

# Switch to non-root user
USER loadbalancer

# Expose ports
# Port 80: Main load balancer
# Port 8081: Stats/monitoring page
EXPOSE 80 8081

# Health check
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD curl -f http://localhost:80/health || exit 1

# Run the load balancer
CMD ["/usr/local/bin/loadbalancer"]
