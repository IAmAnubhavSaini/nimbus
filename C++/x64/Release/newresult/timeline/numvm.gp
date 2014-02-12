set term postscript color eps enhanced "Helvetica" 24  
set size ratio 0.5
# Line style for axes
set style line 80 lt 0
set style line 80 lt rgb "#808080"

# Line style for grid
set style line 81 lt 3  # dashed
set style line 81 lt rgb "#808080" lw 0.5  # grey

set grid back linestyle 81
set border 3 back linestyle 80 # Remove border on top and right.  These
             # borders are useless and make it harder
             # to see plotted lines near the border.
    # Also, put it in grey; no need for so much emphasis on a border.
set xtics nomirror
set ytics nomirror


# Line styles: try to pick pleasing colors, rather
# than strictly primary colors or hard-to-see colors
# like gnuplot's default yellow.  Make the lines thick
# so they're easy to see in small plots in papers.
set style line 1 lt 1
set style line 2 lt 1
set style line 3 lt 2
set style line 4 lt 2
set style line 1 lt rgb "#A00000" lw 5 pt 7 ps 2
set style line 2 lt rgb "#5060D0" lw 5 pt 9 ps 2
set style line 3 lt rgb "#A00000" lw 5 pt 5 ps 2
set style line 4 lt rgb "#5060D0" lw 5 pt 13 ps 2

set output "numvm.eps"

set xlabel "time (sec)"
set ylabel "# of VMs"

set key bottom left samplen 2

#set style data histogram
#set style histogram clustered gap 2
#set style fill solid border -1
#set boxwidth 1
set xrange [0:200]
set yrange [0:10]
#set logscale y
#set mxtics 10
#set xtics ("1" 1, "4" 2, "9" 3, "16" 4)
#set xtics (1,2,4,8,16,32,64,128,256,512) rotate by -75
#flows  TCP to DCTCP to  detour to
#set xtics (1,2,3,4)
#set ytics (0,200,40,60,80)
#set logscale x

plot    "timeline.plo" using ($1):($2) w lp ls 1  title "10% samp" 
		#"cdf_inbound_UDP_packet.plo" using 1:2 w l ls 3 title "Inbound UDP", \
	    #"cdf_outbound_UDP_packet.plo" using 1:2 w l ls 4 title "Outbound UDP"
		#"cdfinboundudp.plo" using 1:2 w l ls 3 title "Inbound UDP", \
	    #"cdfoutboundudp.plo" using 1:2 w l ls 4 title "Outbound UDP"
		
