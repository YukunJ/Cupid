<img src="image/cupid.png" alt="Cupid Logo" height="200">


-----------------

## Cupid
**Cupid** - a matching engine in modern C++ seeking performance and elegance.

This repo will implement a matching engine for finanical instruments, aka an exchange. It will support different matching algorithms and order types. 

Part of the motivation for this repo is from my old [pure C implementation](https://github.com/YukunJ/LimitOrderBook) of the quantcup contest. But this repo will place more emphasis on utilizing modern C++ features for elegance while maintaining the original high performance.

-----------------

### Dependency

I use google's [benchmark](https://github.com/google/benchmark) to conduct performance testing. To fully build the project, it requires to install benchmark on the build host. You may follow the "Installation" section there to install it.

-----------------

### Reference

1. LimitOrderBook (https://github.com/YukunJ/LimitOrderBook)
2. When Nanoseconds Matter: Ultrafast Trading Systems in C++ - David Gross - CppCon 2024 (https://www.youtube.com/watch?v=sX2nF1fW7kI)
3. Google benchmark user guide (https://github.com/google/benchmark/blob/main/docs/user_guide.md)