# simple pyhton precompiler

import sys
import string

def flatten(s):
	ret = ""
	for a in s:
		if a[len(a) - 1] == "\n":
			a = a[:len(a) - 1]
		ret += transformline("." + a)
	return ret 

# \ -> \\
# " -> \"
# $S$ -> " + str(S) + "
def prefix(s):
	ret = ""
	insideVarname = 0
	for i in range(len(s)):
		if s[i] == '\\':
			ret = ret + "\\\\"
			continue	
		if s[i] == '\"':
			ret = ret + "\\\""
			continue
		if insideVarname == 1:
			if s[i] == '$':			
				insideVarname = 0
			continue
		if s[i] == '$':
			varend = string.find(s, "$", i+1)
			varname = s[i+1 : varend]
			ret = ret + "\" + str(" + varname + ") + \""
			insideVarname = 1
			continue
		# default
		ret = ret + s[i]
	ret = ret + "\\n";
	return ret		
	
# WhiteSpace.S -> WhiteSpace f.write("S\n")
# .attr["S"] -> .attributes[(None, "S")].nodeValue
def transformline(s):
	startsWithWhitespace = 1
	placeOfDot = -1
	for i in range(len(s)):
		if s[i] == " ":
			continue
		if s[i] == "\t":
			continue
		if s[i] == ".":
			placeOfDot = i
			break
		#default
		startsWithWhitespace = 0

	if (placeOfDot > -1) and (startsWithWhitespace == 1):
		ret = s[0:placeOfDot]
		ret = ret + "f.write(\"" + prefix(s[placeOfDot+1:]) + "\")" + "\n"
	elif string.find(s, ".attr[") > -1:
		pos = string.find(s, ".attr[");
		pos2 = string.find(s, "]", pos);
		ret = s[:pos] + ".attributes[(None, " + s[pos+6:pos2] + ")].nodeValue" + s[pos2+1:] + "\n"
	elif string.find(s, ":") == 0:
		includeName = s[1:]
		finclude = open(includeName, "r")
		ret = flatten(finclude.readlines())
		finclude.close
	else:
		ret = s	+ "\n"
	return ret

# main
lines = []
fin = open(sys.argv[1], "r")
lines = fin.readlines()
fin.close

fout = open(sys.argv[2], "w")
for line in lines:
	# strip newline
	if line[len(line) - 1] == "\n":
		line = line[:len(line) - 1]
	fout.write(transformline(line))
fout.close
