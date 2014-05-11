#!/usr/bin/python
import urllib2, socket, urllib, sys

# configuration variables
server = 'ns.ralfj.de'
domains = ['your.domain.ralfj.de'] # list of domains to update
password = 'yourpassword'
# END of configuration variables

myip = urllib2.urlopen('https://'+server+'/checkip').read().strip()

def update_domain(domain):
	'''Update the given domain, using the global server, user, password. Returns True on success, False on failure.'''
	global myip
	# check if the domain is already mapped to our current IP
	domainip = socket.gethostbyname(domain)
	if myip == domainip:
		# nothing to do
		return True

	# we need to update the IP
	result = urllib2.urlopen('https://'+server+'/update?password='+urllib.quote(password)+'&domain='+urllib.quote(domain)+'&ip='+urllib.quote(myip)).read().strip()
	if 'good '+myip == result: 
		# all went all right
		return True
	else:
		# Something went wrong
		print "Unexpected answer from server",server,"while updating",domain,"to",myip
		print result
		return False

exitcode = 0
for domain in domains:
	if not update_domain(domain):
		exitcode = 1
sys.exit(exitcode)
