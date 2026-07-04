// Project identifier: 0E04A31E0D60C01986ACB20081C9D8722A2519B6

#include <queue>
#include <iostream>
#include <getopt.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <sstream>
#include "P2random.h"
#include <algorithm>


using namespace std;

struct Order{
    uint32_t id;
    uint32_t traderID;
    uint32_t price;
    mutable uint32_t quantity;
};

//buyer seller comparitors
struct BuyerCmp{
    bool operator()(const Order&a, const Order&b) const {
        if(a.price != b.price) return a.price < b.price; // max heap by price
        return a.id > b.id;
    }
};

struct SellerCmp{
    bool operator()(const Order&a, const Order&b) const {
        if(a.price != b.price) return a.price > b.price; // min heap by price
        return a.id > b.id;
    }
};

enum class TimeStatus{NoTrades, CanBuy, Completed, Potential};

struct Stock{
    priority_queue<Order, vector<Order>, BuyerCmp> buyers;
    priority_queue<Order, vector<Order>, SellerCmp> sellers;

    // for median
    priority_queue<uint32_t> lesserHalf;
    priority_queue<uint32_t, vector<uint32_t>, greater<uint32_t>> greaterHalf;

    // time traveler
    TimeStatus status = TimeStatus::NoTrades;
    uint32_t buy_price, buy_time;
    uint32_t sell_price, sell_time;
    uint32_t pbuy_price, pbuy_time;
};

struct Trader{
    uint32_t bought = 0;
    uint32_t sold = 0;
    int net = 0;
};

struct Options{
    bool verbose = false;
    bool median = false;
    bool trader = false;
    bool time_traveler = false;
};

void printHelp(char *command){
    cout << "Usage: " << command << "[Options] << infile >> outfile\n";
    cout << "This is an emulation of an electronic stock exchange market\n\n";

    cout << "Options: \n";
    cout << "--help/-h              Prints out this message\n";
    cout << "--verbose/-v           Indicates verbose output should be generated\n";
    cout << "--median/-m            Indicates median output should be generated\n";
    cout << "--trader_info/-i       Indicates that trader details output should be generated\n";
    cout << "--time_travelers/-t    Indicates time travelers' output should be generated\n";
}

static void getOptions(int argc, char* argv[], Options &options){
    opterr = static_cast<int>(false);
    int choice = 0;
    int index = 0;

    option longOptions[] = {
        {"help", no_argument, nullptr, 'h'},
        {"verbose", no_argument, nullptr, 'v'},
        {"median", no_argument, nullptr, 'm'},
        {"trader_info", no_argument, nullptr, 'i'},
        {"time_travelers", no_argument, nullptr, 't'},
        {nullptr, no_argument, nullptr, '\0'}
    };

    while((choice = getopt_long(argc, argv, "hvmit", longOptions, &index)) != -1){
        switch(choice){
            case 'h':
                printHelp(*argv);
                exit(0);

            case 'v':
                options.verbose = true;
                break;

            case 'm':
                options.median = true;
                break;

            case 'i':
                options.trader = true;
                break;

            case 't':
                options.time_traveler = true;
                break;
            
            default:
                cerr << "Unknown command line option\n" << flush;
                exit(1);
        }
    }

}

void median_output(vector<Stock> &stocks, int &time){
    for(size_t i = 0; i < stocks.size(); i++){
        int median;
        if(stocks[i].lesserHalf.empty() && stocks[i].greaterHalf.empty()) continue;
        
        if((stocks[i].lesserHalf.size()+stocks[i].greaterHalf.size())%2 == 0){ 
            while(stocks[i].lesserHalf.size() > stocks[i].greaterHalf.size()){
                stocks[i].greaterHalf.push(stocks[i].lesserHalf.top());
                stocks[i].lesserHalf.pop();
            }
            while(stocks[i].lesserHalf.size() < stocks[i].greaterHalf.size()){
                stocks[i].lesserHalf.push(stocks[i].greaterHalf.top());
                stocks[i].greaterHalf.pop();
            }
            median = (stocks[i].lesserHalf.top() + stocks[i].greaterHalf.top())/2;
        } else {
            while(stocks[i].lesserHalf.size() > stocks[i].greaterHalf.size()+1){
                stocks[i].greaterHalf.push(stocks[i].lesserHalf.top());
                stocks[i].lesserHalf.pop();
            }
            while(stocks[i].lesserHalf.size()+1 < stocks[i].greaterHalf.size()){
                stocks[i].lesserHalf.push(stocks[i].greaterHalf.top());
                stocks[i].greaterHalf.pop();
            }
            if(stocks[i].greaterHalf.size()>stocks[i].lesserHalf.size())
                median = static_cast<int>(stocks[i].greaterHalf.top());
            else
                median = static_cast<int>(stocks[i].lesserHalf.top());

        }
        cout << "Median match price of Stock " << i << " at time " << time
        << " is $" << median << "\n";
    }
}

