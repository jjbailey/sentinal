# Description of INI files

INI files must contain a single Global section, and 1 to 16 Log sections.

## Global Section Keys

    pidfile:  process id file, absolute path, required

## Log Section Keys

    command:  command line to run
              absolute path, optional, working directory is `dirname`

    dirname:  thread and `postcmd` working directory, file location
              absolute path, required

    subdirs:  option to search subdirectories; true, false
              default false

    pipename: named pipe/fifo fifo location
              absolute or relative path, required when `command` is defined

    template: output file name, date(1) sequences %F %Y %m %d %H %M %S %s
              relative to dirname, required when `command` is defined

    pcrestr:  perl-compatible regex naming files to manage
              valid perl-compatible regex, required for dfs and exp threads

    uid:      worker and `postcmd` username or uid; username root ok, uid 0 not ok
              default nobody

    gid:      worker and `postcmd` groupname or gid; groupname root ok, gid 0 not ok
              default nogroup

    loglimit: rotate size, units M = MiB, G = GiB; 0 = no rotate (off)
              default unlimited

    diskfree: percent blocks free for unprivileged users; 0 = no monitor (off)
              default off

    inofree:  percent inodes free for unprivileged users; 0 = no monitor (off)
              default off

    expire:   file retention, units = m, H, D, W, M, Y; 0 = no expiration (off)
              default unit = D
              default off

    retmin:   minimum number of logs to retain, regardless of expiration; 0 = none (off)
              default off

    retmax:   maximum number of logs to retain, regardless of expiration; 0 = no max (off)
              default off

    terse:    file removal notification; true = quiet, false = record log removal
              default 0/false

    postcmd:  command to run after the log closes or rotates, %n = filename
              string passed to bash -c, optional, working directory is `dirname`
              used when `command` is defined (log ingestion) or `template` is defined (slm)
              unused when `command` and `template` are not defined
              default none

The keys `template` and `pcrestr` are available as postcmd environment variables.

## Token Expansions

The `template` key can contain tokens similar to the date command:

    %F:  %Y-%m-%d
    %Y:  year
    %m:  month
    %d:  day
    %H:  hour
    %M:  minute
    %S:  second
    %s:  seconds since epoch

The `postcmd` key can contain the current directory, file, hostname, and section:

    (version 1.3.0+)

    %host:  system hostname (nodename)
    %path:  current dirname (path)
    %file:  current filename
    %sect:  section name

    (pre-version 1.3.0, deprecated)

    %h:  system hostname (nodename)
    %p:  current dirname (path)
    %n:  current filename
    %t:  section name

## Threads

Thread names have a kernel-imposed length limit of 16 characters (15 + nul).
Thread names are assigned `<sectionname>_<taskname>`.  Task names and purposes:

    wrk:  worker thread
    dfs:  filesystem free space monitor thread
    exp:  file expire monitor thread
    slm:  simple log monitor thread

Example: file expire thread name for the section called `console`: `console_exp`

## Debugging INI Files

sentinal accepts two flags for debugging.
`-d` prints INI files as parsed.
`-v` prints INI files as interpreted, and prints the threads that will run.

    $ /opt/sentinal/bin/sentinal -f /opt/sentinal/tests/test4.ini -d

    [test4]
    command  = /usr/bin/zstd -1 -T4
    dirname  = /opt/sentinal/tests
    subdirs  = 1
    pipename = test4.fifo
    template = test4-%Y-%m-%d_%H-%M-%S.log.zst
    pcrestr  = test4-
    uid      = sentinal
    gid      = sentinal
    loglimit = 1G
    diskfree = 85
    inofree  =
    expire   =
    retmin   = 3
    retmax   = 25
    postcmd  = dir=%p/$(date +%d/%H/%M) ; mkdir -p $dir ; mv %n $dir

    $ /opt/sentinal/bin/sentinal -f /opt/sentinal/tests/test4.ini -v

    section:  test4
    command:  /usr/bin/zstd -1 -T4
    argc:     3
    path:     /usr/bin/zstd
    argv:     -1 -T4
    dirname:  /opt/sentinal/tests
    subdirs:  1
    pipename: /opt/sentinal/tests/test4.fifo
    template: test4-%Y-%m-%d_%H-%M-%S.log.zst
    pcrestr:  test4-
    uid:      1324
    gid:      1324
    loglimit: 1024MiB
    diskfree: 85.00
    inofree:  0.00
    expire:   0m
    retmin:   3
    retmax:   25
    execcmd:  zstd -f -1 -T4 > test4-2021-12-06_10-59-06.log.zst
    postcmd:  dir=/opt/sentinal/tests/$(date +%d/%H/%M) ; mkdir -p $dir ; mv test4-2021-12-06_10-59-06.log.zst $dir
    threads:  dfs: true   exp: true   slm: false   wrk: true

