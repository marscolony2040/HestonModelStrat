#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include <vector>
#include <curl/curl.h>
#include <math.h>
#include <cmath>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <algorithm>
#include <time.h>
#include <cpprest/ws_client.h>
#include <cctype>
#include <map>
#include <fstream>
#include <chrono>

using namespace boost::property_tree;

std::string premium_key = "Enter Financial Modeling Prep Key Here";

std::string trade_key = "Enter Alpaca Key Here";
std::string trade_sec = "Enter Alpaca Secret Here";

std::string order_url = "https://paper-api.alpaca.markets/v2/positions";
std::string trade_url = "https://paper-api.alpaca.markets/v2/orders";


using namespace web;
using namespace web::websockets::client;

bool error_in_order(ptree ds){
    for(ptree::const_iterator it = ds.begin(); it != ds.end(); ++it){
        if(it->first == "status"){
            std::string stats = it->second.get_value<std::string>();
            if(stats == "accepted" || stats == "pending_new"){
                return true;
            }
        }
    }
    return false;
}

std::string URL(std::string ticker)
{
    std::string url = "https://financialmodelingprep.com/api/v3/historical-price-full/" + ticker + "?apikey=" + premium_key;
    return url;
}

std::string URL2()
{
    std::string url = "https://financialmodelingprep.com/api/v3/stock-screener?exchange=nyse,nasdaq&isEtf=false&marketCapLowerThan=100000000&betaMoreThan=1.1&betaLowerThan=7.0&country=US&apikey=" + premium_key;
    return url;
}

std::string URL3(std::string ticker)
{
    std::string url = "https://financialmodelingprep.com/api/v3/quote-short/" + ticker + "?apikey=" + premium_key;
    return url;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string GET(std::string url) {

    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "User-Agent: MacOS");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request, res will get the return code
        res = curl_easy_perform(curl);
        
        // Check for errors
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Always cleanup
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return readBuffer;
}

std::string POST(const std::string& url, const std::string& json_msg) {
    CURL* curl;
    CURLcode res;
    std::string response_string;
    
    // Initialize CURL
    curl = curl_easy_init();
    if (curl) {
        // Set URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Set HTTP method to POST
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Set POST fields
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_msg.c_str());

        // Set headers
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("APCA-API-KEY-ID: " + trade_key).c_str());
        headers = curl_slist_append(headers, ("APCA-API-SECRET-KEY: " + trade_sec).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set callback function to handle response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        // Perform the request, res will get the return code
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to initialize CURL" << std::endl;
    }

    return response_string;
}

std::string marketBuy(std::string ticker, int size){
    std::string volume = std::to_string(size);
    std::map<std::string, std::string> msg = {
        {"side","buy"},
        {"symbol",ticker},
        {"type","market"},
        {"qty",volume},
        {"time_in_force","gtc"}
    };
    std::string jsonmsg = "{";
    for(auto & entry : msg){
        jsonmsg += "\"" + entry.first + "\":\"" + entry.second + "\",";
    }
    jsonmsg.pop_back();
    jsonmsg += "}";
    std::cout << jsonmsg << std::endl;
    return POST(trade_url, jsonmsg);
}

double Stamp(){
    auto now = std::chrono::system_clock::now();
    std::time_t tsp = std::chrono::system_clock::to_time_t(now);
    double result = (double) tsp;
    return result;
}

void TYPHOON(ptree df, std::vector<double> & prices, bool results){
    for(ptree::const_iterator it = df.begin(); it != df.end(); ++it){
        if(results == true){
            for(ptree::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt){
                if(jt->first == "adjClose"){
                    prices.push_back(jt->second.get_value<double>());
                }
            }
        }
        if(it->first == "historical"){
            TYPHOON(it->second, std::ref(prices), true);
        }
    }
}

void CYCLONE(ptree df, std::vector<std::string> & stock_tickers){
    for(ptree::const_iterator it = df.begin(); it != df.end(); ++it){
        for(ptree::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt){
            if(jt->first == "symbol"){
                stock_tickers.push_back(jt->second.get_value<std::string>().c_str());
            }
        }
    }
}

void KRONOS(ptree df, std::vector<double> & prices){
    for(ptree::const_iterator it = df.begin(); it != df.end(); ++it){
        for(ptree::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt){
            if(jt->first == "price"){
                prices.push_back(jt->second.get_value<double>());
            }
        }
    }
}

ptree JSON(std::string resp){
    std::stringstream ss(resp);
    ptree result;
    read_json(ss, result);
    return result;
}

