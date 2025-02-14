FROM ubuntu:18.04

RUN apt-get update && apt-get install -y curl apt-transport-https lsb-release gnupg2
RUN curl -s https://packages.wazuh.com/key/GPG-KEY-BLACKWELL | apt-key add - && \
    echo "deb https://packages.wazuh.com/3.x/apt/ stable main" | tee /etc/apt/sources.list.d/blackwell.list && \
    apt-get update && apt-get install blackwell-agent=3.13.2-1 -y
