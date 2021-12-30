# sentinal: Software for Logfile and Inode Management

System and application processes can create many files, large files, or many inodes, possibly causing disk partitions to run out of space.  sentinal is a systemd service for monitoring filesystems containing applications' directories and managing the directories to comply with the specifications in an INI configuration file.

Monitoring and management capabilities:

- available disk space by percentage
- available inode usage by percentage or count
- logfiles by size, age, or retention settings
- inodes by age or retention settings
- log ingestion, processing, and rotation

## Configuration

sentinal uses INI files for its runtime configuration.  Each section in the INI file pertains to one resource: a directory, possibly a logfile template, and the conditions for managing the resource.

### INI File Description

    [global]
    pidfile:  sentinal process id and lock file, for manual logrotate

    [section]
    command:  command to run
    dirname:  thread and postcmd working directory, logfile location
    subdirs:  option to search subdirectories for matching files
    pipename: named pipe/fifo fifo location
    template: output file name, date(1) sequences %Y %m %d %H %M %S %s
    pcrestr:  perl-compatible regex naming files to manage
    uid:      username or uid for command/postcmd; default = nobody
    gid:      groupname or gid for command/postcmd; default = nogroup
    loglimit: rotate size, M = MiB, G = GiB; 0 = no rotate (off)
    diskfree: percent blocks free; 0 = no monitor (off)
    inofree:  percent inodes free; 0 = no monitor (off)
    expire:   log retention, units = m, H, D, W, M, Y; 0 = no expiration (off)
    retmin:   minimum number of logs to retain; 0 = none (off)
    retmax:   maximum number of logs to retain; 0 = no max (off)
    postcmd:  command to run after log closes or rotates, %n = filename

`section` is the section name.  It must be unique in the INI file.

`pipename` is the path to the FIFO, either absolute or relative to dirname, created by sentinal, owned by uid, in group gid:

    # ls -l example.log
    prw-r--r-- 1 sentinal sentinal 0 Dec  2 10:12 example.log

Note the following conditions.  If:

`command` is set, `template` must be set.

`loglimit` is greater than zero, sentinal rotates the log after it reaches the given size.

`diskfree` is greater than zero, sentinal creates a thread to discard logs to free disk space.

`inofree` is greater than zero, sentinal creates a thread to discard logs (files) to free inodes.

`expire` is greater than zero, sentinal removes logs older than the given time.

`retmin` is greater than zero, sentinal retains `n` number of logs, regardless of expiration.

`retmax` is greater than zero, sentinal retains a maximum number of `n` logs, regardless of expiration.

`postcmd` is specified, the value is passed as a command to `bash -c` after the log closes or rotates.  Optional.

### Disk Space Example

To monitor console logs in /opt/sentinal/log for 20% free disk space, and to retain at least 3 logs and at most 50 logs:

    [global]
    pidfile  = /run/diskfree-only.ini

    [console]
    dirname  = /opt/sentinal/log
    subdirs  = 0
    pcrestr  = console
    diskfree = 20
    retmin   = 3
    retmax   = 50

### Inode Usage Example

To monitor inode usage for 15% free and a maximum of 5M files where they are less than a week old:

    [global]
    pidfile  = /run/inode-usage.pid

    [files]
    dirname  = /path/to/appfiles
    subdirs  = 1
    pcrestr  = appdata-
    inofree  = 15
    expire   = 7D
    retmax   = 5M

### Simple Log Monitor

sentinal can monitor logs and process them when they reach a given size.  In this example, sentinal runs logrotate on chattyapp.log when the log exceeds 5MiB in size:

    [global]
    pidfile  = /run/chattyapp.pid

    [chattyapp]
    dirname  = /var/log
    subdirs  = 1
    template = chattyapp.log
    pcrestr  = chattyapp\.log\.
    uid      = root
    gid      = root
    loglimit = 5M
    postcmd  = /usr/sbin/logrotate -f /opt/sentinal/etc/chattyapp.conf

### Logfile Ingestion and Processing

sentinal can ingest and process logs, rotate them on demand or when they reach a given size, and optionally post-process logs after rotation.  For logfile processing, replace the application's logfile with a FIFO, and set sentinal to read from it.

For example, this configuration connects the dd program to example.log for log ingestion, and rotates and compresses the log when it reaches 5G in size:

    [example]
    command  = /bin/dd bs=64K status=none
    dirname  = /var/log
    pipename = example.log
    template = example-%Y-%m-%d_%H-%M-%S.log
    pcrestr  = example-
    uid      = appowner
    gid      = appgroup
    loglimit = 5G
    postcmd  = /usr/bin/zstd --rm -T0 %n 2>/dev/null

This example does basically the same as above, but with on-the-fly compression (no intermediate files), and rotates the compressed log when it reaches 1G in size:

    [example]
    command  = /usr/bin/zstd -T0
    dirname  = /var/log
    pipename = example.log
    template = example-%Y-%m-%d_%H-%M-%S.log.zst
    pcrestr  = example-
    uid      = appowner
    gid      = appgroup
    loglimit = 1G

