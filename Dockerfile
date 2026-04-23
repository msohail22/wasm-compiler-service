FROM ubuntu:22.04

# Install compiler + build tools
RUN apt-get update && \
    apt-get install -y clang build-essential && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY main.cpp .

# Compile the server
RUN g++ -O2 main.cpp -o server

EXPOSE 8080

CMD ["./server"]