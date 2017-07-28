import shapefile
import os
import zipfile


sf = shapefile.Reader('cb_2016_us_state_20m')

for s in sf.iterShapeRecords():
	w = shapefile.Writer()
	w._shapes.append(s.shape)
	w.records.append(s.record)
	w.fields = sf.fields
	print s.record
	os.mkdir(s.record[5])
	w.save(s.record[5]+"/"+s.record[5])