std::vector<double> Computer(std::vector<double> prices, double dt){
    
    std::vector<double> result, sim_vol, ror;

    auto mean = [](std::vector<double> x){
        double total = 0;
        for(auto & i : x){
            total += i;
        }
        return total / (double) x.size();
    };

    auto variance = [&](std::vector<double> x){
        double total = 0;
        double mu = mean(x);
        for(auto & i : x){
            total += pow(i - mu, 2);
        }
        return total / ((double) x.size() - 1);
    };

    for(int i = 1; i < prices.size(); ++i){
        ror.push_back(prices[i]/prices[i-1] - 1.0);
    }

    double price = prices[prices.size() - 1];
    double drift = mean(ror);

    int window = 50;
    for(int i = window; i < ror.size(); ++i){
        std::vector<double> hold = {ror.begin() + (i - window), ror.begin() + i};
        double vol = variance(hold);
        sim_vol.push_back(vol);
    }

    double theta = sqrt(mean(sim_vol));
    double sigma = sqrt(variance(sim_vol));
    double mu_vol = mean(sim_vol);

    double top = 0, bot = 0;
    for(int i = 1; i < sim_vol.size(); ++i){
        top += (sim_vol[i] - mu_vol)*(sim_vol[i-1] - mu_vol);
        bot += pow(sim_vol[i] - mu_vol, 2);
    }

    double kappa = -log(top/bot)/dt;
    double volatility = variance(ror);

    result = {kappa, theta, sigma, price, drift, volatility};

    return result;
}

double dWT(){
    int num = 10;
    double dw = (rand() % (2*num + 1)) - num;
    return dw/100.0;
}

void OptionsParser(ptree data, std::string set_date, std::map<std::string, std::vector<double>> & calls, std::map<std::string, std::vector<double>> & puts){
    for(ptree::const_iterator it = data.begin(); it != data.end(); ++it){
        if(it->first == "c"){
            for(ptree::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt){
                for(ptree::const_iterator kt = jt->second.begin(); kt != jt->second.end(); ++kt){
                    if(kt->first == "l"){
                        calls["strike"].push_back(atof(jt->first.c_str()));
                        calls["lastPrice"].push_back(kt->second.get_value<double>());
                    }
                }
            }
        }
        if(it->first == "p"){
            for(ptree::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt){
                for(ptree::const_iterator kt = jt->second.begin(); kt != jt->second.end(); ++kt){
                    if(kt->first == "l"){
                        puts["strike"].push_back(atof(jt->first.c_str()));
                        puts["lastPrice"].push_back(kt->second.get_value<double>());
                    }
                }
            }
        }
        if(it->first == set_date){
            OptionsParser(it->second, set_date, std::ref(calls), std::ref(puts));
        }
        if(it->first == "options"){
            OptionsParser(it->second, set_date, std::ref(calls), std::ref(puts));
        }
    }
}

int main()
{
    srand(time(NULL));

    std::string init_response = GET(URL2());
    
    std::map<std::string, double> current_prices;
    std::vector<std::string> stock_tickers;// = {"F","GS","TSLA","NVDA","MSFT","AAPL","AMZN","SPY","WMT","HD","QCOM","META","XOM","JPM","GOOGL"};
    CYCLONE(JSON(init_response), std::ref(stock_tickers));


    
    double balance = 40000;
    int weight = int(0.025*balance);
    int qfin = int(balance/weight);
    int counter = 1;

    
    for(auto & ticker : stock_tickers){
        
        std::string response = GET(URL(ticker));
        std::string current_price = GET(URL3(ticker));

        std::vector<double> prices;
        TYPHOON(JSON(response), std::ref(prices), false);
        std::reverse(prices.begin(), prices.end());
        KRONOS(JSON(current_price), std::ref(prices));

        double t = 1.0;
        int n = 2000;
        int p = 200;
        double dt = t / (double) n;

        std::vector<double> params = Computer(prices, dt);

        int Go_Long = 0, Go_Short = 0;

        for(int k = 0; k < 200; ++k){
            double forecast_price = 0;
            for(int i = 0; i < p; ++i){
                double S0 = params[3];
                double v0 = 0.3;
                for(int j = 0; j < n; ++j){
                    double dw = dWT();
                    v0 += params[0]*(params[1] - v0)*dt + params[2]*sqrt(v0)*dw;
                    S0 += params[4]*S0*dt + sqrt(v0)*S0*dw;
                }
                forecast_price += S0;
            }
            forecast_price /= (double) p;
            if(params[3] <= forecast_price){
                Go_Long += 1;
            } else {
                Go_Short += 1;
            }
        }

        if(Go_Long >= 170){
            std::string resp = marketBuy(ticker, int(weight/params[3]));
            std::cout << resp << std::endl;
            
            std::cout << "Count: " << counter << "\tTicker: " << ticker << "\tLong: " << Go_Long << "\tShort: " << Go_Short << "\tCurrent Stock Price: " << params[3] << std::endl;
            

            
            if(error_in_order(JSON(resp)) == true){
                std::cout << "Good Order" << std::endl;
                counter += 1;
            } else {
                std::cout << "Bad Order" << std::endl;
            }
            
            
        } else {
            std::cout << "Non-Valid Order: " << counter << std::endl;
        }
        if(counter >= qfin){
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)/2.0);
    }
    
    

    return 0;
}
