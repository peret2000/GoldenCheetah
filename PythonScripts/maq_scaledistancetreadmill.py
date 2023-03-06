import numpy as np

def main():

	activity = GC.activity()

	if ('altitude' in activity or 'longitude' in activity or 'latitude' in activity):
		return

	distance = GC.series(GC.SERIES_KM)

	if (GC.getTag('Distance') == distance[-1]):
		return

	speed = GC.series(GC.SERIES_KPH)

	ratio = GC.getTag('Distance') / distance[-1]
	np_dist = np.array(distance)
	np_speed = np.array(speed)
	np_dist = np_dist * ratio
	np_speed = np_speed * ratio

	for i in range(0,len(distance)):
		distance[i] = np_dist[i]
		speed[i] = np_speed[i]

def manageError(e):
	print("Error :", e.__str__())
	GC.setTag('Notes', GC.getTag('Notes') + '\n ERROR in processing script ScaleDistanceTreadmill. ' + e.__str__())
	GC.setTag('Error', '1')

if __name__ == "__main__":
	try:
		main()
	except Exception as e:
		manageError(e)


