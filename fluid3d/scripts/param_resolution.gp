set term png size 640,480 font arial 11 
set output "param.png"
set title "Resolution change impact on computing a single step \n GPU run"
set key inside left
set key autotitle columnheader
#set yrange [0:50000000]
set xlabel "Resolution values (³)"
set ylabel "Step computation time, s"
set yrange [0:maxy]
set xrange [0:maxx]
set xtics nomirror rotate by -45 scale 0
set style line 2 linecolor rgb '#0060ad' linetype 1 linewidth 2
f(x) = a*(x*x*x) + b
fit f(x) 'param.dat' using 1:2 via a, b
plot "param.dat" using 1:2 pt 1 ps 1 , f(x) ls 2 title "fitted"
