FROM ubuntu

ENV DEBIAN_FRONTEND "noninteractive"
RUN apt-get update -qq && \
    apt-get install --no-install-recommends --no-install-suggests -y \
                    build-essential git python3-pip python3 nginx && pip3 install --no-cache-dir fastapi uvicorn

RUN git clone https://github.com/laurolins/nanocube.git /nanocube-src

RUN mkdir /data /www
RUN cp -a /nanocube-src/web/dist/* /www

WORKDIR nanocube-src
RUN ./configure --with-polycover --prefix=/nanocube && make && make install


COPY ./docker-app /app

#setup a sample /data

#generate a sample cube
RUN bash -c '/nanocube/bin/nanocube create <(gunzip -c ./data/crime50k.csv.gz) ./data/crime50k.map /data/crime50k.nanocube -header'

#copy the sample configuration
RUN mv /app/config.json /data

#cleanup
RUN rm -rf /nanocube-src && \
    apt-get remove --purge -y build-essential git python3-pip && \
    apt-get autoremove -y && apt-get clean && \
    rm -rf /var/lib/apt/lists/*



#run
WORKDIR /app
CMD ["/app/run.sh"]