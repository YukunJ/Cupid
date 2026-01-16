#!/usr/bin/env python3

"""
Generate realistic market traces to benchmark matching engine performance
Based on the assumption that for a single instrument:
1. its order book's two sides might be deep
2. but most activities happen close to the top of the book
"""

import random
import json
import struct
import statistics
from enum import IntEnum
from typing import List, Dict, Tuple, Optional, Deque
import numpy as np
from dataclasses import dataclass, asdict
import argparse
import csv
from collections import defaultdict, deque

class ActionType(IntEnum):
    LIMIT = 0
    CANCEL = 1

@dataclass
class Order:
    id: int = 0
    px: int = 0  # Price with 4 decimal (ex. 1000000 = $100)
    qty: int = 0
    side: int = 0  # -1 for ask, 1 for bid
    instr: str = ""
    trader: str = ""

@dataclass
class BenchmarkAction:
    type: ActionType
    order: Order
    cancel_id: int = 0

class OrderBook:
    """Simulates order book for just 1 instrument"""
    
    def __init__(self):
        # Current best bid/ask price
        self.best_bid: Optional[int] = None
        self.best_ask: Optional[int] = None
        
        # Track active orders by price level (FIFO queues)
        # price -> deque of (order_id, qty)
        self.bid_pricelevels: Dict[int, Deque[Tuple[int, int]]] = defaultdict(deque)
        self.ask_pricelevels: Dict[int, Deque[Tuple[int, int]]] = defaultdict(deque)
        
        # Track all active orders for cancellation
        self.all_orders: Dict[int, Tuple[int, int, int]] = {}  # order_id -> (side, price, qty)
        self.next_order_id = 1

        self.tick_size = 100

    def __repr__(self) -> str:
        self._update_bbo()
        return f"OrderBook(best_bid={self.best_bid}, best_ask={self.best_ask}, spread={self.spread()})"

    def _update_bbo(self):
        """
        Scan through the full order book to update bbo, don't care performance here
        Precondition: the order book must be valid:
        1. If a price level exists, there must be at least 1 resting order with positive quantity
        2. Order fully executed should be deleted from the price level already
        """
        self.best_bid = max(self.bid_pricelevels.keys()) if self.bid_pricelevels else None
        self.best_ask = min(self.ask_pricelevels.keys()) if self.ask_pricelevels else None

    def add_limit_order(self, side: int, price: int, qty: int) -> int:
        assert side in (-1, 1) # ask or bid
        assert price > 0 and (price % self.tick_size) == 0
        assert qty > 0
        order_id = self.gen_next_order_id()
        if side == 1: # bid
            while qty > 0 and self.best_ask and self.best_ask <= price:
                pl = self.ask_pricelevels[self.best_ask]
                while pl and qty:
                    oid, oqty = pl.popleft()
                    executed_qty = min(oqty, qty)
                    oqty -= executed_qty
                    qty -= executed_qty

                    if oqty > 0:
                        # resting order partial fill
                        pl.appendleft((oid, oqty))
                        self.all_orders[oid] = (-1, self.best_ask, oqty)
                    else:
                        del self.all_orders[oid]
                if not pl:
                    del self.ask_pricelevels[self.best_ask]
                    self._update_bbo()
        else:
            while qty > 0 and self.best_bid and self.best_bid >= price:
                pl = self.bid_pricelevels[self.best_bid]
                while pl and qty:
                    oid, oqty = pl.popleft()
                    executed_qty = min(oqty, qty)
                    oqty -= executed_qty
                    qty -= executed_qty

                    if oqty > 0:
                        # resting order partial fill
                        pl.appendleft((oid, oqty))
                        self.all_orders[oid] = (1, self.best_bid, oqty)
                    else:
                        del self.all_orders[oid]
                if not pl:
                    del self.bid_pricelevels[self.best_bid]
                    self._update_bbo()
        if qty > 0:
            if side == -1:
                self.ask_pricelevels[price].append((order_id, qty))
            else:
                self.bid_pricelevels[price].append((order_id, qty))
            self.all_orders[order_id] = (side, price, qty)
            self._update_bbo()
        return order_id

    def cancel_order(self, order_id: int) -> bool:
        """Cancel an order if it exists"""
        if order_id not in self.all_orders:
            return False
        side, px, _ = self.all_orders[order_id]
        if side == 1: # bid
            pl = self.bid_pricelevels[px]
            self.bid_pricelevels[px] = deque([(oid, qty) for oid, qty in pl if oid != order_id])
            if not self.bid_pricelevels[px]:
                del self.bid_pricelevels[px]
        else: # ask
            pl = self.ask_pricelevels[px]
            self.ask_pricelevels[px] = deque([(oid, qty) for oid, qty in pl if oid != order_id])
            if not self.ask_pricelevels[px]:
                del self.ask_pricelevels[px]           

        del self.all_orders[order_id]
        self._update_bbo()
        return True

    def gen_next_order_id(self) -> int:
        next_id = self.next_order_id
        self.next_order_id += 1
        return next_id

    def spread(self) -> Optional[int]:
        if self.best_ask and self.best_bid:
            return self.best_ask - self.best_bid
        else:
            return None