#GC.deleteSeries(GC.SERIES_HRV)


list = GC.xdataNames()
print(list)
if 'TRAIN' in list:
    print('SI')
else:
    print('NO')
GC.xdataSeries('TRAIN','TARGET').remove()