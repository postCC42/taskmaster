# Overview

## How supervisord works
- [ref](http://supervisord.org/)

- how to install supervisord
    - `sudo pip install supervisor`
    - `sudo echo_supervisord_conf | sudo tee /etc/supervisord.conf > /dev/null`: Supervisor provides a utility command (echo_supervisord_conf) to generate a default configuration file (supervisord.conf)
    - `sudo supervisord -c /etc/supervisord.conf`: This command starts Supervisor as a daemon process based on the configuration provided in /etc/supervisord.conf. The Supervisor daemon will read this configuration file and manage the programs specified within it.
    - `cat /etc/supervisord.conf`: check the conf