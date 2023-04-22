def main():

	activity = GC.activity()

	sport = GC.getTag('Sport')
	subsport = GC.getTag('SubSport')
	device = GC.getTag('Device')

	# Si la actividad es Run pero no hay SubSport ni coordenadas GPS -> Es VirtualRun

	if not ('altitude' in activity or 'longitude' in activity or 'latitude' in activity) and sport=='Run' and (subsport=="" or subsport=="treadmill"):
		subsport='VirtualRun'
		GC.setTag('SubSport', subsport)

	# Si la actividad es Bike pero no hay coordenadas GPS -> Es VirtualRide

	if not ('altitude' in activity or 'longitude' in activity or 'latitude' in activity) and sport=='Bike':
		subsport='VirtualRide'
		GC.setTag('SubSport', subsport)

	# Si la actividad es Bike pero no hay SubSport, y sí hay coordenadas GPS -> SubSport es Ride
	# Es el caso de actividades importadas desde Garmin directamente, sin pasar por Strava

	if 'altitude' in activity and 'longitude' in activity and sport=='Bike' and subsport=="":
		subsport='Ride'
		GC.setTag('SubSport', subsport)


	# Si el dispositivo es GoldenCheetah -> Sport="Bike" & SubSport="VirtualRide"
	if (device=='GoldenCheetah'):
		sport='Bike'
		subsport='VirtualRide'
		GC.setTag('Sport', sport)
		GC.setTag('SubSport', subsport)


	# General: PotenciaEstimada=0, salvo en los siguientes casos:
	# Que ya esté a 1 (no se puede saber si es estimada o no, así que se respeta ese valor)
	# Sport='Run', o SubSport='Ride', o Sport='Hike', o Sport='Walk', pero no Dispositivo='Kinomap' o 'Kinomap Run', y no tiene potencia calculada

	potestimada = 0
	try:
		potestimada = int(GC.getTag("PotenciaEstimada"))
	except:
		pass

	if potestimada == 0 and not ('power' in activity):
		if ((sport=='Run' or subsport=='Ride' or sport=='Hike' or sport=='Walk') and not (device=='Kinomap' or device=='Kinomap Run')):
			GC.setTag('PotenciaEstimada', '1')


def manageError(e):
	print("Error :", e.__str__())
	GC.setTag('Notes', GC.getTag('Notes') + '\n ERROR in processing script PreprocessImport. ' + e.__str__())
	GC.setTag('Error', '1')

if __name__ == "__main__":
	try:
		main()
	except Exception as e:
		manageError(e)


