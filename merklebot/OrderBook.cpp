#include "OrderBook.h"
#include "CSVReader.h"
#include <map>
#include <algorithm>
#include <iostream>


/** construct, reading a csv data file */
OrderBook::OrderBook(std::string filename)
{
    orders = CSVReader::readCSV(filename);
}

/** return vector of all know products within the dataset*/
std::vector<std::string> OrderBook::getKnownProducts()
{
    std::vector<std::string> products;

    std::map<std::string,bool> prodMap;

    for (OrderBookEntry& e : orders)
    {
        prodMap[e.product] = true;
    }
    
    // now flatten the map to a vector of strings
    for (auto const& e : prodMap)
    {
        products.push_back(e.first);
    }

    return products;
}

/** return whether a product exists in timestamp or doesn't */
bool OrderBook::isProductInTimestamp(std::string product, std::string timestamp, OrderBookType type)
{
    std::vector<std::string> products = getKnownProducts(timestamp, type);
    if (std::find(products.begin(), products.end(), product) == products.end()) {
        return false;
    }
    return true;
}

/** return whether a product exists in last timestamps or doesn't */
bool OrderBook::isProductInTimestamp(std::string product, std::string currentTime, OrderBookType type, int lastTimestamps)
{
    int passedTimestamps = 0;
    bool isInTimestamp = false;
    std::string lastReadTimestamp = "-1";
    
    //traverses all orders from end to start
    for (auto it = orders.rbegin(); it != orders.rend(); ++it)
    {   
        // verifies orders previous time to current time
        if (!isInTimestamp && it->timestamp == currentTime) {
            isInTimestamp = true;
        }

        //verifies the order within the last few requested timestamps
        if (passedTimestamps >= lastTimestamps + 1) {
            return false;
        }

        //if statement to check if the last read timestamp isn't the current timestamp
        if (lastReadTimestamp != it->timestamp) {

            // if statement to check
            if (lastReadTimestamp != "-1" && isInTimestamp) {
                passedTimestamps = passedTimestamps + 1;
            }
            lastReadTimestamp = it->timestamp;
        }

        //if we're in the time window the product & type exists
        if (isInTimestamp && it->orderType == type && it->product == product) {
            return true;
        }
    }
    return false;
}

/** calculates average for product in last timestamp */
double OrderBook::calcProductInTimestampsAvg(std::string product, std::string currentTime, int lastTimestamps, OrderBookType type)
{
    int passedTimestamps = 0;
    bool isInTimestamp = false;
    double sum = 0;
    int amountOfProducts = 0;
    std::string lastReadTimestamp = "-1";

    // traverses all orders from end to start
    for (auto it = orders.rbegin(); it != orders.rend(); ++it)
    {   
        // verifies orders previous time to current time
        if (!isInTimestamp && it->timestamp == currentTime) {
            isInTimestamp = true;
        }

        //verifies the order within the last few requested timestamps
        if (passedTimestamps >= lastTimestamps + 1) {
            break;
        }

        //if statement to check if the last read timestamp isn't the current timestamp
        if (lastReadTimestamp != it->timestamp) {

            // if statement to check
            if (lastReadTimestamp != "-1" && isInTimestamp) {
                passedTimestamps = passedTimestamps + 1;
            }
            lastReadTimestamp = it->timestamp;
        }

        //if we're in the time window the product & type exist
        if (isInTimestamp && it->orderType == type && it->product == product) {
            sum = sum + it->price;
            amountOfProducts = amountOfProducts + 1;
        }
    }

    return sum / amountOfProducts;
}

/** gets all orders for product in last timestamp */
std::vector<double> OrderBook::getOrdersInTimesteps(std::string product, std::string currentTime, int timesteps, OrderBookType type)
{
    int passedTimestamps = 0;
    bool isInTimestamp = false;
    std::string lastReadTimestamp = "-1";
    std::vector<double> prices;

    // traverses all orders from end to start
    for (auto it = orders.rbegin(); it != orders.rend(); ++it)
    {   
        // verifies orders previous time to current time
        if (!isInTimestamp && it->timestamp == currentTime) {
            isInTimestamp = true;
        }

        // verifies orders previous time to current time
        if (passedTimestamps >= timesteps + 1) {
            break;
        }

        //if statement to check if the last read timestamp isn't the current timestamp
        if (lastReadTimestamp != it->timestamp) {

            // if statement to check
            if (lastReadTimestamp != "-1" && isInTimestamp) {
                passedTimestamps = passedTimestamps + 1;
            }
            lastReadTimestamp = it->timestamp;
        }

        //if we're in the time window the product & type exist
        if (isInTimestamp && it->orderType == type && it->product == product) {
            prices.push_back(it->price);
        }
    }

    return prices;
}

