# Steps:
# docker build -t ubuntu_18.04_goldencheetah .
# docker run -d -it --restart always --device /dev/fuse --cap-add SYS_ADMIN --security-opt apparmor:unconfined -h ubuntu18.08_GC --name ubuntu18.08_GC ubuntu_18.04_goldencheetah
#               cap-add and security options are needed to mount fuse device (when creating appimage)

# docker exec -it ubuntu18.08_GC bash

# Depends on git project https://github.com/peret2000/ConfigureGit4Bash.git to configure Git

## There must exist a file along with Dockerfile: secrets_goldencheetah.txt
## With environment variables to be used by script before_install.sh. Its content must be:
#export GC_STRAVA_CLIENT_SECRET=XXXXXX
#export GC_CLOUD_DB_BASIC_AUTH=XXXXXX
#export GC_CLOUD_DB_APP_NAME=XXXXXX

## Also, there must exist a file along with Dockerfile: secrets_appuser_goldencheetah_password.txt
## with the password for the app user. Usefule to 'sudo'. Format:
# appuser:<password>

## And, finally, a file with user and token for pushover message (token for the app where the message will be sent to):
## secrets_pushover_goldencheetahdev.txt. Format:
#export PUSHOVERTOKEN=<app token>
#export PUSHOVERUSER=<user id>

FROM ubuntu:18.04

RUN useradd --create-home appuser

#Install necessary packages
RUN apt update && apt install -y sudo wget automake libtool bison flex curl software-properties-common fuse libfuse2 && usermod -aG sudo appuser
	# To have last git version
RUN add-apt-repository ppa:git-core/ppa -y && apt update && apt install git -y
#Not necessary but recommended
RUN DEBIAN_FRONTEND=noninteractive apt install -y procps screen tzdata locales nano
RUN rm -rf /var/lib/apt/lists/*

#User must have 'sudo' privileges
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

#Also, it is necessary to set a timezone to avoid interruptions during script execution (can be an environment variable)
ENV TZ=Europe/Madrid
RUN echo $TZ > /etc/timezone
RUN rm -f /etc/localtime && ln -s /usr/share/zoneinfo/$TZ /etc/localtime

# Set locale
RUN sed -i '/es_ES.UTF-8/s/^# //g' /etc/locale.gen && \
	locale-gen
ENV LANG es_ES.UTF-8
ENV LANGUAGE es_ES.UTF-8
ENV LC_ALL es_ES.UTF-8


WORKDIR /home/appuser
USER appuser
ENV TRAVIS_BUILD_DIR=/home/appuser/Git/GoldenCheetah_Debug

#To avoid confirmation of github.com
RUN mkdir ~/Git && mkdir -p ~/.ssh && ssh-keyscan github.com >> ~/.ssh/known_hosts
RUN git clone https://github.com/peret2000/GoldenCheetah.git $TRAVIS_BUILD_DIR && cd $TRAVIS_BUILD_DIR && git remote add goldencheetah https://github.com/GoldenCheetah/GoldenCheetah.git



RUN git config --global user.email "you@example.com" && git config --global user.name "Your Name"
#RUN cd ${TRAVIS_BUILD_DIR} && git fetch --all && git checkout MyBuildAdapt && git branch -D tmp && git checkout -b tmp && git merge --no-edit goldencheetah/master
RUN cd ${TRAVIS_BUILD_DIR} && git fetch --all && git checkout goldencheetah/master

#Changes in the script to work unattended and with no errors
RUN sed -i "/E298A3A825C0D65DFD57CBB651716619E084DAB9/c\mkdir -p ~/.gnupg && echo \"disable-ipv6\" >> ~/.gnupg/dirmngr.conf && sudo apt-key adv --homedir ~/.gnupg --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9" $TRAVIS_BUILD_DIR/travis/linux/before_install.sh
RUN sed -i "/r-base-dev/c\sudo apt-get install -qq r-base-dev" $TRAVIS_BUILD_DIR/travis/linux/before_install.sh

RUN mkdir $TRAVIS_BUILD_DIR/D2XX

RUN cd $TRAVIS_BUILD_DIR && ./travis/linux/before_install.sh && rm awscliv2.zip libftd2xx-x86_64-1.3.6.tgz sip-4.19.8.tar.gz v0.1.1git1.tar.gz
RUN cd $TRAVIS_BUILD_DIR && rm -rf aws sip-4.19.8 srmio-0.1.1git1
RUN cd ${TRAVIS_BUILD_DIR} && git checkout -- ./travis/linux/before_install.sh && git checkout MyBuildAdapt

RUN ln -s $TRAVIS_BUILD_DIR/scripts/daily_compile.sh $TRAVIS_BUILD_DIR/.. \
	&& ln -s $TRAVIS_BUILD_DIR/scripts/compile_and_continue.sh $TRAVIS_BUILD_DIR/..

#For after_success.sh script
RUN sudo addgroup fuse && sudo adduser appuser fuse

#Git configurations. Prepare for git usage
RUN git clone https://github.com/peret2000/ConfigureGit4Bash.git && ./ConfigureGit4Bash/configuregit.sh \
	&& rm -rf ConfigureGit4Bash

#Modifications for the daily script
COPY secrets_goldencheetah.txt .
RUN sudo chmod a+rw secrets_goldencheetah.txt && cat secrets_goldencheetah.txt >> .profile && rm secrets_goldencheetah.txt && echo "export ENV_LOADED=1" >> .profile

COPY secrets_pushover_goldencheetahdev.txt .
RUN sudo chmod a+rw secrets_pushover_goldencheetahdev.txt && cat secrets_pushover_goldencheetahdev.txt >> .profile && rm secrets_pushover_goldencheetahdev.txt

USER root
RUN chmod u+s /usr/sbin/cron
RUN sed -i '/%sudo ALL=(ALL) NOPASSWD:ALL/d' /etc/sudoers

#Management of appuser password through secret file
COPY secrets_appuser_goldencheetah_password.txt .
RUN (cat secrets_appuser_goldencheetah_password.txt | sudo chpasswd) && rm secrets_appuser_goldencheetah_password.txt

USER appuser

#Cron configuration
RUN echo "20 4 * * * $TRAVIS_BUILD_DIR/../daily_compile.sh  2>&1" |crontab -
RUN echo "#!/bin/bash" > ./entry.sh && echo "cron" >> ./entry.sh && echo "sleep infinity" >> ./entry.sh && chmod u+x ./entry.sh
ENTRYPOINT ["./entry.sh"]
