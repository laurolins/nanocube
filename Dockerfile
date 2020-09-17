FROM ubuntu

ENV DEBIAN_FRONTEND "noninteractive"

COPY ./docker-app /app

#build
RUN /app/build.sh

#run
WORKDIR /app
CMD /app/run.sh