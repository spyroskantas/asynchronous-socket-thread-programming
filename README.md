# asynchronous-socket-thread-programming

A program showing thread and socket programming usage for learning purposes.

This program is a traveling monitor service that decides who can travel given his vaccination information. It was inspired by the recent Covid-19 virus outbreak.

usage: ./travelMonitorClient –m numberOfServers -b socketBufferSize(read/write) -c cyclicBufferSize -s sizeOfBloomFilter -i input_dir -t numberOfThreads

cyclicBuffer: a buffer that gathers information like, folders to add to database, for threads to take.
