examples/ALP/  
    Sample code of fork from Advanced Linux Programming book.

examples/gettimeofday/ 
    Sample code of using gettimeofday

examples/mqueue/
    Sample code of using POSIX queue functions

tools/
    A python script to run each given (N,B,P,C) 500 times and compute the following items: 
    - average run time, 
    - standard deviation of the run time
    NOTE: copy this file to the same folder as './produce.out' and then run it (python collect_data.py)

src/
    - Contains the skeleton for the Lab 3 Deliverable
    processes/
        - Your produce.c should create separate processes for Producers and Consumers
    threads/
        - Your produce.c should create separate threads for Producers and Consumers