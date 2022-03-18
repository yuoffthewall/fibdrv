set title "Execution time comparision"
set xlabel "n"
set ylabel "Execution time(ns)"
set terminal png font " Times_New_Roman,12 "
set output "output.png"
set xtics 0 ,10 ,300
set ytics 0 ,10 ,300
set key left 

plot \
"time1.txt" using 1:2 with linespoints linewidth 2 title "fib", \
"time2.txt" using 1:2 with linespoints linewidth 2 title "fast doubling"
