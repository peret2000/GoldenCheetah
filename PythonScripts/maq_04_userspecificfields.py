# Específico por usuario, ya que la configuración no se puede hacer por usuario

# En este script se pueden asignar todos aquellos valores que son diferentes por usuario

def main():


	subsport = GC.getTag('SubSport')
	device = GC.getTag('Device')

	if GC.athlete()['name'] == 'MiguelAngel':
		# Si viene de Garmin, pone el peso que tiene configurado
		GC.setTag('Weight', '0')
		# Si SubSport='VirtualRun' -> InclCinta = 2
		if subsport == 'VirtualRun' and not (device=='Kinomap' or device=='Kinomap Run' or device=='Zwift Run'):
			GC.setTag('InclCinta', '2')
			print(GC.getTag("InclCinta"))
		# Si SubSport='lap swimming' -> Longitud de piscina = 10 (piscina de la comunidad)
		if subsport == 'lap swimming':
			GC.setTag('Pool Length', '10')
	elif GC.athlete()['name'] == 'Julio':
		# Si SubSport='VirtualRun' -> InclCinta = 1
		if subsport == 'VirtualRun' and not (device=='Kinomap' or device=='Kinomap Run'):
			GC.setTag('InclCinta', '1')




def manageError(e):
	print("Error :", e.__str__())
	GC.setTag('Notes', GC.getTag('Notes') + '\n ERROR in processing script UserSpecificFields. ' + e.__str__())
	GC.setTag('Error', '1')

if __name__ == "__main__":
	try:
		main()
	except Exception as e:
		manageError(e)

