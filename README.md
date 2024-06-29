# HestonModelStrat

## Description
This repository contains C++ code which conducts a simulation with the Heston Model to forecast a future stock price and go long or short based on the results of the simulation. If there is a significant weight towards going long, an algorithmic order will be executed on Alpaca's Paper Trading Engine. The data used in this program is fetched from FinancialModelingPrep.

## Requirements
You will need a free key from Financial Modeling Prep in order to fetch stock data, and also a free key and secret from Alpaca in order to place algorithmic orders on their paper trading engine. You will also need ```CURL``` and ```cpprest``` installed on your computer in order for the system to run.

## Running
You can compile the code using the ```no.sh``` shell script in this repository in order to run the program

