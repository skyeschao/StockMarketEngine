# Stock Market Order Matching Engine

A C++ implementation of a stock exchange matching engine that processes buy and sell orders in real time. The system maintains an order book for multiple stocks, executes trades using price-time priority, and provides optional analytics including running trade medians, trader summaries, and optimal buy/sell timing analysis.

## Features

* **Real-time order matching**

  * Matches buy and sell orders as they arrive.
  * Supports multiple stocks and traders.
  * Executes trades using price-time priority.

* **Efficient order book**

  * Buy orders stored in a max-priority queue.
  * Sell orders stored in a min-priority queue.
  * Optimized for fast order insertion and matching.

* **Trade analytics**

  * Running median of executed trade prices.
  * Per-trader statistics:

    * Shares bought
    * Shares sold
    * Net cash transfer
  * Time traveler analysis to determine the most profitable historical buy/sell opportunity for each stock.

* **Command-line options**

  * Verbose trade output
  * Median price reporting
  * Trader information
  * Time traveler analysis

## Implementation

The project is written in modern C++ using STL containers and algorithms.

Key data structures include:

* `std::priority_queue` for buy and sell order books
* Additional priority queues for online median calculation
* Vectors for stock and trader management
* Custom comparators to enforce exchange matching rules

The matching engine prioritizes:

1. Best available price
2. Earliest submitted order when prices are equal

## Example Workflow

1. Read market configuration.
2. Process incoming buy and sell orders.
3. Match compatible orders immediately.
4. Update trader statistics.
5. Track median trade prices.
6. Generate end-of-day reports.

## Project Structure

```text
market.cpp        Main matching engine
P2random.*        Random order generation utilities
```

## Building

Compile with a C++17-compatible compiler:

```bash
g++ -std=c++17 market.cpp P2random.cpp -o market
```

## Running

```bash
./market [options] < input.txt
```

Available options:

| Option                   | Description                                       |
| ------------------------ | ------------------------------------------------- |
| `-v`, `--verbose`        | Print every executed trade                        |
| `-m`, `--median`         | Output running median trade prices                |
| `-i`, `--trader_info`    | Display trader statistics                         |
| `-t`, `--time_travelers` | Display optimal historical buy/sell opportunities |
| `-h`, `--help`           | Show usage information                            |

## Performance

The matching engine is designed around priority queues, allowing order insertion and matching operations to scale efficiently while maintaining correct exchange ordering rules. Median tracking is performed online without storing all historical trade prices, keeping memory usage low even for large input streams.

## Technologies

* C++
* Standard Template Library (STL)
* Priority Queues
* Custom Comparators
* Command-Line Interface
* Online Median Algorithm
