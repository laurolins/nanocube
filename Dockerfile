FROM ubuntu

COPY ./docker-app /app

#build
ENV DEBIAN_FRONTEND "noninteractive"
RUN /app/build.sh

#run
WORKDIR /app
ENTRYPOINT ["/app/run.sh"]
CMD ["crime","/data","/data/config.json","/data/crime50k.db"]
