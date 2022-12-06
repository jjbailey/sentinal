# Description of INI files

INI files must contain a single Global section, and 1 to 16 Log sections.

## Global Section Keys

    pidfile:   process id file, absolute path, required
    database:  name of the sqlite3 database, optional, :memory: or path
               default :memory:

## Section Keys

    command:   command line to run
               absolute path, optional, working directory is `dirname`

    dirlimit:  directory size limit, units M = MiB, G = GiB; 0 = unlimited (off)
               default unlimited

    dirname:   thread and `postcmd` working directory, file location
               absolute path, required

    diskfree:  percent blocks free for unprivileged users; 0 = no monitor (off)
               default off

    expire:    file retention, units = m, H, D, W, M, Y; 0 = no expiration (off)
               default unit = D
               default off

    expiresiz: dfs and exp threads: remove file if it exceeds specified size
               default 0/any size

    gid:       worker and `postcmd` groupname or gid; groupname root ok, gid 0 not ok
               default nogroup

    inofree:   percent inodes free for unprivileged users; 0 = no monitor (off)
               default off

    pcrestr:   perl-compatible regex naming files to manage
               valid perl-compatible regex, required for dfs and exp threads

    pipename:  named pipe/fifo fifo location
               absolute or relative path, required when `command` is defined

    postcmd:   command to run after the log closes or rotates, %file = filename
               string passed to bash -c, optional, working directory is `dirname`
               used when `command` is defined (log ingestion) or `template` is defined (slm)
               unused when `command` and `template` are not defined
               default none

    retmax:    maximum number of logs to retain, regardless of expire time; 0 = no max (off)
               default off

    retmin:    minimum number of logs to retain, regardless of expire time; 0 = none (off)
               default off

    rmdir:     remove empty directories; true, false
               default 0/false

    rotatesiz: wrk and slm threads: rotate size, units M = MiB, G = GiB; 0 = no rotate (off)
               default off (no rotate)

    subdirs:   option to search subdirectories; true, false
               default 0/false

    symlinks:  follow symlinks; true, false
               default 0/false

    template:  output file name, date(1) sequences %F %Y %m %d %H %M %S %s
               relative to dirname, required when `command` is defined

    terse:     file removal notification; true = quiet, false = record file/directory removal
               default 0/false

    truncate:  truncate slm-managed files; true, false
               default 0/false

    uid:       worker and `postcmd` username or uid; username root ok, uid 0 not ok
               default nobody

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

    %path:  current dirname (path)
    %file:  current filename
    %host:  system hostname (nodename)
    %sect:  section name

## Threads

Thread names have a kernel-imposed length limit of 16 characters (15 + nul).
Thread names are assigned `<sectionname>_<taskname>`.  Task names and purposes:

    dfs:  filesystem free space monitor thread
    exp:  file expire monitor thread
    slm:  simple log monitor thread
    wrk:  worker (log ingestion) thread

Example: file expire thread name for the section called `console`: `console_exp`

## Debugging INI Files

sentinal accepts two options for debugging.
`-d|--debug` prints INI files as parsed.
`-v|--verbose` prints INI files as interpreted and the threads that will run.

    $ /opt/sentinal/bin/sentinal -f /opt/sentinal/tests/test4.ini -d

    [test4]
    command   = /usr/bin/zstd -T4
    dirname   = /opt/sentinal/tests
    dirlimit  = 
    subdirs   = true
    pipename  = test4.fifo
    template  = test4-%Y-%m-%d_%H-%M-%S.log.zst
    pcrestr   = test4-
    uid       = sentinal
    gid       = sentinal
    rotatesiz = 1G
    expiresiz = 
    diskfree  = 85
    inofree   = 
    expire    = 
    retmin    = 3
    retmax    = 25
    terse     = 
    rmdir     = 
    symlinks  = 
    postcmd   = 
    truncate  = 

    $ /opt/sentinal/bin/sentinal -f /opt/sentinal/tests/test4.ini -v

    [test4]
    command   = /usr/bin/zstd -T4
    #           zstd -f -T4 > /opt/sentinal/tests/test4-2022-07-03_12-12-23.log.zst
    dirname   = /opt/sentinal/tests
    dirlimit  = 0MiB
    subdirs   = 1
    pipename  = /opt/sentinal/tests/test4.fifo
    template  = test4-%Y-%m-%d_%H-%M-%S.log.zst
    #           test4-2022-07-03_12-12-23.log.zst
    pcrestr   = test4-
    uid       = 1324
    gid       = 1324
    rotatesiz = 1024MiB
    expiresiz = 0MiB
    diskfree  = 85.00
    inofree   = 0.00
    expire    = 0m
    retmin    = 3
    retmax    = 25
    terse     = 0
    rmdir     = 0
    symlinks  = 0
    postcmd   = 
    truncate  = 0
    # threads   dfs: true   exp: true   slm: false   wrk: true

## Key Usage

The following table lists the possible INI file keys and the threads
in which they can be used.

| Key | Pertains to |
|:---:|:---:|
| command | wrk |
| dirlimit | exp |
| dirname | dfs exp slm wrk |
| diskfree | dfs |
| expire | exp |
| expiresiz | exp |
| gid | slm wrk |
| inofree | dfs |
| pcrestr | dfs exp wrk |
| pipename | wrk |
| postcmd | slm wrk |
| retmax | exp |
| retmin | dfs exp |
| rmdir | dfs exp |
| rotatesiz | slm wrk |
| subdirs | dfs exp |
| symlinks | dfs exp |
| template | slm wrk |
| terse | dfs exp |
| truncate | slm |
| uid | slm wrk |

