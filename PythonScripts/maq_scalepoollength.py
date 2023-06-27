import numpy as np

def main():

	sport = GC.getTag('Sport')
	subsport = GC.getTag('SubSport')

	if (sport != 'Swin' and subsport != 'lap swimming'):
		return

	list = GC.xdataNames()
	if not 'SWIM' in list:
		return


	pl = GC.getTag('Pool Length')

	# Compara distancia entre 'laps' y el tag 'Pool Length', mirando la distancia de la primera vuelta
	distlaps = GC.xdataSeries('SWIM','km')
	np_distlaps = np.array(distlaps)
	ratio = float(pl) / (distlaps[1]*1000)
	if (ratio > 0.99 and ratio < 1.01):
		return

	# Ajusta 'Distance' en la pestaña XDATA->SWIM
	np_distlaps = np.round(np_distlaps * ratio, 5)
	for i in range(0,len(distlaps)):
		distlaps[i] = np_distlaps[i]

	# Hace el mismo ajuste en la distancia y velocidad
	distance = GC.series(GC.SERIES_KM)
	speed = GC.series(GC.SERIES_KPH)

	np_dist = np.array(distance) * ratio
	np_speed = np.array(speed) * ratio
	for i in range(0,len(distance)):
		distance[i] = np_dist[i]
		speed[i] = np_speed[i]




if __name__ == "__main__":
	main()