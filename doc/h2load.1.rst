
.. GENERATED by help2rst.py.  DO NOT EDIT DIRECTLY.

.. program:: h2load

h2load(1)
=========

SYNOPSIS
--------

**h2load** [OPTIONS]... [URI]...

DESCRIPTION
-----------

benchmarking tool for HTTP/2 and SPDY server

.. describe:: <URI>

    Specify URI to access.   Multiple URIs can be specified.
    URIs are used  in this order for each  client.  All URIs
    are used, then  first URI is used and then  2nd URI, and
    so  on.  The  scheme, host  and port  in the  subsequent
    URIs, if present,  are ignored.  Those in  the first URI
    are used solely.  Definition of a base URI overrides all
    scheme, host or port values.

OPTIONS
-------

.. option:: -n, --requests=<N>

    Number of requests.

    Default: ``1``

.. option:: -c, --clients=<N>

    Number of concurrent clients.

    Default: ``1``

.. option:: -t, --threads=<N>

    Number of native threads.

    Default: ``1``

.. option:: -i, --input-file=<PATH>

    Path of a file with multiple URIs are separated by EOLs.
    This option will disable URIs getting from command-line.
    If '-' is given as <PATH>, URIs will be read from stdin.
    URIs are used  in this order for each  client.  All URIs
    are used, then  first URI is used and then  2nd URI, and
    so  on.  The  scheme, host  and port  in the  subsequent
    URIs, if present,  are ignored.  Those in  the first URI
    are used solely.  Definition of a base URI overrides all
    scheme, host or port values.

.. option:: -m, --max-concurrent-streams=(auto|<N>)

    Max concurrent streams to  issue per session.  If "auto"
    is given, the number of given URIs is used.

    Default: ``auto``

.. option:: -w, --window-bits=<N>

    Sets the stream level initial window size to (2\*\*<N>)-1.
    For SPDY, 2**<N> is used instead.

    Default: ``30``

.. option:: -W, --connection-window-bits=<N>

    Sets  the  connection  level   initial  window  size  to
    (2**<N>)-1.  For SPDY, if <N>  is strictly less than 16,
    this option  is ignored.   Otherwise 2\*\*<N> is  used for
    SPDY.

    Default: ``30``

.. option:: -H, --header=<HEADER>

    Add/Override a header to the requests.

.. option:: --ciphers=<SUITE>

    Set allowed  cipher list.  The  format of the  string is
    described in OpenSSL ciphers(1).

.. option:: -p, --no-tls-proto=<PROTOID>

    Specify ALPN identifier of the  protocol to be used when
    accessing http URI without SSL/TLS.
    Available protocols: spdy/2, spdy/3, spdy/3.1 and h2c

    Default: ``h2c``

.. option:: -d, --data=<PATH>

    Post FILE to  server.  The request method  is changed to
    POST.

.. option:: -r, --rate=<N>

    Specifies  the  fixed  rate  at  which  connections  are
    created.   The   rate  must   be  a   positive  integer,
    representing the  number of  connections to be  made per
    second.  When the rate is 0,  the program will run as it
    normally does, creating connections at whatever variable
    rate it wants.  The default value for this option is 0.

.. option:: -C, --num-conns=<N>

    Specifies  the total  number of  connections to  create.
    The  total  number of  connections  must  be a  positive
    integer.  On each connection, :option:`-m` requests are made.  The
    test  stops once  as soon  as the  <N> connections  have
    either  completed   or  failed.   When  the   number  of
    connections is  0, the program  will run as  it normally
    does, creating as many connections  as it needs in order
    to make  the :option:`-n`  requests specified.  The  default value
    for this option is 0.  The  :option:`-n` option is not required if
    the :option:`-C` option is being used.

.. option:: -T, --connection-active-timeout=<N>

    Specifies  the maximum  time that  h2load is  willing to
    keep a  connection open,  regardless of the  activity on
    said  connection.   <N>  must  be  a  positive  integer,
    specifying  the  number of  seconds  to  wait.  When  no
    timeout value is set (either active or inactive), h2load
    will keep a connection  open indefinitely, waiting for a
    response.