/** calcs prediction for product in last timestamp */
double OrderBook::calcProductPrediction(std::string product, std::string currentTime, int timesteps, OrderBookType type, std::string requestedOperator)
{   
    // Retrieve all orders of product (to predict ask we take all bids, to predict bid we take all asks)
    // Sort by price
    // If max is requested proceed to calculate p90 price
    // If min is requested proceed to calculate p10 price

    std::vector<double> orders = getOrdersInTimesteps(product, currentTime, timesteps, type);
    std::sort(std::begin(orders), std::end(orders));

    int amountOfItems = (int) (0.1 * orders.size()); 
    
    if (requestedOperator == "min") {
        double sum = 0;
        for(int i = 0; i < amountOfItems; i++) 
        {
            sum = sum + orders[i];
        }
        return sum / amountOfItems;
    } else {
        double sum = 0;
        for(int i = orders.size()-1; i >= orders.size() - amountOfItems; i--) 
        {
            sum = sum + orders[i];
        }
        return sum / amountOfItems;
    }
}

/** return vector of all known products in the dataset that matches the timestamp*/
std::vector<std::string> OrderBook::getKnownProducts(std::string timestamp, OrderBookType type)
{
    std::vector<std::string> products;

    std::map<std::string,bool> prodMap;

    for (OrderBookEntry& e : orders)
    {
        if (e.timestamp == timestamp && e.orderType == type)
        {
            prodMap[e.product] = true;
        }
    }
    
    // now flatten the map to a vector of strings
    for (auto const& e : prodMap)
    {
    products.push_back(e.first);
    }

    return products;
}

/** returns pair of vectors of Orders each with different bookType*/
std::pair<std::vector<OrderBookEntry>, std::vector<OrderBookEntry>> OrderBook::getOrdersByBidAsk(std::string product, std::string timestamp)
{
    std::vector<OrderBookEntry> asks_sub;
    std::vector<OrderBookEntry> bids_sub;
    
    for (OrderBookEntry& e : orders)
    {
        if (e.timestamp > timestamp) {
            break;
        }

        if (e.product == product && 
            e.timestamp == timestamp )
            {
                if (e.orderType == OrderBookType::ask) {
                    asks_sub.push_back(e);
                } else if (e.orderType == OrderBookType::bid) {
                    bids_sub.push_back(e);
                }
            }
    }
    return std::make_pair(asks_sub, bids_sub);
}

/** return vector of Orders according to the sent filters*/
std::vector<OrderBookEntry> OrderBook::getOrders(OrderBookType type, 
                                        std::string product, 
                                        std::string timestamp)
{
    std::vector<OrderBookEntry> orders_sub;
    for (OrderBookEntry& e : orders)
    {
        if (e.timestamp > timestamp) {
            break;
        }

        if (e.orderType == type && 
            e.product == product && 
            e.timestamp == timestamp )
            {
                orders_sub.push_back(e);
            }
    }
    return orders_sub;
}


double OrderBook::getHighPrice(std::vector<OrderBookEntry>& orders)
{
    double max = orders[0].price;
    for (OrderBookEntry& e : orders)
    {
        if (e.price > max)max = e.price;
    }
    return max;
}


double OrderBook::getLowPrice(std::vector<OrderBookEntry>& orders)
{
    double min = orders[0].price;
    for (OrderBookEntry& e : orders)
    {
        if (e.price < min)min = e.price;
    }
    return min;
}

std::string OrderBook::getEarliestTime()
{
    return orders[0].timestamp;
}

