#!/bin/bash

#rm multithreaded-time-get-01.txt && 
for i in {1..150}; do { echo "POST /home/matheus/Documents/ multithreaded-example-get1.pdf" | time ./client 127.0.0.1 4001 | { tail -n 0 >/dev/null;  }  } 2>&1 | awk '{print $10}' >> multithreaded-time-get-01.txt ; done &&  awk '{s+=$0}END{print "A execução de 150 operacoes no servidor de arquivos multithread para o método POST é: "s/150}' multithreaded-time-get-01.txt
