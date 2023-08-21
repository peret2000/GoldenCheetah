# Steps:
# docker build -t ubuntu_18.04_goldencheetah .
# docker run -d -it --device /dev/fuse --cap-add SYS_ADMIN --security-opt apparmor:unconfined -h ubuntu18.08_GC --name ubuntu18.08_GC ubuntu_18.04_goldencheetah
# 		cap-add and security options are needed to mount fuse device (when creating appimage)

# docker exec -it ubuntu18.08_GC bash

## There must exist a file along with Dockerfile: secrets_goldencheetah.txt
## With environment variables to be used by script before_install.sh. Its content must be:
#export GC_STRAVA_CLIENT_SECRET=XXXXXX
#export GC_CLOUD_DB_BASIC_AUTH=XXXXXX
#export GC_CLOUD_DB_APP_NAME=XXXXXX

FROM ubuntu:18.04

RUN useradd --create-home appuser

#Install necessary packages
RUN apt update && apt install -y sudo git wget automake libtool bison flex curl software-properties-common fuse libfuse2 && usermod -aG sudo appuser
#Not necessary but recommended
RUN apt install -y procps screen
RUN rm -rf /var/lib/apt/lists/*

#User must have 'sudo' privileges
#Also, it is necessary to set a timezone to avoid interruptions during script execution (can be an environment variable)
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers && ln -s /usr/share/zoneinfo/Europe/Madrid /etc/localtime


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

RUN ln -s $TRAVIS_BUILD_DIR/scripts/daily_compile.sh $TRAVIS_BUILD_DIR/..

#For after_success.sh script
RUN sudo addgroup fuse && sudo adduser appuser fuse

RUN cd $TRAVIS_BUILD_DIR && git config alias.lld 'log --pretty=format:"%C(yellow)%h%x20%C(cyan)%d%x20%C(green)%ad%x20%C(reset)%s%x20%C(red)(%an)" --graph --all --date=relative'

#Modifications for the daily script
COPY secrets_goldencheetah.txt .
RUN sudo chmod a+rw secrets_goldencheetah.txt && cat secrets_goldencheetah.txt >> .profile && rm secrets_goldencheetah.txt && echo "export ENV_LOADED=1" >> .profile

#Cron configuration
RUN echo "20 4 * * * $TRAVIS_BUILD_DIR/../daily_compile.sh  2>&1" |crontab -
RUN echo "#!/bin/bash" > ./entry.sh && echo "sudo service cron start && sudo sed -i '/%sudo ALL=(ALL) NOPASSWD:ALL/d' /etc/sudoers && sleep infinity" >> ./entry.sh && chmod u+x ./entry.sh
ENTRYPOINT ["./entry.sh"]