.. option:: -N, --connection-inactivity-timeout=<N>

    Specifies the amount  of time that h2load  is willing to
    wait to see activity on a given connection.  <N> must be
    a positive integer, specifying  the number of seconds to
    wait.  When  no timeout value  is set (either  active or
    inactive),   h2load   will   keep  a   connection   open
    indefinitely, waiting for a response.

.. option:: --timing-script-file=<PATH>

    Path of a file containing one  or more lines separated by
    EOLs. Each script line  is composed of  two tab-separated
    fields. The first field  represents  the time offset from
    the  start of  execution, expressed  as milliseconds with
    microsecond  resolution.  The second field represents the
    URI.   This   option  will   disable  URIs  getting  from
    command-line.  If '-'  is given as <PATH>,  script  lines
    will be read from stdin.  Script lines are used  in order
    for each  client.  If  :option:`-n` is  given, it must be less than
    or equal to the number of script lines, larger values are
    clamped  to  the   number  of  script  lines.  If  :option:`-n`  is
    not given,  the number of requests  will  default to  the
    number of script lines. The scheme, host and port defined
    in the  first URI  are used  solely.  Values contained in
    other URIs, if  present, are  ignored.  Definition  of  a
    base  URI  overrides  all  scheme, host  or port  values.

.. option:: -B, --base-uri=<URI>

    Specify URI from which the scheme, host and port will be
    used  for  all requests.   The  base  URI overrides  all
    values  defined either  at  the command  line or  inside
    input files.

.. option:: -v, --verbose

    Output debug information.

.. option:: --version

    Display version information and exit.

.. option:: -h, --help

    Display this help and exit.

OUTPUT
------

requests
  total
    The number of requests h2load was instructed to make.
  started
    The number of requests h2load has started.
  done
    The number of requests completed.
  succeeded
    The number of requests completed successfully.  Only HTTP status
    code 2xx or3xx are considered as success.
  failed
    The number of requests failed, including HTTP level failures
    (non-successful HTTP status code).
  errored
    The number of requests failed, except for HTTP level failures.
    This is the subset of the number reported in ``failed`` and most
    likely the network level failures or stream was reset by
    RST_STREAM.
  timeout
    The number of requests whose connection timed out before they were
    completed.   This  is  the  subset   of  the  number  reported  in
    ``errored``.

status codes
  The number of status code h2load received.

traffic
  total
    The number of bytes received from the server "on the wire".  If
    requests were made via TLS, this value is the number of decrpyted
    bytes.
  headers
    The number of response header bytes from the server without
    decompression.  For HTTP/2, this is the sum of the payload of
    HEADERS frame.  For SPDY, this is the sum of the payload of
    SYN_REPLY frame.
  data
    The number of response body bytes received from the server.

time for request
  min
    The minimum time taken for request and response.
  max
    The maximum time taken for request and response.
  mean
    The mean time taken for request and response.
  sd
    The standard deviation of the time taken for request and response.
  +/- sd
    The fraction of the number of requests within standard deviation
    range (mean +/- sd) against total number of successful requests.

time for connect
  min
    The minimum time taken to connect to a server.
  max
    The maximum time taken to connect to a server.
  mean
    The mean time taken to connect to a server.
  sd
    The standard deviation of the time taken to connect to a server.
  +/- sd
    The  fraction  of  the   number  of  connections  within  standard
    deviation range (mean  +/- sd) against total  number of successful
    connections.

time for 1st byte (of (decrypted in case of TLS) application data)
  min
    The minimum time taken to get 1st byte from a server.
  max
    The maximum time taken to get 1st byte from a server.
  mean
    The mean time taken to get 1st byte from a server.
  sd
    The standard deviation of the time taken to get 1st byte from a
    server.
  +/- sd
    The fraction of the number of connections within standard
    deviation range (mean +/- sd) against total number of successful
    connections.

FLOW CONTROL
------------

h2load sets large flow control window by default, and effectively
disables flow control to avoid under utilization of server
performance.  To set smaller flow control window, use :option:`-w` and
:option:`-W` options.  For example, use ``-w16 -W16`` to set default
window size described in HTTP/2 and SPDY protocol specification.

SEE ALSO
--------

:manpage:`nghttp(1)`, :manpage:`nghttpd(1)`, :manpage:`nghttpx(1)`
