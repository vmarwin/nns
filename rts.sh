#!/bin/sh

#bot parametrs:
#isin, f-alfa, s-alfa, period, size, mtpl, ntr, amount, start, end, close, ref-key, fut-key, trd-key

#run bot on 1 processor core
taskset -c 1 ./bot RTS-3.22 0.9 0.99999 10 100 900 1 1 25200 85200 85740 ref_1 futures_quote_1 trade_1
