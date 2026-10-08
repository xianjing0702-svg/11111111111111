I will divide the project into a few parts. 

Task currently: 
1.Build document management
---Use Document.c  and  Document.h as the header file

2. Build text processer
---Use text.c  and  text.h as the header file

3.Build Linear search method
---Use search.c  and  search.h as the header file

4.Build inverted index method
---Use index.c  and  index.h as the header file

5. Benchmark test( test the difference between using linear search method and inverted index search method)
---Use benchmark.c  and  index.h as the header file

6. main c (no need do yet)

Difference betwween .h   and  .c
.h is tell the program WHAT exists
.c is tell WHAT actual the program is 

The flowchart:

 \                 COMP1028 PROJECT
                           │
                           ▼
                 ┌─────────────────┐
                 │ 1. Document     │
                 │    Management   │
                 └────────┬────────┘
                          │
                          ▼
                 ┌─────────────────┐
                 │ 2. Text         │
                 │    Processing   │
                 └────────┬────────┘
                          │
                          ▼
              ┌───────────┴───────────┐
              ▼                       ▼
     ┌─────────────────┐     ┌─────────────────┐
     │ 3. Linear       │     │ 4. Inverted     │
     │    Search       │     │    Index        │
     └────────┬────────┘     └────────┬────────┘
              │                       │
              └───────────┬───────────┘
                          ▼
                 ┌─────────────────┐
                 │ 5. Benchmark    │
                 └────────┬────────┘
                          │
                          ▼
                 ┌─────────────────-
                 │ 6. Main.c       │
                 │    Integration  │
                 └─────────────────┘
                          │
                          ▼
                    FINAL PROGRAM
