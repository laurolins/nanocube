AUTOMAKE_OPTIONS=subdir-objects
ACLOCAL_AMFLAGS=-I m4

nanocube_webconfigdir=${prefix}
nobase_dist_nanocube_webconfig_SCRIPTS=\
bin/nanocube_webconfig.py \
bin/nanocube_webconfig

nanocube_webdir=${prefix}/share/nanocube/web

install-data-hook:
	rm -rf ${nanocube_webdir} &&  \
	mkdir -p ${nanocube_webdir} && \
	cp -av ./dist/* ${nanocube_webdir}