### Precedence of Keys

`retmin` takes precedence over `diskfree`.
`diskfree` and `inofree` take precedence over `expire`, `retmax`.
The lesser of `expire` and `retmax` takes precedence over the other.

### systemd unit file

sentinal runs as a systemd service.  The following is an example of a unit file:

    [Unit]
    Description=Shim to zstd-compress logs
    StartLimitBurst=10
    StartLimitIntervalSec=0
    After=network.target network-online.target systemd-networkd.service

    [Service]
    Type=simple
    Restart=always
    RestartSec=1
    User=root
    ExecStart=/opt/sentinal/bin/sentinal -f /opt/sentinal/etc/sentinal.ini
    ExecReload=/bin/kill -s HUP $MAINPID

    [Install]
    WantedBy=multi-user.target

Note: if an application never needs root privileges to run and process logs, consider using the application's user and group IDs in the unit file.  User=root is useful (and likely necessary) when a single sentinal instance monitors several different applications.

## sentinal Status

The INI file tests/test4.ini is used here as an example.

    # systemctl status sentinal
    * sentinal.service - sentinal service for example.ini
         Loaded: loaded (/etc/systemd/system/sentinal.service; disabled; vendor preset: enabled)
         Active: active (running) since Wed 2021-11-24 13:01:47 PST; 4s ago
       Main PID: 1927853 (sentinal)
          Tasks: 4 (limit: 76887)
         Memory: 516.0K
         CGroup: /system.slice/sentinal.service
                 `-1927853 /opt/sentinal/bin/sentinal -f /opt/sentinal/etc/example.ini

    Nov 24 13:01:47 loghost systemd[1]: Started sentinal service for example.ini.
    Nov 24 13:01:47 loghost sentinal[1927853]: test4: command: /usr/bin/zstd -1 -T4
    Nov 24 13:01:47 loghost sentinal[1927853]: test4: monitor log size: 1024MiB
    Nov 24 13:01:47 loghost sentinal[1927853]: test4: monitor log min retention: 3
    Nov 24 13:01:47 loghost sentinal[1927853]: test4: monitor log max retention: 25
    Nov 24 13:01:47 loghost sentinal[1927853]: test4: monitor disk: / for 85.00% free
    Nov 24 13:01:47 loghost sentinal[1927853]: test4: /opt/sentinal/tests: 88.69% blocks free

    (In this example, /opt is in the / filesystem)

## Build, Install

sentinal requires the pcre-devel package for building the software.

    # cd sentinal
    # make
    # make install

Create a systemd unit file and add it to the local systemd directory, or run

    # make systemd

to install an example as a starting point.

    Edit /etc/systemd/system/sentinal.service as necessary.

    # systemctl daemon-reload

## Test INI Files

sentinal provides two options for testing INI files.  `-d` prints INI file sections as parsed, where the output is similar to the input.  `-v` prints INI file sections with the keys evaluated as they would be at run time, including symlink resolution and relative to full pathname conversion.

## Run

    # systemctl enable sentinal
    # systemctl start sentinal

Useful commands for monitoring sentinal:

    $ journalctl -f -n 20 -t sentinal
    $ journalctl -f _SYSTEMD_UNIT=example.service
    $ ps -lT -p $(pidof sentinal)
    $ top -H -S -p $(pidof sentinal)
    $ htop -d 5 -p $(pidof sentinal)
    # lslocks -p $(pidof sentinal)

Examples of on-demand log rotation:

    # systemctl reload sentinal
    # pkill -HUP sentinal
    # kill -HUP $(cat /path/to/pidfile)

## Notes

- Linux processes writing to pipes block when processes are not reading from them.  systemd manages sentinal to ensure sentinal is always running.  See README.fifo for more detail.

- The default pipe size in Linux is either 64KB or 1MB. sentinal increases its pipe sizes on 3.x.x and newer kernels to 64MiB.  Consider this a tuning parameter that can affect performance.

- In the on-the-fly compression example, zstd can be changed to a different program, e.g., gzip or (p)bzip2, though they are slower and may impact the performance of the writer application.

- For inode management, sentinal counts the number of files and directories in `dirname`, not the number of inodes in the filesystem.

- sentinal reports free space for unprivileged users, which may be less than the privileged users' values reported by disk utility programs.

- sentinal configuration without direct log handling (`command` and `pipename` are unset) does not start the worker thread, leaving the other threads to watch the logs and disk space as they would normally.

- The `loglimit` key represents bytes written to disk.  When `command` specifies a compression program, log rotation occurs after sentinal writes `loglimit` bytes post-compression.

- sentinal removes empty subdirectories within `dirname`.  To negate this behavior, create a file in the subdirectory, where the file name does not match `pcrestr`, for example, `.persist`.

