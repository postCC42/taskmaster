# Overview

## How supervisord works
- [ref](http://supervisord.org/)

- how to install supervisord
    - `sudo pip install supervisor`
    - `sudo echo_supervisord_conf | sudo tee /etc/supervisord.conf > /dev/null`: Supervisor provides a utility command (echo_supervisord_conf) to generate a default configuration file (supervisord.conf)
    - `sudo supervisord -c /etc/supervisord.conf`: This command starts Supervisor as a daemon process based on the configuration provided in /etc/supervisord.conf. The Supervisor daemon will read this configuration file and manage the programs specified within it.
    - `cat /etc/supervisord.conf`: check the conf

- sipervisord vs systemctl
    -  While systemctl manages system services, Supervisord focuses on managing individual processes, which can be more lightweight and flexible for certain applications, especially in development or specialized environments.
- supervisord purpose
    - Supervisord ensures that specified processes are running and handles tasks such as starting them on system boot (autostart), restarting them if they fail unexpectedly (autorestart), and providing logging and monitoring capabilities.
- Config
    - The configuration file (/etc/supervisord.conf) defines which processes (programs) to manage and how to manage them, including startup options, logging, and more.
    - Each program section in the configuration file specifies how to start, stop, and manage a particular process:
    ```
    [program:myprogram]
    command=/path/to/your/program arguments    ; Command to start your program
    directory=/path/to/your/program/directory  ; Directory to run the command from
    autostart=true                              ; Start this program when Supervisor starts
    autorestart=true                            ; Restart the program automatically if it exits unexpectedly
    stdout_logfile=/var/log/myprogram.log       ; Log file for program stdout
    stderr_logfile=/var/log/myprogram.err       ; Log file for program stderr
    ```
        - command: Specify the command to start the specified program. e need to replace /path/to/your/program with the actual path and include any necessary arguments.
        - directory: Optionally specify the directory from which to run the command.
        - autostart: Determines if the program should start automatically when Supervisor starts (true or false).
        - autorestart: Controls whether Supervisor should restart the program if it exits (true or false).
        - stdout_logfile and stderr_logfile: Paths to log files for capturing program output.
    - we can add env variable for each process:
    `environment=ENV_VAR1="value1",ENV_VAR2="value2"`

- exemple usage:
    - start a program: `sudo supervisorctl start myprogram`
    - stop a program: `sudo supervisorctl stop myprogram`
    - restart a program: `sudo supervisorctl restart myprogram`
    - check program status: `sudo supervisorctl status`
- when to use supervisord is useful:
    - Supervisord is designed to handle and manage long-running processes or applications. It is particularly useful in environments where you need:
        - process management => define and control specific applications or scripts independently.
        - automatic restart => It ensures that if a managed process crashes or exits unexpectedly, it can automatically restart the process according to configured rules 
        - Logging and Monitoring => Supervisord provides logging capabilities (stdout_logfile, stderr_logfile) to capture the output of managed processes. It also offers a monitoring interface (supervisorctl) to check the status of processes and manage them.
        - Environment Management => You can define environment variables (environment section in the configuration) specific to each process, allowing you to configure the runtime environment as needed.
