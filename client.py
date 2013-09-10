#!/usr/bin/python
import urllib2, socket, urllib, sys

# configuration variables
server = 'ns.ralfj.de'
user = 'yourusername'
password = 'yourpassword'
domain = 'your.domain.ralfj.de'
# END of configuration variables

# check if the domain is already mapped to our current IP
myip = urllib2.urlopen('https://'+server+'/checkip').read().strip()
currip = socket.gethostbyname(domain)
if myip == currip:
	# nothing to do
	sys.exit(0)

# we need to update the IP
result = urllib2.urlopen('https://'+server+'/update?user='+urllib.quote(user)+'&password='+urllib.quote(password)+'&domain='+urllib.quote(domain)+'&ip='+urllib.quote(myip)).read().strip()
if 'good '+myip == result: 
	# nothing to do, all went all right
	sys.exit(0)

# there was an error :(
print result
sys.exit(1)
