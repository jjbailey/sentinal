### Examples of pcretest Usage

#### Ends with a digit

    $ pcretest '/var/log/sys.*\d$' /var/log/sys*
    no match: /var/log/syslog
    match:    /var/log/syslog.1
    no match: /var/log/syslog.2.gz

#### Negative lookahead for pid files

    $ pcretest '^(?!.*\.(?:pid)$)' notapidfile pidfile.pid
    match:    notapidfile
    no match: pidfile.pid

#### Negative lookbehind for pid files

    $ pcretest '.*(?<!\.pid)$' notapidfile pidfile.pid
    match:    notapidfile
    no match: pidfile.pid

#### Negative lookahead for compressed files

    $ pcretest '^(?!.*\.(?:bz2|gz|lz|zip|zst)$)' $(find testdir -type f)
    no match: testdir/ddrescue-1.25.tar.lz
    match:    testdir/nxserver.log
    no match: testdir/syslog.2.gz
