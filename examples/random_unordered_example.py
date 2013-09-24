# import struct
import sys
import os

n = 65000

options = [ ("unordered_nc_txt.dmp", "unordered_nc_bin.dmp", lambda x: n-1-x),
("ordered_nc_txt.dmp",   "ordered_nc_bin.dmp",   lambda x: x)]

for (txt_filename, bin_filename, time_func) in options:

    f = open(txt_filename,"w")
    f.write('''name: unordered_example
encoding: text
field: a nc_dim_cat_2
field: t nc_dim_time_2
field: x nc_var_uint_2

''')
    
    for i in xrange(n):
        f.write("%d %d %d\n" % (i, time_func(i), 1))
    
    f.close()
    
    cmd = "cat %s | ncdmp --encoding=b copy=a,a copy=t,t copy=x,x > %s" % (txt_filename, bin_filename)
    os.system(cmd)




#txt_filename = "unordered_nc_txt.dmp"
#bin_filename = "unordered_nc_bin.dmp"