std::string OrderBook::getNextTime(std::string timestamp)
{
    std::string next_timestamp = "";
    for (OrderBookEntry& e : orders)
    {
        if (e.timestamp > timestamp) 
        {
            next_timestamp = e.timestamp;
            break;
        }
    }
    if (next_timestamp == "")
    {
        next_timestamp = orders[0].timestamp;
    }
    return next_timestamp;
}

void OrderBook::insertOrder(OrderBookEntry& order)
{
    orders.push_back(order);
    std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);
}

std::vector<OrderBookEntry> OrderBook::matchAsksToBids(std::string product, std::string timestamp)
{
    std::pair<std::vector<OrderBookEntry>, std::vector<OrderBookEntry>> orders = getOrdersByBidAsk(product, timestamp);
    std::vector<OrderBookEntry> bids = orders.first;
    std::vector<OrderBookEntry> asks = orders.second;

    // sales = []
    std::vector<OrderBookEntry> sales; 

    // I put in a little check to ensure we have bids and asks
    // to process.
    if (asks.size() == 0 || bids.size() == 0)
    {
        std::cout << " OrderBook::matchAsksToBids no bids or asks" << std::endl;
        return sales;
    }

    // sort asks lowest first
    std::sort(asks.begin(), asks.end(), OrderBookEntry::compareByPriceAsc);
    // sort bids highest first
    std::sort(bids.begin(), bids.end(), OrderBookEntry::compareByPriceDesc);
    // for ask in asks:
    std::cout << "max ask " << asks[asks.size()-1].price << std::endl;
    std::cout << "min ask " << asks[0].price << std::endl;
    std::cout << "max bid " << bids[0].price << std::endl;
    std::cout << "min bid " << bids[bids.size()-1].price << std::endl;
    
    for (OrderBookEntry& ask : asks)
    {
    //     for bid in bids:
        for (OrderBookEntry& bid : bids)
        {
    //         if bid.price >= ask.price # we have a match
            if (bid.price >= ask.price)
            {
    //             sale = new order()
    //             sale.price = ask.price
            OrderBookEntry sale{ask.price, 0, timestamp, 
                product, 
                OrderBookType::asksale};

                if (bid.username == "simuser")
                {
                    sale.username = "simuser";
                    sale.orderType = OrderBookType::bidsale;
                }
                if (ask.username == "simuser")
                {
                    sale.username = "simuser";
                    sale.orderType =  OrderBookType::asksale;
                }
            
    //             # now work out how much was sold and 
    //             # create new bids and asks covering 
    //             # anything that was not sold
    //             if bid.amount == ask.amount: # bid completely clears ask
                if (bid.amount == ask.amount)
                {
    //                 sale.amount = ask.amount
                    sale.amount = ask.amount;
    //                 sales.append(sale)
                    sales.push_back(sale);
    //                 bid.amount = 0 # make sure the bid is not processed again
                    bid.amount = 0;
    //                 # can do no more with this ask
    //                 # go onto the next ask
    //                 break
                    break;
                }
    //           if bid.amount > ask.amount:  # ask is completely gone slice the bid
                if (bid.amount > ask.amount)
                {
    //                 sale.amount = ask.amount
                    sale.amount = ask.amount;
    //                 sales.append(sale)
                    sales.push_back(sale);
    //                 # we adjust the bid in place
    //                 # so it can be used to process the next ask
    //                 bid.amount = bid.amount - ask.amount
                    bid.amount =  bid.amount - ask.amount;
    //                 # ask is completely gone, so go to next ask                
    //                 break
                    break;
                }


    //             if bid.amount < ask.amount # bid is completely gone, slice the ask
                if (bid.amount < ask.amount && 
                   bid.amount > 0)
                {
    //                 sale.amount = bid.amount
                    sale.amount = bid.amount;
    //                 sales.append(sale)
                    sales.push_back(sale);
    //                 # update the ask
    //                 # and allow further bids to process the remaining amount
    //                 ask.amount = ask.amount - bid.amount
                    ask.amount = ask.amount - bid.amount;
    //                 bid.amount = 0 # make sure the bid is not processed again
                    bid.amount = 0;
    //                 # some ask remains so go to the next bid
    //                 continue
                    continue;
                }
            }
        }
    }
    return sales;             
}
