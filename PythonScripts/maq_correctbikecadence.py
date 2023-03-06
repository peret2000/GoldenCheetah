import numpy as np


# Si la actividad es bicicleta, outdoor, y la cadencia es superior a 100, la divide por dos,
# se asume que es un fallo del sensor de cadencia, que por alguna razón la registra duplicada
# en algunos momentos

def main():

	activity = GC.activity()

	sport = GC.getTag('Sport')
	subsport = GC.getTag('SubSport')

	if not (sport=='Bike' and subsport=='Ride' and 'cadence' in activity):
		return


	cadence = GC.series(GC.SERIES_CAD)

	for i in range(0,len(cadence)):
		if (cadence[i] > 100):
			cadence[i] = cadence[i] / 2


def manageError(e):
	print("Error :", e.__str__())
	GC.setTag('Notes', GC.getTag('Notes') + '\n ERROR in processing script CorrectBikeCadence. ' + e.__str__())
	GC.setTag('Error', '1')

if __name__ == "__main__":
	try:
		main()
	except Exception as e:
		manageError(e)


