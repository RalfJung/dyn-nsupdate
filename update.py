#!/usr/bin/python
import cgi, os, sys, subprocess
import cgitb
cgitb.enable()
form = cgi.FieldStorage()

# print headers
print "Content-Type: text/plain"
print ""

# get input
if "user" not in form or "password" not in form or "domain" not in form or "ip" not in form:
    print "Error:"
    print "Mandatory argument missing: You must supply all of 'user', 'password', 'domain', 'ip'"
    sys.exit()

ip = form["ip"].value
domain = form["domain"].value
user = form["user"].value
password = form["password"].value

# run update program
p = subprocess.Popen(["/var/lib/named/dyn-nsupdate", user, password, domain, ip], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
(stdout, stderr) = p.communicate()
if stdout: raise Exception("Unexpected output from dyn-nsupdate")

# check what it did
if p.returncode == 1:
	# error :/
	print "There was an error while updating the DNS:"
	print stderr
else:
	# all right!
	if p.returncode or stderr:
		raise Exception("Unexpected return code or output from dyn-nsupdate")
	print "good",ip
