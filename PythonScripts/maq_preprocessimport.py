def main():

	activity = GC.activity()
	metrics = GC.activityMetrics()

	sport = metrics['Sport']
	subsport = metrics['SubSport']
	device = metrics['Device']

	# Si la actividad es Run pero no hay SubSport ni coordenadas GPS -> Es VirtualRun

	if not ('altitude' in activity or 'longitude' in activity or 'latitude' in activity) and sport=='Run' and subsport=="":
		subsport='VirtualRun'
		GC.setTag('SubSport', subsport)

	# Si la actividad es Bike pero no hay coordenadas GPS -> Es VirtualRide

	if not ('altitude' in activity or 'longitude' in activity or 'latitude' in activity) and sport=='Bike':
		subsport='VirtualRide'
		GC.setTag('SubSport', subsport)

	# Si el dispositivo es GoldenCheetah -> Sport="Bike" & SubSport="VirtualRide"
	if (device=='GoldenCheetah'):
		sport='Bike'
		subsport='VirtualRide'
		GC.setTag('Sport', sport)
		GC.setTag('SubSport', subsport)

	# General: PotenciaEstimada=0, salvo en los siguientes casos:
	# Sport='Run', o SubSport='Ride', o Sport='Hike', o Sport='Walk', pero no Dispositivo='Kinomap' o 'Kinomap Run'

	potenciaestimada='0'
	GC.setTag('PotenciaEstimada', potenciaestimada)
	if ((sport=='Run' or subsport=='Ride' or sport=='Hike' or sport=='Walk') and not (device=='Kinomap' or device=='Kinomap Run')):
		potenciaestimada='1'
		GC.setTag('PotenciaEstimada', potenciaestimada)


def manageError(e):
	print("Error :", e.__str__())
	GC.setTag('Notes', GC.activityMetrics()['Notes'] + '\n ERROR in processing script PreprocessImport. ' + e.__str__())
	GC.setTag('Error', '1')

if __name__ == "__main__":
	try:
		main()
	except Exception as e:
		manageError(e)


