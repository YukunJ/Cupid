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

dummy_order = Order(0, 0, 0, 0, "", "")

@dataclass
class BenchmarkAction:
    type: ActionType
    order: Order
    cancel_id: int = 0

class OrderBook:
    """order book for just 1 instrument"""
    
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

    def active_order_ids(self) -> List[int]:
        return list(self.all_orders.keys())

class OrderTraceGenerator:
    """simulate a series of traces of realistic market order activity"""
    def __init__(self):
        self.ob: OrderBook = OrderBook()
        self.ticker: str = "AAPL"
        self.traders: List[str] = ["TR1", "TR2", "TR3", "TR4", "TR5"]
        self.traces: List[BenchmarkAction] = list()

        # custom params
        self.reference_px = 1000000 # $100.0
        self.top_book_ratio = 0.7 # 70% activity happen on BBO
        self.max_pricelevel = 1000 # usually there is at most 1000 price levels per side
        self.cancel_prob = 0.3
    
    def _should_cross_spread(self) -> bool:
        """determine if next order should cross the spread to execute"""
        if not self.ob.spread():
            return False
        spread_in_ticks = self.ob.spread() / self.ob.tick_size
        # base probability: 20% at 1 tick, increasing to at most 50% at 20 ticks
        cross_prob = min(0.2 + (spread_in_ticks - 1) * 0.015, 0.5)
        return random.random() < cross_prob

    def _generate_depth(self):
        # 80% on Top of book, the rest 20% distribute across 1-1000. 
        # the closer to top of book, the more likely
        weights = [0.8]  # weight for 0 TOB
    
        relative_weights = []
        for i in range(1, self.max_pricelevel+1):
            relative_weights.append(1 / (i + 1))  # 1/x decay
        
        sum_relative = sum(relative_weights)
        for w in relative_weights:
            weights.append(w * 0.2 / sum_relative)
        
        return random.choices(range(0, self.max_pricelevel+1), weights=weights)[0]

    def _generate_price(self, side: int, depth: int) -> int:
        if side == 1: # bid
            if not self.ob.best_bid:
                return self.reference_px - self.ob.tick_size
            else:
                return self.ob.best_bid - depth * self.ob.tick_size
        else:
            if not self.ob.best_ask:
                return self.reference_px + self.ob.tick_size
            else:
                return self.ob.best_ask + depth * self.ob.tick_size

    def _generate_quantity(self, depth) -> int:
        # the closer to tob, the more likely it will be of 1 round lot
        # the deeper in the book, the bigger the size
        return 100 + min(1000, depth * 50)

    def generate_random_limit_order(self):
        all_sides = [side for side, _, _ in self.ob.all_orders.values()]
        imbalance = sum(all_sides)
        bid_ratio = max(0.3, min(0.7, 0.5 + imbalance / 100))
        
        side = 1 if random.random() < bid_ratio else -1
        depth = self._generate_depth()
        price = max(self.ob.tick_size, self._generate_price(side, depth))
        quantity = self._generate_quantity(depth)
        should_cross = self._should_cross_spread()
        if should_cross:
            # change the price to cross the spread last minute
            if side == 1:
                price = self.ob.best_ask or self.reference_px + self.ob.tick_size
            else:
                price = self.ob.best_bid or self.reference_px - self.ob.tick_size
        trader = random.choice(self.traders)
        self.generate_limit_order_trace(side, price, quantity, self.ticker, trader)

    def generate_N_trace(self, N: int):
        self.traces.clear()
        self.seed_initial_book(num_levels=10)
        while len(self.traces) < N:
            if random.random() < self.cancel_prob and self.ob.active_order_ids():
                # cancel
                self.generate_random_cancel()
            else:
                self.generate_random_limit_order()

    def generate_random_cancel(self):
        active_order_ids = self.ob.active_order_ids()
        assert active_order_ids
        active_order_ids_weight = [1/(oid +0.001) for oid in active_order_ids]
        to_cancel_id = random.choices(active_order_ids, weights=active_order_ids_weight, k=1)[0]
        self.generate_cancel_trace(to_cancel_id)

    def generate_limit_order_trace(self, side, price, quantity, ticker, trader) -> int:
        self.traces.append(BenchmarkAction(ActionType.LIMIT, Order(0, price, quantity, side, ticker, trader), 0))
        return self.ob.add_limit_order(side, price, quantity)

    def generate_cancel_trace(self, order_id) -> bool:
        self.traces.append(BenchmarkAction(ActionType.CANCEL, dummy_order, order_id))
        return self.ob.cancel_order(order_id)

    def seed_initial_book(self, num_levels=10):
        """Seed both sides with initial liquidity"""
        for i in range(num_levels):
            bid_price = self.reference_px - (i + 1) * self.ob.tick_size
            self.generate_limit_order_trace(1, bid_price, 200 + i*100, self.ticker, "MM1")
            
            ask_price = self.reference_px + (i + 1) * self.ob.tick_size
            self.generate_limit_order_trace(-1, ask_price, 200 + i*100, self.ticker, "MM1")

if __name__ == "__main__":
    generator = OrderTraceGenerator()
    
    # Test 1: Generate small number of traces
    print("Generating 100 traces...")
    try:
        generator.generate_N_trace(100)
        print(f"Success! Generated {len(generator.traces)} traces")
        
        # Count action types
        limit_count = sum(1 for a in generator.traces if a.type == ActionType.LIMIT)
        cancel_count = sum(1 for a in generator.traces if a.type == ActionType.CANCEL)
        print(f"Limit orders: {limit_count}, Cancels: {cancel_count}")
        
        # Show some sample traces
        print("\nFirst 5 traces:")
        for i, trace in enumerate(generator.traces[:5]):
            if trace.type == ActionType.LIMIT:
                print(f"  {i}: LIMIT - Side: {trace.order.side}, Price: ${trace.order.px/10000:.2f}, Qty: {trace.order.qty}")
            else:
                print(f"  {i}: CANCEL - Order ID: {trace.cancel_id}")
                
        # Check book state
        print(f"\nFinal book state: {generator.ob}")
        
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()