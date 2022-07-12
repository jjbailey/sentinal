## Linux Page Cache

Linux tools that need to search large directories with millions of files can sometimes run slowly.  Programs such as find, which might generally take seconds to run,
can instead take minutes or hours to run.  sentinal is no exception.

Linux provides a page cache for access to recently-used filesystem blocks to improve performance.
Keeping a directory's disk blocks in the cache can significantly enhance sentinal's
performance and the performance of other programs reading from the same directory.

### Example

This sentinal section attempts to cache a large directory's disk blocks by running an
pagecache thread to continuously scan /path/to/storage:

    [pagecache]
    dirname   = /path/to/storage
    subdirs   = true

This section is about the least amount of work a thread can do. Adding it to an INI
file for deep directory scans is a net gain. Depending on the amount of system RAM
and demand, a page cache thread might not provide any meaningful improvement.

### References

https://www.cs.princeton.edu/courses/archive/fall19/cos316/lectures/11-page-cache.pdf
\
https://medium.com/marionete/linux-disk-cache-was-always-there-741bef097e7f
\
https://en.wikipedia.org/wiki/Page_cache

