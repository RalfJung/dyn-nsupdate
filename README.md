# dyn-nsupdate: Self-made DynDNS

## Introduction

Welcome to [dyn-nsupdate](https://www.ralfj.de/projects/dyn-nsupdate),
a collection of tools using
[BIND](https://www.isc.org/downloads/bind/),
[CGI](https://en.wikipedia.org/wiki/Common_Gateway_Interface) and
[Python](https://www.python.org/) to provide DynDNS services with your
own nameserver. Both IPv4 and IPv6 are fully supported.

dyn-nsupdate consists of two pieces: The server part provides a way to update IP 
addresses in Bind's DNS zones via CGI, in a safe manner. The client part uses CGI
to update some domain to the current address(es) of the machine it is running 
on. Alternatively, some routers can be configured to do this themselves. The 
FritzBox is known to be supported.

## Server Setup

In the following, replace `dyn.example.com` by whatever domain will be managed 
through DynDNS. I assume that BIND has already been set up for 
`dyn.example.com` as a dynamic zone that can be updated through `nsupdate 
-l`. This can be achieved by setting `update-policy local;` in the zone 
configuration. Furthermore, I assume the directory `/var/lib/bind/` exists.

There are two pieces that have to be installed: A setuid wrapper which checks 
the passwords, and applies the updates; and some CGI scripts offered through a 
webserver. Please read this guide carefully and make sure you understand the
security implications of what you are doing. setuid wrappers are not toys!

Let's first set up the setuid wrapper. To compile it, you will need cmake and 
boost, including the regex and program_options boost packages. Starting in the 
source directory, run::

    cd nsupd-wrapper
    mkdir -p build
    cd build
    DIR=/var/lib/bind
    cmake .. -DCMAKE_BUILD_TYPE=Release -DDYNNSUPDATE_CONFIG_FILE=$DIR/dyn-nsupdate.conf
    make

This should compile the binary `dyn-nsupdate`. Notice that the path to the 
configuration file will be hard-coded into the binary. If it were run-time 
configurable, then a user could call the script with her own configuration file, 
gaining access to all domains BIND lets you configure. If you want to put the 
files in another directory, change the configuration file name accordingly. Make 
sure the file (and all of the directories it is in) can *not be written by 
non-root*. The setuid wrapper trusts that file. You can now install it and the 
sample configuration file, and set their permissions::

    sudo install dyn-nsupdate $DIR/dyn-nsupdate -o bind -g bind -m +rx,u+ws
    sudo install ../../dyn-nsupdate.conf.dist $DIR/dyn-nsupdate.conf -o bind -g bind -m u+rw

Finally, edit the config file. The format should be pretty self-explanatory. In 
particular, **change the password**!

Now, let's go on with the CGI scripts. They are using Python 2, so make sure you 
have that installed. There are two scripts: One is used for clients to detect 
their current external IP address, and one is used to do the actual update of 
the domain. The first script is used by the "web" IP detection method (see 
client configuration below). It should be available on a domain that is 
available only through a single protocol, i.e., IPv4 only or IPv6 only. This is 
required to reliably detect the current address of the given protocol. If you 
want to support both IPv4 and IPv6, I suggest you have three domains 
`ipv4.ns.example.com`, `ipv6.ns.example.com` and `ns.example.com` where 
only the latter is available via both protocols (this is something you have to 
configure in your `example.com` DNS zone). All can serve the same scripts 
(e.g. via a `ServerAlias` in the apache configuration). I also **strongly 
suggest** you make these domains *HTTPS-only*, as the client script will send a 
password!

Choose some directory (e.g., `/srv/ns.example.com`) for the new domain, and 
copy the content of `server-scripts` there. Now configure your webserver 
appropriately for CGI scripts to be executed there. You can find a sample 
configuration for apache in `apache-ns.example.com.conf`. If you used a 
non-default location for the `dyn-nsupdate` wrapper, you have to change the 
path in the `update` CGI script accordingly.

That's it! Your server is now configured. You can use `curl` to test your 
setup::

    DOMAIN=test.dyn.example.com
    PW=some_secure_password
    curl 'https://ns.example.com/update?domain=$DOMAIN&password=$PW&ip=127.0.0.1'


## Client setup (using the script)

You can find the client script at `client-scripts/dyn-ns-client`. It requires 
Python 3. Copy that script to the machine that should be available under the 
dynamic domain. Also copy the sample configuration file 
`dyn-ns-client.conf.dist` to `$HOME/.config/dyn-nsupdate/dyn-ns-client.conf`.
You can choose another name, but then you will have to tell the script about it. 
Call `dyn-ns-client --help` for this and other options the script accepts. An 
important aspect of configuration is how to detect the current addresses of the 
machine the script is running on. For IPv4, this can only be "web", which can 
deal with NAT. For IPv6, the script can alternatively attempt to detect the 
correct local address to use. The sample file contains comments that should 
explain everything.

Note that the script can update a list of domain names, in case you need the 
machine to have several names. It is preferable to use a CNAME instead, this 
will reduce the number of updates performed in the zone.

To run the script regularly, simply set up a cronjob. You can do so by running 
`crontab -e`, and add a line as follows::

    */15 * * * * /home/user/dyn-ns-client

This sets the update interval to 15min. If your IP address changes daily, you 
may want to reduce this to 5min to have a smaller timeframe during which your 
server is not available.

If you want to be emailed about changes in your IP address, pass `-v` as 
argument. The script will then only produce output if it has to update the DNS
record.

## Client setup (using a router)

Some routers are able to perform the update of the domain names themselves. The 
FritzBox is known to be supported. To configure it to tell your server about the 
current IP address, go to the DynDNS configuration section of the FritzBox and 
choose the "custom" DynDNS provider. Then enter the following settings:

- Update-URL: `https://ns.example.com/update?domain=<domain>&password=<pass>&ip=<ipaddr>`
- Domain Name: `test.dyn.example.com`
- User Name: `just_something`
- Password: `some_secure_password`

Note that the user name is ignored.



## Source, License

You can find the sources in the
[git repository](http://www.ralfj.de/git/dyn-nsupdate.git) (also
available [on GitHub](https://github.com/RalfJung/dyn-nsupdate)).  They
are provided under a
[2-clause BSD license](http://opensource.org/licenses/bsd-license.php). See
the file `LICENSE-BSD` for more details.

## Contact

If you found a bug, or want to leave a comment, please
[send me a mail](mailto:post-AT-ralfj-DOT-de). All sorts of feedback are
welcome :)