void verbose_output( Order& buyer,  Order& seller, int& stockID, bool buyerFirst){
    cout << "Trader " << buyer.traderID << " purchased " << min(buyer.quantity, seller.quantity)
    << " shares of Stock " << stockID << " from Trader " << seller.traderID << " for $";
    if(buyerFirst) cout << buyer.price;
    else cout << seller.price;
    cout << "/share\n";

}

void add_order(string& buysell, Order &o, Stock& s){
    if(buysell == "BUY") { //buy
        s.buyers.push(o);
    } else { // sell
        s.sellers.push(o);
    }
}

void make_trade(string& buysell, Order& order, vector<Stock>& stocks, 
    int& stockID, vector<Trader> &traders, Options& options, uint32_t &trade_count){

    Stock& stock = stocks[static_cast<size_t>(stockID)];
    if(buysell == "BUY") {
        while(!stock.sellers.empty() && order.quantity > 0 && stock.sellers.top().price <= order.price){
             Order temp_seller = stock.sellers.top();
            if(temp_seller.quantity > order.quantity){
                if(options.verbose) verbose_output(order, temp_seller, stockID, false);
                temp_seller.quantity -= order.quantity;
                if(options.trader){
                    traders[static_cast<size_t>(order.traderID)].bought += order.quantity;
                    traders[static_cast<size_t>(temp_seller.traderID)].sold += order.quantity;
                    traders[static_cast<size_t>(order.traderID)].net -= temp_seller.price * order.quantity;
                    traders[static_cast<size_t>(temp_seller.traderID)].net += temp_seller.price*order.quantity;
                }
                order.quantity = 0;
                stock.sellers.pop();
                stock.sellers.push(temp_seller);

            } else if(temp_seller.quantity < order.quantity){
                if(options.verbose) verbose_output(order, temp_seller, stockID, false);

                order.quantity -= temp_seller.quantity;
                stock.sellers.pop();

                if(options.trader){
                    traders[static_cast<size_t>(order.traderID)].bought += temp_seller.quantity;
                    traders[static_cast<size_t>(temp_seller.traderID)].sold += temp_seller.quantity;
                    traders[static_cast<size_t>(order.traderID)].net -= temp_seller.price * temp_seller.quantity;
                    traders[static_cast<size_t>(temp_seller.traderID)].net += temp_seller.price * temp_seller.quantity;
                }
                
            } else {
                if(options.verbose) verbose_output(order, temp_seller, stockID, false);
                if(options.trader){
                    traders[static_cast<size_t>(order.traderID)].bought += order.quantity;
                    traders[static_cast<size_t>(temp_seller.traderID)].sold += order.quantity;
                    traders[static_cast<size_t>(order.traderID)].net -= temp_seller.price*order.quantity;
                    traders[static_cast<size_t>(temp_seller.traderID)].net += temp_seller.price*order.quantity;
                }

                order.quantity = 0; 
                stock.sellers.pop();


            }
            if(options.median){
                if(stock.lesserHalf.empty() || temp_seller.price <= stock.lesserHalf.top()){
                    stock.lesserHalf.push(temp_seller.price);
                } else {
                    stock.greaterHalf.push(temp_seller.price);
                }
            }
            trade_count++;
        }
        if(order.quantity > 0){
            add_order(buysell, order, stock);
        }
    } else {
        while(!stock.buyers.empty() && order.quantity > 0 && stock.buyers.top().price >= order.price){
            Order temp_buyer = stock.buyers.top();
            if(temp_buyer.quantity > order.quantity){
                if(options.verbose) verbose_output(temp_buyer, order, stockID, true);

                temp_buyer.quantity -= order.quantity;
                if(options.trader){
                    traders[static_cast<size_t>(order.traderID)].sold += order.quantity;
                    traders[static_cast<size_t>(temp_buyer.traderID)].bought += order.quantity;
                    traders[static_cast<size_t>(order.traderID)].net += temp_buyer.price*order.quantity;
                    traders[static_cast<size_t>(temp_buyer.traderID)].net -= temp_buyer.price*order.quantity;
                }
                order.quantity = 0; 
                stock.buyers.pop();
                stock.buyers.push(temp_buyer);

            } else if(temp_buyer.quantity < order.quantity){
                if(options.verbose) verbose_output(temp_buyer, order, stockID, true);

                order.quantity -= temp_buyer.quantity;
                stock.buyers.pop();

                if(options.trader){
                    traders[static_cast<size_t>(order.traderID)].sold += temp_buyer.quantity;
                    traders[static_cast<size_t>(temp_buyer.traderID)].bought += temp_buyer.quantity;
                    traders[static_cast<size_t>(order.traderID)].net += temp_buyer.price*temp_buyer.quantity;
                    traders[static_cast<size_t>(temp_buyer.traderID)].net -= temp_buyer.price* temp_buyer.quantity;
                }
            } else {
                if(options.verbose) verbose_output(temp_buyer, order, stockID, true);
                if(options.trader){                
                    traders[static_cast<size_t>(order.traderID)].sold += order.quantity;
                    traders[static_cast<size_t>(temp_buyer.traderID)].bought += order.quantity;
                    traders[static_cast<size_t>(order.traderID)].net += temp_buyer.price*order.quantity;
                    traders[static_cast<size_t>(temp_buyer.traderID)].net -= temp_buyer.price*order.quantity;
                }
                order.quantity = 0; 
                stock.buyers.pop();
            }
            if(options.median){
                if(stock.lesserHalf.empty() || temp_buyer.price <= stock.lesserHalf.top()){
                    stock.lesserHalf.push(temp_buyer.price);
                } else {
                    stock.greaterHalf.push(temp_buyer.price);
                }
            }
            trade_count++;

        }
        if(order.quantity > 0){
            add_order(buysell, order, stock);
        }
    }

}

