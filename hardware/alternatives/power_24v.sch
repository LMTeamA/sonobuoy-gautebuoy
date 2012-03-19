v 20110115 2
C 40000 40000 0 0 0 title-B.sym
B 43400 45800 2400 3900 3 0 0 0 -1 -1 0 -1 -1 -1 -1 -1
T 43500 47800 9 10 1 0 0 0 2
TEN 10-1222
DC/DC
P 42900 49300 43400 49300 1 0 0
{
T 42900 49300 5 10 0 0 0 0 1
pintype=unknown
T 43455 49295 5 10 1 1 0 0 1
pinlabel=12V
T 43305 49345 5 10 1 1 0 6 1
pinnumber=0
T 42900 49300 5 10 0 0 0 0 1
pinseq=0
}
P 42900 48900 43400 48900 1 0 0
{
T 42900 48900 5 10 0 0 0 0 1
pintype=unknown
T 43455 48895 5 10 1 1 0 0 1
pinlabel=GND
T 43305 48945 5 10 1 1 0 6 1
pinnumber=2
T 42900 48900 5 10 0 0 0 0 1
pinseq=0
}
P 46300 49400 45800 49400 1 0 0
{
T 46300 49400 5 10 0 0 0 0 1
pintype=unknown
T 45745 49395 5 10 1 1 0 6 1
pinlabel=+12V
T 45895 49445 5 10 1 1 0 0 1
pinnumber=0
T 46300 49400 5 10 0 0 0 0 1
pinseq=0
}
P 46300 48700 45800 48700 1 0 0
{
T 46300 48700 5 10 0 0 0 0 1
pintype=unknown
T 45745 48695 5 10 1 1 0 6 1
pinlabel=0V
T 45895 48745 5 10 1 1 0 0 1
pinnumber=0
T 46300 48700 5 10 0 0 0 0 1
pinseq=0
}
P 46300 47900 45800 47900 1 0 0
{
T 46300 47900 5 10 0 0 0 0 1
pintype=unknown
T 45745 47895 5 10 1 1 0 6 1
pinlabel=-12V
T 45895 47945 5 10 1 1 0 0 1
pinnumber=0
T 46300 47900 5 10 0 0 0 0 1
pinseq=0
}
C 40600 48800 1 0 0 connector2-1.sym
{
T 40800 49800 5 10 0 0 0 0 1
device=CONNECTOR_2
T 40600 49600 5 10 1 1 0 0 1
refdes=CONN?
}
N 42900 49300 42300 49300 4
N 42900 48900 42300 48900 4
N 42300 48900 42300 49000 4
C 48700 48800 1 0 0 lm7805-1.sym
{
T 50300 50100 5 10 0 0 0 0 1
device=L7805CV
T 50100 49800 5 10 1 1 0 6 1
refdes=5V
}
C 48700 45900 1 0 0 lm7912-1.sym
{
T 49000 47050 5 10 0 0 0 0 1
device=MC79L05AC
T 50100 46900 5 10 1 1 0 6 1
refdes=-5V
T 49000 47650 5 10 0 0 0 0 1
footprint=TO220
T 49000 47250 5 10 0 0 0 0 1
symversion=1.0
}
N 46300 49400 48700 49400 4
N 50300 49400 53800 49400 4
N 46300 48500 53000 48500 4
N 49500 48500 49500 48800 4
N 46300 48500 46300 48700 4
N 46300 47900 46900 47900 4
N 46900 46500 46900 47900 4
N 51400 47400 51400 48500 4
N 51400 46500 50300 46500 4
N 48700 46500 46900 46500 4
N 49500 45500 52200 45500 4
N 52200 45500 52200 48500 4
N 47600 50200 55300 50200 4
N 47600 50200 47600 49400 4
N 47400 44100 47400 46500 4
N 50600 46500 50600 45200 4
N 50600 45200 52700 45200 4
C 50900 48500 1 90 0 capacitor-1.sym
{
T 50200 48700 5 10 0 0 90 0 1
device=CAPACITOR
T 50400 48700 5 10 1 1 90 0 1
refdes=0.1µF
T 50000 48700 5 10 0 0 90 0 1
symversion=0.1
}
C 51600 46500 1 90 0 capacitor-1.sym
{
T 50900 46700 5 10 0 0 90 0 1
device=CAPACITOR
T 51100 46700 5 10 1 1 90 0 1
refdes=0.1µF
T 50700 46700 5 10 0 0 90 0 1
symversion=0.1
}
C 48600 46500 1 90 0 capacitor-1.sym
{
T 47900 46700 5 10 0 0 90 0 1
device=CAPACITOR
T 48100 46700 5 10 1 1 90 0 1
refdes=0.3µF
T 47700 46700 5 10 0 0 90 0 1
symversion=0.1
}
C 48600 48500 1 90 0 capacitor-1.sym
{
T 47900 48700 5 10 0 0 90 0 1
device=CAPACITOR
T 48100 48700 5 10 1 1 90 0 1
refdes=0.3µF
T 47700 48700 5 10 0 0 90 0 1
symversion=0.1
}
N 49500 45500 49500 45900 4
N 48400 47400 48400 48500 4
C 57000 50400 1 180 0 connector5-1.sym
{
T 55200 48900 5 10 0 0 180 0 1
device=CONNECTOR_5
T 56900 48700 5 10 1 1 180 0 1
refdes=CONN?
}
C 57000 47900 1 180 0 connector5-1.sym
{
T 55200 46400 5 10 0 0 180 0 1
device=CONNECTOR_5
T 56900 46200 5 10 1 1 180 0 1
refdes=CONN?
}
C 57000 45500 1 180 0 connector5-1.sym
{
T 55200 44000 5 10 0 0 180 0 1
device=CONNECTOR_5
T 56900 43800 5 10 1 1 180 0 1
refdes=CONN?
}
N 53400 50200 53400 45300 4
N 53400 45300 55300 45300 4
N 55300 47700 53400 47700 4
N 55300 49900 53800 49900 4
N 53800 45000 53800 49900 4
N 53800 47400 55300 47400 4
N 53800 45000 55300 45000 4
N 53000 44700 53000 49600 4
N 55300 47100 53000 47100 4
N 55300 49600 53000 49600 4
N 53000 44700 55300 44700 4
N 52700 44400 52700 49300 4
N 52700 49300 55300 49300 4
N 55300 46800 52700 46800 4
N 55300 44400 52700 44400 4
N 47400 44100 55300 44100 4
N 55000 44100 55000 49000 4
N 55000 46500 55300 46500 4
C 50600 44100 1 0 0 12V-minus-1.sym
C 52300 45200 1 0 0 5V-minus-1.sym
C 50800 50200 1 0 0 12V-plus-1.sym
C 51300 49400 1 0 0 5V-plus-1.sym
C 51700 48200 1 0 0 gnd-1.sym
N 55300 49000 55000 49000 4