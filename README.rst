dyn-nsupdate: Dynamically and securely update DNS zones via CGI
===============================================================

Introduction
------------

Welcome to dyn-nsupdate_, a collection of tools using BIND_, CGI_ and Python_ to 
provide DynDNS services. Both IPv4 and IPv6 are fully supported.

dyn-nsupdate consists of two pieces: The server part provides a way to update IP 
addresses in Bind's DNS zone via CGI, in a safe manner. The client part uses CGI 
to update some domain to the current address(es) of the machine it is running 
on. Alternatively, some routers can be configured to do this themselves. The 
FritzBox is known to be supported.

.. _dyn-nsupdate: https://www.ralfj.de/projects/dyn-nsupdate
.. _BIND: https://www.isc.org/downloads/bind/
.. _CGI: https://en.wikipedia.org/wiki/Common_Gateway_Interface
.. _Python: https://www.python.org/

Server Setup
------------

In the following, replace ``dyn.example.com`` by whatever domain will be managed 
through DynDNS. I assume that BIND has already been set up for 
``dyn.example.com`` as a dynamic zone that can be updated through ``nsupdate 
-l``. This can be achieved by setting ``update-policy local;`` in the zone 
configuration. Furthermore, I assume the directory ``/var/lib/bind/`` exists.

There are two pieces that have to be installed: A setuid wrapper which checks 
the passwords, and applies the updates; and some CGI scripts offered through a 
webserver.

Let's start with the setuid wrapper. To compile it, you will need cmake and 
boost, including the regex and program_options boost packages. Starting in the 
source directory, run::

  cd nsupd-wrapper
  mkdir -p build
  cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release -DDYNNSUPDATE_CONFIG_FILE=/var/lib/bind/dyn-nsupdate.conf
  make

This should compile the binary ``dyn-nsupdate``. If you want to put the files in 
another directory, change the configuration file name accordingly. You can now 
install it and the sample configuration file, and set their permissions::

  sudo install dyn-nsupdate /var/lib/bind/dyn-nsupdate -o bind -g bind -m +rx,u+ws
  sudo install ../../dyn-nsupdate.conf /var/lib/bind/dyn-nsupdate.conf -o bind -g bind -m u+rw

Finally, edit the config file. The format should be pretty self-explanatory. In 
particular, *change the password*!

Now, let's go on with the CGI scripts. They are using Python 2, so make sure you 
have that installed. There are two scripts: One is used for clients to detect 
their current external IP address, and one is used to do the actual update of 
the domain. The first script should be available on a domain that is available 
only through a single protocol, i.e., IPv4 only or IPv6 only. If you want to 
support both IPv4 and IPv6, I suggest you have three domains 
``ipv4.ns.example.com``, ``ipv6.ns.example.com`` and ``ns.example.com`` where 
only the latter is available via both protocols (this is something you have to 
configure in your ``example.com`` zone). All can serve the same scripts (e.g. 
via a ``ServerAlias`` in the apache configuration). I also **strongly suggest** 
you make these domains *HTTPS-only*, as the client script will send a password!

Choose some directory (e.g., ``/srv/ns.example.com``) for the new domain, and 
copy the content of ``server-scripts`` there. Now configure your webserver 
appropriately for CGI scripts to be executed there. You can find a sample 
configuration for apache in ``apache-ns.example.com.conf``. If you used a 
non-default location for the ``dyn-nsupdate`` wrapper, you have to change the 
path in the ``update`` CGI script accordingly.

That's it! Your server is now configured. You can use ``curl`` to test your 
setup::

  curl 'https://ns.example.com/update?domain=test.dyn.example.com&password=some_secure_password&ip=127.0.0.1'


Client setup (using the script)
-------------------------------

You can find the client script at ``client-scripts/dyn-ns-client``. It requires 
Python 3. Copy that script to the machine that should be available under the 
dynamic domain. Then change the configuration section at the top to match your 
setup. Note that the script can update a list of domain names, in case you need 
the machine to have several names (it is preferable to use a CNAME instead, this 
will reduce the number of updates performed in the zone). The ``serverIPv4`` and 
``serverIPv6`` are only used if IPv4/IPv6 is enabled. These machines must be 
available with that protocol only, otherwise it is not possible to reliably 
detect the current external address.

To run the script regularly, simply set up a cronjob. You can do so by running 
``crontab -e``, and add a line as follows::

  */15 * * * * /home/user/dyn-ns-client

This sets the update interval to 15min. If your IP address changes daily, you 
may want to reduce this to 5min to have a smaller timeframe during which your 
server is not available.

Client setup (using a router)
-----------------------------

Some routers are able to perform the update of the domain names themselves. The 
FritzBox is known to be supported. To configure it to tell your server about the 
current IP address, go to the DynDNS configuration section of the FritzBox and 
choose the "custom" DynDNS provider. Then enter the following settings:

- Update-URL: ``https://ns.example.com/update?domain=<domain>&password=<pass>&ip=<ipaddr>``
- Domain Name: ``test.dyn.example.com``
- User Name: ``just_something``
- Password: ``some_secure_password``

Note that the user name is ignored.



Source, License
---------------

You can find the sources in the `git repository`_. They are provided under a 
2-clause BSD license.

.. _git repository: http://www.ralfj.de/git/dyn-nsupdate.git

Contact
-------

If you found a bug, or want to leave a comment, please
`send me a mail <mailto:post-AT-ralfj-DOT-de>`_.