void processOrders(istream &is, int &num_stocks, int &num_traders,
    vector<Stock> &stocks, vector<Trader> &traders, Options& options, uint32_t &trade_count){
    stocks.resize(static_cast<size_t>(num_stocks));
    if(options.trader) traders.resize(static_cast<size_t>(num_traders));

    Order order;
    int CURRENT_TIMESTAMP = 0;

    char trader_char,stock_char,p_char,q_char;
    string buysell;
    uint32_t curr_id = 0;
    int timestamp, stockID;
    int traderID_t, price_t, quant_t;
    while(is>> timestamp >> buysell >>
        trader_char>>traderID_t>>
        stock_char>>stockID>>
        p_char >> price_t >>
        q_char >> quant_t){ // for each order

        order.id = curr_id;
        curr_id++;

        // TL input errors
        if(timestamp < 0){
            cerr << "Timestamps cannot be negative";
            exit(1);
        }
        if(timestamp < CURRENT_TIMESTAMP){
            cerr << "Timestamps cannot be descending.\n";
            exit(1);
        }
        if(traderID_t >= num_traders || traderID_t < 0){
            cerr << "Trader ID is out of bounds.\n";
            exit(1);
        }
        if(stockID >= num_stocks || stockID < 0){
            cerr << "Stock ID is out of bounds.\n";
            exit(1);
        }
        if(price_t <= 0 || quant_t <= 0){
            cerr << "Price and Quantity must both be positive";
            exit(1);
        }

        order.traderID = static_cast<uint32_t>(traderID_t);
        order.price = static_cast<uint32_t>(price_t);
        order.quantity = static_cast<uint32_t>(quant_t);

        
        // time travel
        if(options.time_traveler){
            Stock &stock = stocks[static_cast<size_t>(stockID)];
            if(stock.status == TimeStatus::NoTrades){
                if(buysell == "SELL"){ //SELL
                    stock.status = TimeStatus::CanBuy;
                    stock.buy_price = order.price;
                    stock.buy_time = static_cast<uint32_t>(timestamp);
                }
            } else if(stock.status == TimeStatus::CanBuy){
                if(buysell == "SELL"){ //SELL
                    if(order.price < stock.buy_price){
                        stock.buy_price = order.price;
                        stock.buy_time = static_cast<uint32_t>(timestamp);
                    }
                } else { //BUY
                    if(order.price > stock.buy_price){ 
                        stock.status = TimeStatus::Completed;
                        stock.sell_price = order.price;
                        stock.sell_time = static_cast<uint32_t>(timestamp);
                    }
                }
            } else if(stock.status == TimeStatus::Completed){
                if(buysell == "SELL"){ //SELL
                    if(order.price < stock.buy_price){
                        stock.status = TimeStatus::Potential;
                        stock.pbuy_price = order.price;
                        stock.pbuy_time = static_cast<uint32_t>(timestamp);
                    }
                } else { //BUY
                    if(order.price > stock.sell_price){
                        stock.sell_price = order.price;
                        stock.sell_time = static_cast<uint32_t>(timestamp);
                    }
                }
            } else { // status = POTENTIAL
                if(buysell == "SELL"){ //SELL
                    if(order.price < stock.pbuy_price){
                        stock.pbuy_price = order.price;
                        stock.pbuy_time = static_cast<uint32_t>(timestamp);
                    }
                } else { //BUY
                    // potential has higher profit
                    if((static_cast<int>(order.price) - static_cast<int>(stock.pbuy_price)) > 
                        (static_cast<int>(stock.sell_price) - static_cast<int>(stock.buy_price))){
                        stock.buy_price = stock.pbuy_price;
                        stock.buy_time = stock.pbuy_time;
                        stock.sell_price = order.price;
                        stock.sell_time = static_cast<uint32_t>(timestamp);
                        stock.status = TimeStatus::Completed;
                    }
                }
            }
        }

        if(timestamp !=  CURRENT_TIMESTAMP){       
            if(options.median) median_output(stocks, CURRENT_TIMESTAMP);
            CURRENT_TIMESTAMP = timestamp;
        }
        make_trade(buysell, order, stocks, stockID, traders, options, trade_count);
        
    }
    if(options.median) median_output(stocks, CURRENT_TIMESTAMP); // last median output
}


