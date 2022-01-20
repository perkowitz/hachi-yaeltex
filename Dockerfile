FROM ubuntu:latest

RUN apt-get update -y
RUN apt-get install curl -y
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh -s 0.15.0

ENV PYTHONUNBUFFERED 1
RUN mkdir /code
WORKDIR /code
ENV PYTHONPATH "${PYTHONPATH}:/code"

COPY . /code/

RUN arduino-cli core install arduino:samd
RUN arduino-cli lib install Keyboard
RUN arduino-cli board listall
