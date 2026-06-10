# C++ Order Management System (OMS)

A high-performance **Order Management System (OMS)** written in **C++17** that consumes real-time market data from Binance, maintains an in-memory order book, synchronizes snapshots with streaming updates, executes orders, exposes REST APIs, and provides logging and metrics for monitoring.

---

## Features

* Real-time Binance WebSocket market data streaming
* Snapshot synchronization using REST API
* In-memory Order Book
* Sequence number validation and replay mechanism
* Order Management

  * New Orders
  * Open Orders
  * Partial Filled Orders
  * Filled Orders
  * Cancel Orders
* Execution Engine
* REST API Server
* Metrics Collection
* Asynchronous Logging using spdlog
* Thread-safe architecture

---

## Project Structure

```text
.
├── include/
│   ├── order_book.h
│   ├── websocket_client.h
│   ├── snapshot_manager.h
│   ├── sync_engine.h
│   ├── order_manager.h
│   ├── execution_engine.h
│   ├── api_server.h
│   ├── metrics_collector.h
│   └── async_logger.h
│
├── src/
│   ├── main.cpp
│   ├── order_book.cpp
│   ├── websocket_client.cpp
│   ├── snapshot_manager.cpp
│   ├── sync_engine.cpp
│   ├── order_manager.cpp
│   ├── execution_engine.cpp
│   ├── api_server.cpp
│   ├── metrics_collector.cpp
│   └── async_logger.cpp
│
└── CMakeLists.txt
```

---

## Architecture

```text
                   Binance

          REST API       WebSocket
              |               |
              |               |
      +-------------------------------+
      |       Snapshot Manager        |
      +-------------------------------+
                     |
                     v

      +-------------------------------+
      |         Sync Engine           |
      | Sequence Validation           |
      | Replay Buffered Updates       |
      +-------------------------------+
                     |
                     v

      +-------------------------------+
      |          Order Book           |
      | Bids / Asks                   |
      +-------------------------------+
                     |
          --------------------
          |                  |
          v                  v

+----------------+   +------------------+
| Order Manager  |   | Execution Engine |
+----------------+   +------------------+

          |
          v

+--------------------+
| REST API Server    |
+--------------------+

          |
          v

+--------------------+
| Metrics + Logging  |
+--------------------+
```

---

## Market Data Flow

1. Connect to Binance WebSocket stream.
2. Buffer incoming depth updates.
3. Fetch the latest snapshot through REST API.
4. Validate sequence numbers:

   * Ignore old updates.
   * Replay buffered updates.
   * Apply new updates sequentially.
5. Update in-memory order book.

---

## Order Lifecycle

```text
NEW
 |
 v
OPEN
 |
 +------------------+
 |                  |
 v                  v
PARTIAL FILLED     CANCELLED
 |
 v
FILLED
```

---

## Technologies Used

* C++17
* CMake
* WebSocket Client
* OpenSSL
* httplib
* nlohmann/json
* spdlog
* std::thread
* Mutex / Synchronization Primitives

---

## Build

```bash
mkdir build
cd build

cmake ..
make
```

---

## Run

```bash
./orderbook
```

---

## Logging

The system maintains logs for:

* Order Book Updates
* New Orders
* Open Orders
* Partial Filled Orders
* Filled Orders
* Cancelled Orders
* Execution Events
* Snapshot Synchronization
* WebSocket Events

---

## Performance Considerations

* In-memory data structures for low latency
* Buffered replay mechanism for snapshot synchronization
* Asynchronous logging to reduce I/O overhead
* Thread-safe shared resources
* Efficient sequence number validation

---

## Future Enhancements

* Matching Engine
* Multi-symbol support
* Lock-free queues
* Persistence Layer
* Market Making Strategies
* Latency Benchmarking

---

## Author

**Bhargav Sasi Chandan**

Passionate about:

* Low Latency Systems
* Order Management Systems
* Market Microstructure
* Quantitative Development
* AWS Cloud Architecture
* High Performance C++

##
