FROM ubuntu

ENV DEBIAN_FRONTEND "noninteractive"

COPY ./docker-app /app

#build
RUN /app/build.sh

#run
WORKDIR /app
ENTRYPOINT ["/app/run.sh"]
CMD ["crime","/data","/data/config.json","/data/crimes50k.db"]