void load_stocks(vector<Stock> &stocks, vector<Trader> &traders, Options& options, uint32_t &trade_count){
    string line;
    //comment
    getline(cin, line);
    //mode
    getline(cin, line);
    string mode = line.substr(line.size()-2);

    //num traders
    getline(cin,line);
    int num_traders = stoi(line.substr(line.find(' ')+1));
    // num stocks
    getline(cin, line);
    int num_stocks = stoi(line.substr(line.find(' ') +1));

    stringstream ss;

    if(mode == "PR"){
        //random seed
        getline(cin,line);
        unsigned int seed = static_cast<unsigned int>(stoul(line.substr(line.find(' ')+1)));

        // num of orders
        getline(cin,line);
        unsigned int num_orders = static_cast<unsigned int>(stoul(line.substr(line.find(' ')+1)));

        // arrival rate
        getline(cin,line);
        unsigned int rate = static_cast<unsigned int>(stoul(line.substr(line.find(' ')+1)));

        P2random::PR_init(ss, seed, static_cast<unsigned int>(num_traders), 
                            static_cast<unsigned int>(num_stocks), num_orders, rate);
    }
    
    
    if (mode == "PR") processOrders(ss, num_stocks, num_traders, stocks, traders, options, trade_count); 
    else processOrders(cin, num_stocks, num_traders, stocks, traders, options, trade_count);

}


int main(int argc, char* argv[]){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Options options;
    getOptions(argc, argv, options);

    uint32_t trade_count = 0;
    vector<Stock> stocks;
    vector<Trader> traders;
    cout << "Processing orders...\n";
    load_stocks(stocks, traders, options, trade_count);

    // summary output
    cout << "---End of Day---\nTrades Completed: " << trade_count << "\n";

    // trader output
    if(options.trader){
        cout<<"---Trader Info---\n";
        for(size_t i = 0; i < traders.size(); i++){
            cout<<"Trader " << i << " bought " << traders[i].bought << " and sold " << traders[i].sold
            << " for a net transfer of $" << traders[i].net << '\n';
        }
    }

    // time travel
    if(options.time_traveler){
        cout<<"---Time Travelers---\n";
        for(size_t i = 0; i < stocks.size(); i++){
            if(stocks[i].buy_price >= stocks[i].sell_price){
                cout<<"A time traveler could not make a profit on Stock " << i << '\n';
            } else {
                cout<<"A time traveler would buy Stock " << i << " at time " << stocks[i].buy_time
                << " for $" << stocks[i].buy_price << " and sell it at time " << stocks[i].sell_time
                << " for $" << stocks[i].sell_price << "\n";
            }
        }
    }



}