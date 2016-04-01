set term x11
set output "kernels.png"
set title "Fluid steps execution times over " . maxx . " frames \n Plot as stacked histogram"
set key outside
set key autotitle columnheader
#set yrange [0:50000000]
set xlabel "Frames"
set ylabel "Time, ms"
set yrange [0:maxy]
set xrange [0:maxx]
set xtics nomirror rotate by -45 scale 0
set style data histograms
set style histogram rowstacked
set style fill solid noborder
set boxwidth 1 
plot for [i=2:fields] data using i
