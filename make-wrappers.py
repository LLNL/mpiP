#!/usr/local/bin/python 
# -*- python -*-
#
# Jeffrey Vetter <vetter@llnl.gov>
# Lawrence Livermore National Lab, Center for Applied Scientific Computing
# 04 Oct 2000
#

# make-wrappers.py -- parse the mpi prototype file and generate a
# series of output files, which include the wrappers for profiling
# layer and other data structures.

import sys
import string
import os
import copy
import re
import time
import getopt
import socket
import pdb

cnt = 0
fdict = {}
lastFunction = "NULL"
verbose = 0
baseID = 1000

messParamDict = {

    ( "MPI_Allgather", "sendcount"):1,
    ( "MPI_Allgather", "sendtype"):2,
    ( "MPI_Allgatherv", "sendcount"):1,
    ( "MPI_Allgatherv", "sendtype"):2,
    ( "MPI_Allreduce", "count"):1,
    ( "MPI_Allreduce", "datatype"):2,
    ( "MPI_Alltoall", "sendcount"):1,
    ( "MPI_Alltoall", "sendtype"):2,
    ( "MPI_Bcast", "count"):1,
    ( "MPI_Bcast", "datatype"):2,
    ( "MPI_Bsend", "count"):1,
    ( "MPI_Bsend", "datatype"):2,
    ( "MPI_Gather", "sendcnt"):1,
    ( "MPI_Gather", "sendtype"):2,
    ( "MPI_Gatherv", "sendcnt"):1,
    ( "MPI_Gatherv", "sendtype"):2,
    ( "MPI_Ibsend", "count"):1,
    ( "MPI_Ibsend", "datatype"):2,
    ( "MPI_Irsend", "count"):1,
    ( "MPI_Irsend", "datatype"):2,
    ( "MPI_Isend", "count"):1,
    ( "MPI_Isend", "datatype"):2,
    ( "MPI_Issend", "count"):1,
    ( "MPI_Issend", "datatype"):2,
    ( "MPI_Reduce", "count"):1,
    ( "MPI_Reduce", "datatype"):2,
    ( "MPI_Rsend", "count"):1,
    ( "MPI_Rsend", "datatype"):2,
    ( "MPI_Scan", "count"):1,
    ( "MPI_Scan", "datatype"):2,
    ( "MPI_Scatter", "sendcnt"):1,
    ( "MPI_Scatter", "sendtype"):2,
    ( "MPI_Send", "count"):1,
    ( "MPI_Send", "datatype"):2,
    ( "MPI_Sendrecv", "sendcount"):1,
    ( "MPI_Sendrecv", "sendtype"):2,
    ( "MPI_Sendrecv_replace", "count"):1,
    ( "MPI_Sendrecv_replace", "datatype"):2,
    ( "MPI_Ssend", "count"):1,
    ( "MPI_Ssend", "datatype"):2
    }

ioParamDict = {

    ( "MPI_File_read", "count"):1,
    ( "MPI_File_read", "datatype"):2,
    ( "MPI_File_read_all", "count"):1,
    ( "MPI_File_read_all", "datatype"):2,
    ( "MPI_File_read_at", "count"):1,
    ( "MPI_File_read_at", "datatype"):2,
    ( "MPI_File_write", "count"):1,
    ( "MPI_File_write", "datatype"):2,
    ( "MPI_File_write_all", "count"):1,
    ( "MPI_File_write_all", "datatype"):2,
    ( "MPI_File_write_at", "count"):1,
    ( "MPI_File_write_at", "datatype"):2
    }

noDefineList = [
    "MPI_Pcontrol"
    ]

class VarDesc:
    def __init__ (self,name, basetype, pointerLevel, arrayLevel):
	"initialize a new variable description structure"
	self.name = name
	self.basetype = basetype
	self.pointerLevel = pointerLevel
	self.arrayLevel = arrayLevel
	self.recordIt = 0

class fdecl:
    def __init__ (self,name, id, returntype, paramList, protoline):
	"initialize a new function declaration structure"
	self.name = name
	self.id = id
	self.returntype = returntype
	self.paramList = paramList
	self.paramDict = {}
	self.protoline = protoline
	self.wrapperPreList = []
	self.wrapperPostList = []
	self.nowrapper = 0
	self.paramConciseList = []
	self.extrafields = {}
	self.extrafieldsList = []
        self.sendCountPname = ""
        self.sendTypePname = ""
        self.recvCountPname = ""
        self.recvTypePname = ""
        self.ioCountPname = ""
        self.ioTypePname = ""

class xlateEntry:
    def __init__ (self,mpiType,varName):
        "initialize a new Fortran translation structure"
        self.mpiType = mpiType
        self.varName = varName
        

def ProcessDirectiveLine(lastFunction, line):
    tokens = string.split(line)
    if tokens[0] == "nowrapper":
	fdict[lastFunction].nowrapper = 1
    elif tokens[0] == "extrafield":
	fdict[lastFunction].extrafieldsList.append(tokens[2])
	fdict[lastFunction].extrafields[tokens[2]] = tokens[1]
    else:
	print "Warning: ",lastFunction," unknown directive [",string.strip(line),"]"


def ProcessWrapperPreLine(lastFunction, line):
    #print "Processing wrapper pre [",string.strip(line),"] for ",lastFunction
    fdict[lastFunction].wrapperPreList.append(line)


def ProcessWrapperPostLine(lastFunction, line):
    #print "Processing wrapper post [",string.strip(line),"] for ",lastFunction
    fdict[lastFunction].wrapperPostList.append(line)


def DumpDict():
    for i in flist:
	print i
	if verbose:
	    print "\tParams\t",fdict[i].paramList
	    if fdict[i].wrapperPreList:
		print "\tpre\t", fdict[i].wrapperPreList
	    if fdict[i].wrapperPostList:
		print "\tpost\t", fdict[i].wrapperPostList


#####
##### Some MPI types we want to record. If there are pointers to them, deref them.
##### To simplify, assume that the value can be an 'int' for printing and assignment.
#####
def SpecialParamRecord(funct,param):
    global flist
    global fdict
    basetype = fdict[funct].paramDict[param].basetype
    pointerLevel = fdict[funct].paramDict[param].pointerLevel
    arrayLevel = fdict[funct].paramDict[param].arrayLevel

    simplePointer = (pointerLevel == 1) and (arrayLevel == 0)

    if basetype == "MPI_Request" and simplePointer:
	return 1
    elif basetype == "MPI_Comm" and simplePointer:
	return 1
    elif basetype == "MPI_Datatype" and simplePointer:
	return 1
    elif basetype == "MPI_Group" and simplePointer:
	return 1
    elif basetype == "MPI_Info" and simplePointer:
	return 1
    elif basetype == "int" and simplePointer:
	return 1
    else:
	return 0



#####
##### ParamDictUpdate - updates the datastructures for a function after
#####    the basics have been entered. Must be called for each function.
#####
def ParamDictUpdate(fname):
    global flist
    global fdict
    global messParamDict
    global ioParamDict
    for p in fdict[fname].paramList:
	## check for pointers, arrays
	pname = "NULL"
	basetype = "NULL"
        pointerLevel = string.count(p,"*")
        arrayLevel = string.count(p,"[")
	if (pointerLevel > 0) and (arrayLevel > 0):
	    ## handle pointers and arrays
	    pname = p[string.rfind(p,"*")+1:string.find(p,"[")]
	    basetype = p[0:string.find(p,"*")]
	elif pointerLevel > 0:
	    ## handle pointers
	    pname = p[string.rfind(p,"*")+1:len(p)]
	    basetype = p[0:string.find(p,"*")]
	elif arrayLevel > 0:
	    ## handle arrays
	    pname = p[string.find(p," "):string.find(p,"[")]
	    basetype = p[0:string.find(p," ")]
	else:
	    ## normal hopefully :)
	    tokens = string.split(p)
	    if len(tokens) == 1:
		## must be void
		pname = ""
		basetype = "void"
	    else:
		pname = string.strip(tokens[1])
		basetype = string.strip(tokens[0])

	pname = string.strip(pname)
	basetype = string.strip(basetype)
	fdict[fname].paramDict[pname] = VarDesc(pname,basetype,pointerLevel,arrayLevel)
	fdict[fname].paramConciseList.append(pname)

        #  Identify and assign message size parameters
        if messParamDict.has_key((fname,pname)):
            paramMessType = messParamDict[(fname,pname)]
            if paramMessType == 1:
                fdict[fname].sendCountPname = pname
            elif paramMessType == 2:
                fdict[fname].sendTypePname = pname
            elif paramMessType == 3:
                fdict[fname].recvCountPname = pname
            elif paramMessType == 4:
                fdict[fname].recvTypePname = pname                
            
        #  Identify and assign io size parameters
        if ioParamDict.has_key((fname,pname)):
            paramMessType = ioParamDict[(fname,pname)]
            if paramMessType == 1:
                fdict[fname].ioCountPname = pname
            elif paramMessType == 2:
                fdict[fname].ioTypePname = pname
            
	if (fdict[fname].paramDict[pname].pointerLevel == 0) \
	   and (fdict[fname].paramDict[pname].arrayLevel == 0) \
	   and (fdict[fname].paramDict[pname].basetype != "void"):
	    fdict[fname].paramDict[pname].recordIt = 1
	elif SpecialParamRecord(fname,pname):
	    fdict[fname].paramDict[pname].recordIt = 1
	else:
	    pass

	if verbose:
	    #print "\t->",p
	    print "\t",pname, basetype, pointerLevel, arrayLevel


#####
##### Parses the input file and loads the information into the function dict.
#####
def ReadInputFile(f):
    # parser states
    p_start = "start"
    p_directives = "directives"
    p_wrapper_pre = "wrapper_pre"
    p_wrapper_post = "wrapper_post"
    parserState = p_start
    global cnt
    global fdict
    global flist

    fcounter = baseID

    print "-----*----- Parsing input file"
    while 1:
	##### read a line from input
	rawline = f.readline()
	if not rawline:
	    break
	cnt = cnt + 1
	line = re.sub("\@.*$","",rawline)

	##### break it into tokens
	tokens = string.split(line)
	if not tokens:
	    continue

	##### determine what type of line this is and then parse it as required
	if (string.find(line,"(") != -1) \
	   and (string.find(line,")") != -1) \
	   and (string.find(line,"MPI_") != -1) \
	   and parserState == p_start:
	    ##### we have a prototype start line
	    name = tokens[1]
	    retype = tokens[0]
	    lparen = string.index(line,"(")
	    rparen = string.index(line,")")
	    paramstr = line[lparen+1:rparen]
	    paramList = map(string.strip,string.split(paramstr,","))
	    #    print cnt, "-->", name,  paramList
	    fdict[name] = fdecl(name, fcounter, retype, paramList,line)
	    if name not in noDefineList:
	      fcounter = fcounter + 1
	    ParamDictUpdate(name)
	    lastFunction = name
	    if verbose:
		print name
	else:
	    ##### DIRECTIVES
	    if tokens[0] == "directives" and parserState != p_directives:
		##### beginning of directives
		parserState = p_directives
	    elif tokens[0] == "directives" and parserState == p_directives:
		##### end of directives
		parserState = p_start
	    elif parserState == p_directives:
		##### must be a directive, process it
		ProcessDirectiveLine(lastFunction, line)

	    ##### CODE WRAPPER PRE
	    elif tokens[0] == "wrapper_pre" and parserState != p_wrapper_pre:
		##### beginning of wrapper_pre
		parserState = p_wrapper_pre
	    elif tokens[0] == "wrapper_pre" and parserState == p_wrapper_pre:
		##### end of wrapper_pre
		parserState = p_start
	    elif parserState == p_wrapper_pre:
		##### must be a directive, process it
		ProcessWrapperPreLine(lastFunction, line)

	    ##### CODE WRAPPER POST
	    elif tokens[0] == "wrapper_post" and parserState != p_wrapper_post:
		##### beginning of wrapper_post
		parserState = p_wrapper_post
	    elif tokens[0] == "wrapper_post" and parserState == p_wrapper_post:
		##### end of wrapper_post
		parserState = p_start
	    elif parserState == p_wrapper_post:
		##### must be a directive, process it
		ProcessWrapperPostLine(lastFunction, line)

	    ##### UNKNOWN
	    else:
		print "Unknown input line ",cnt, ":", line,

    flist = fdict.keys()
    flist.sort()
    print "-----*----- Parsing completed: ", len(fdict), " functions found."


###
### create a standard file header and return the init list
###
def StandardFileHeader(fname):
    olist = []
    olist.append("/* " + fname + " */\n")
    olist.append("/* DO NOT EDIT -- AUTOMATICALLY GENERATED! */\n")
    olist.append("/* Timestamp: " + time.strftime("%d %B %Y %H:%M", time.localtime(time.time())) + "  */\n")
    olist.append("/* Location: " + socket.gethostname () + " " + os.name + " */\n")
    olist.append("/* Creator: " + os.environ["LOGNAME"] + "  */\n")
    olist.append("\n")
    olist.append("\n")
    return olist


###
### Scan the lists of all functions and look for optimization 
### opportunities (in space/speed).
###
### NOT USED
###
def ParameterOptimization():
    global flist
    global fdict
    ##### visit each function and update each functions parameter dictionary
    for funct in flist:
	if verbose:
	    print funct
	ParamDictUpdate(funct)



###
### Creates a structure entry for one function
###
def CreateStruct(funct,olist):
    global flist
    global fdict
    count = 0
    for p in fdict[funct].paramConciseList:
	if fdict[funct].paramDict[p].recordIt:
	    count = count + 1
    for f in fdict[funct].extrafieldsList:
	count = count + 1

    if count > 0:
	olist.append("\ntypedef struct _mpiTi_" + funct + "_t \n{\n")
	for p in fdict[funct].paramConciseList:
	    if fdict[funct].paramDict[p].recordIt:
		olist.append(fdict[funct].paramDict[p].basetype + "\t" + p + ";\n")
	for f in fdict[funct].extrafieldsList:
	    olist.append(fdict[funct].extrafields[f] + "\t" + f + ";\n")
	olist.append("} mpiTi_" + funct + "_t;\n")


###
### Creates the structure for storing extra parameters.
###
def CreateOptStruct(funct,olist):
    global flist
    global fdict
    count = 0
    for p in fdict[funct].paramConciseList:
	if fdict[funct].paramDict[p].recordIt:
	    count = count + 1
    for f in fdict[funct].extrafieldsList:
	count = count + 1

    if count > 0:
	olist.append("\tmpiTi_" + funct + "_t " + funct + ";\n")


###
### Create the structure files.
###
def GenerateStructureFile():
    global flist
    global fdict
    print "-----*----- Generating structure files"
    cwd = os.getcwd()
    os.chdir(cwd)
    sname = cwd + "/mpiPi_def.h"
    g = open(sname, "w")
    olist = StandardFileHeader(sname)
    
##### -jsv 8/14/01
#    olist.append("#include <mpi.h>\n")
    olist.append("\n")
    olist.append("#define mpiPi_BASE " + str(baseID) + "\n")
    olist.append("\n")

    for funct in flist:
      if funct not in noDefineList:
	olist.append("#define mpiPi_" + funct + " " + str(fdict[funct].id) + "\n")

    olist.append("\n")

##### -jsv 8/14/01
#     for funct in flist:
# 	CreateStruct(funct,olist)

#     olist.append("\n\n\ntypedef union _mpiTi_mpi_t\n{\n")
#     for funct in flist:
# 	CreateOptStruct(funct,olist)
#     olist.append("} mpiTi_mpi_t;\n")

    olist.append("\n")
    olist.append("/* eof */\n")
    g.writelines(olist)
    g.close()

###
### generates one print statement for one function incl parameters
###
def CreatePrintStmt(funct,olist):
    global flist
    global fdict
    count = 0
    
    olist.append("\n")
    olist.append("void\n")
    olist.append("mpiTi_eprint_" + funct + "(mpiTi_event_t*e)\n{\n")
    olist.append("fprintf(mpiTi.oFile,")
    olist.append("\"%lld %d %x %d \"")
    olist.append(" \""+funct+" \"")
    for param in fdict[funct].paramConciseList:
	if fdict[funct].paramDict[param].recordIt:
	    if fdict[funct].paramDict[param].basetype == "double":
		olist.append("\"%g \"")
	    else:
		olist.append("\"%d \"")
    for f in fdict[funct].extrafieldsList:
	olist.append("\"%d \"")
    olist.append("\"\\n\"")
    olist.append(",e->ts,e->rank,(unsigned)e->pc, e->dur ")
    for param in fdict[funct].paramConciseList:
	if fdict[funct].paramDict[param].recordIt:
	    olist.append(",e->opt.mpi."+funct+"."+param)
    for f in fdict[funct].extrafieldsList:
	olist.append(",e->opt.mpi."+funct+"."+ f)
    olist.append(");\n")
    olist.append("return;\n")
    olist.append("}\n")
    olist.append("\n")


###
### For all the functions, generate a print statement
###
def GeneratePrintStmtsFile():
    global flist
    global fdict

    print "-----*----- Generating the event print statments"
    cwd = os.getcwd()
    os.chdir(cwd)
    cname = cwd + "/eprint.c"
    g = open(cname, "w")
    olist = StandardFileHeader(cname)
    olist.append("#include \"mpiTi.h\"\n")
    olist.append("\n")

    for funct in flist:
	CreatePrintStmt(funct,olist)

    olist.append("\n")
    olist.append("/* eof */\n")
    g.writelines(olist)
    g.close()

    cname = cwd + "/eprint.h"
    g = open(cname, "w")
    olist = StandardFileHeader(cname)
    olist.append("#include \"mpiTi.h\"\n")
    olist.append("\n")

    for funct in flist:
	olist.append("extern void mpiTi_eprint_"+funct+"(mpiTi_event_t*e);\n")

    olist.append("\n")
    olist.append("/* eof */\n")
    g.writelines(olist)
    g.close()


###
### Generate a lookup table where mpiT can grab variables and function pointers.
###
def GenerateLookup():
    global flist
    global fdict

    print "-----*----- Generating the lookup table"
    cwd = os.getcwd()
    os.chdir(cwd)
    sname = cwd + "/lookup.c"
    g = open(sname, "w")
    olist = StandardFileHeader(sname)
    ##### -jsv 8/14
#    olist.append("#include \"mpiTi.h\"\n")
#    olist.append("#include \"eprint.h\"\n")
    olist.append("#include \"mpiPi.h\"\n")
    olist.append("#include \"mpiPi_def.h\"\n")
    olist.append("\n")
    olist.append("\n")

    olist.append("mpiPi_lookup_t mpiPi_lookup [] = {\n")

    counter = 0
    for funct in flist:
        if funct not in noDefineList:
	  if counter < len(flist) \
	    and counter > 0 :
	    olist.append(",\n")
	  olist.append("\t{ mpiPi_" + funct)
	  olist.append(", \"" + funct + "\"")
	  olist.append("}")
	  counter = counter + 1

    olist.append(",\n\t{0,NULL}};\n")

    olist.append("\n")
    olist.append("/* eof */\n")
    g.writelines(olist)
    g.close()


###
### Create a MPI wrapper for one function using the information in the function dict.
### First, generate a generic wrapper, and then the FORTRAN, C wrappers.
###
def CreateWrapper(funct, olist):
    global fdict

    if fdict[funct].nowrapper:
	return

    if verbose:
	print "Wrapping ",funct

    olist.append("\n\n\n/* --------------- " + funct + " --------------- */\n" )

    #####
    ##### Generic wrapper
    #####

    ##### funct decl
    olist.append("\nstatic int mpiPif_" + funct + "( jmp_buf * base_jbuf, " )
    # add parameters
    for i in fdict[funct].paramConciseList:
	if (fdict[funct].paramDict[i].pointerLevel == 0) \
	   and (fdict[funct].paramDict[i].arrayLevel == 0) \
	   and (fdict[funct].paramDict[i].basetype != "void"):
	    olist.append(fdict[funct].paramDict[i].basetype + " * " + i)
	elif (fdict[funct].paramDict[i].pointerLevel > 0):
	    olist.append(fdict[funct].paramDict[i].basetype)
	    for j in xrange(1,fdict[funct].paramDict[i].pointerLevel+1):
		olist.append(" *")
	    olist.append(i)
	else:
	    pass
	if fdict[funct].paramConciseList.index(i) < len(fdict[funct].paramConciseList) - 1:
	    olist.append(", ")
    olist.append(")")
    # start wrapper code
    olist.append("{")
    olist.append( "int rc, enabledState; double dur; int tsize; double messSize = 0.; double ioSize = 0.; \nmpiPi_TIME start, end;\nvoid *call_stack[MPIP_CALLSITE_STACK_DEPTH_MAX] = { NULL };\n" )

    olist.append("\nif (mpiPi.enabled) {\n")
    if fdict[funct].wrapperPreList:
	olist.extend(fdict[funct].wrapperPreList)

    # capture timer
    olist.append("mpiPi_GETTIME (&start);\n" )

    # capture HW counters
#    olist.append("if(mpiTi.hwc_enabled){ \n" \
#		 + "mpiThwc_read_and_reset(hwc);\n"  \
#		 + "}\n" )

    # capture call stack
    olist.append("mpiPi_RecordTraceBack((*base_jbuf), call_stack, MPIP_CALLSITE_STACK_DEPTH);\n"  )

    # end of enabled check
    olist.append("}\n\n")

    # call PMPI 
    olist.append("enabledState = mpiPi.enabled;\n")
    olist.append("mpiPi.enabled = 0;\n")
    olist.append("\nrc = P" + funct + "( " )

    for i in fdict[funct].paramConciseList:
	if (fdict[funct].paramDict[i].pointerLevel == 0) \
	   and (fdict[funct].paramDict[i].arrayLevel == 0) \
	   and (fdict[funct].paramDict[i].basetype != "void"):
	    olist.append(" * " + i)
	elif (fdict[funct].paramDict[i].pointerLevel > 0):
	    olist.append(i)
	else:
	    print "Warning: passing on arg",i,"in",funct
	if fdict[funct].paramConciseList.index(i) < len(fdict[funct].paramConciseList) - 1:
	    olist.append(", ")
    olist.append(");\n\n")

    olist.append("mpiPi.enabled = enabledState;\n")
    olist.append("if (mpiPi.enabled) {\n")
    olist.append("\n" \
		 + "mpiPi_GETTIME (&end);\n" \
		 + "dur = mpiPi_GETTIMEDIFF (&end, &start);\n" \
		 + "if ( dur < 0 ) mpiPi_msg_warn(\"Negative time difference : %11.9f\\n\", dur);\n" \
#		 + "{\n" \
#		 + " mpiTi_event_t e;\n" \
#		 + "e.op = mpiTi_"+funct+";\n" \
# 		 + "e.rank = mpiTi.worldRank;\n" \
# 		 + "e.pc = GetParentPC((*base_jbuf));\n" \
# 		 + "e.dur = duration;\n" \
# 		 + "if(!PROFILER_ONLY){\n"\
# 		 + "e.ts = mpiTi_GETUSECS (&mpiTi.timer, &start) - mpiTi.initTime;\n" \
		  )

#     for p in fdict[funct].paramConciseList:
# 	if fdict[funct].paramDict[p].recordIt:
# 	    olist.append("e.opt.mpi."+funct+"."+p +" = *(" + p + ");\n")

#     olist.append("}\n")

#    if fdict[funct].wrapperPostList:
#	olist.extend(fdict[funct].wrapperPostList)

#     olist.append("\nif(mpiTi.callstack_enabled){"
# 		 + "for(i=0; call_stack[i] != 0; i++) {\n" \
# 		 + "cs_e[i].op = mpiTi_MPI_Z_StackFrame;\n" \
# 		 + "cs_e[i].rank = mpiTi.worldRank;\n" \
# 		 + "cs_e[i].pc = GetParentPC((*base_jbuf));\n" \
# 		 + "cs_e[i].dur = 0;\n" \
# 		 + "cs_e[i].ts = e.ts;\n" \
# 		 + "cs_e[i].opt.mpi.MPI_Z_StackFrame.s = (long) call_stack[i];\n"\
# 		 + "cs_e[i].opt.mpi.MPI_Z_StackFrame.ht = i;\n"\
# 		 + "}\n" \
# 		 + "for (; fbuf_Insert_Block (mpiTi.eBuf, i, (void *) cs_e) != 0;)\n" \
# 		 + "{;\n" \
# 		 + " }}\n" )

#     olist.append("\n" \
# 		 + "if(mpiTi.hwc_enabled) {"
# 		 + "hwc_e.op = mpiTi_MPI_ZHW_Read;\n" \
# 		 + "hwc_e.rank = mpiTi.worldRank;\n" \
# 		 + "hwc_e.pc = GetParentPC((*base_jbuf));\n" \
# 		 + "hwc_e.dur = 0;\n" \
# 		 + "hwc_e.ts = e.ts;\n" \
# 		 + "hwc_e.opt.mpi.MPI_ZHW_Read.cyc = hwc[0];\n"\
# 		 + "hwc_e.opt.mpi.MPI_ZHW_Read.inst = hwc[1];\n"\
# 		 + "hwc_e.opt.mpi.MPI_ZHW_Read.lds = hwc[2];\n"\
# 		 + "hwc_e.opt.mpi.MPI_ZHW_Read.fops = hwc[3];\n"\
# 		 + "hwc_e.opt.mpi.MPI_ZHW_Read.fmas = hwc[4];\n"\
# 		 + "for (; fbuf_Insert (mpiTi.eBuf, (void *) &hwc_e) != 0;)\n" \
# 		 + "{;\n" \
# 		 + " }\n" \
# 		 + "}\n" )

#     olist.append( "\n" )

#     olist.append( "if(mpiTi.tracing_enabled){" \
# 		  + "for (; fbuf_Insert (mpiTi.eBuf, (void *) &e) != 0;)\n" \
# 		  + "{;\n" \
# 		  + " }\n" \
# 		  + " }\n" 
# 		  )


#     olist.append( "}\n" \
# 		  + "mpiTi_flush_if_nec();\n" \
# 		  + "if(mpiTi.hwc_enabled) { mpiThwc_reset(); }\n" \
# 		  + "return rc;\n" )
    #
    if fdict[funct].sendCountPname != "":
        olist.append( "\n" 
                      + "PMPI_Type_size(*" + fdict[funct].sendTypePname + ", " 
                      + "&tsize);\n" 
                      + "messSize = (double)(tsize * *"
                      +  fdict[funct].sendCountPname + ");\n")
                  
    if fdict[funct].ioCountPname != "":
        olist.append( "\n" 
                      + "PMPI_Type_size(*" + fdict[funct].ioTypePname + ", " 
                      + "&tsize);\n" 
                      + "ioSize = (double)(tsize * *"
                      +  fdict[funct].ioCountPname + ");\n")
    
    olist.append( "\n" \
		  + "mpiPi_update_callsite_stats(" + "mpiPi_" + funct + ", " \
                  + "mpiPi.rank, "
                  + "call_stack, "
                  + "dur, "
                  + "(double)messSize,"
                  + "(double)ioSize"
                  + ");\n" )

    # end of enabled check
    olist.append("}\n\n")

    olist.append("return rc;\n")

    olist.append("}" + " /* " + funct + " */\n\n")


    decl =    "\n" \
	   + "int rc;\n" \
	   + "jmp_buf jbuf;\n"
    
#	   + "double dur;\n" \
#	   + "mpiPi_TIME start, end;\n" \
#	   + "void * call_stack[MPIP_CALLSITE_STACK_DEPTH] = {NULL};" 
#	   + "int i;\n" \
#	   + "long long hwc[16] = {0};\n"  \
#	   + "mpiTi_event_t cs_e[64];\n" \
#	   + "mpiTi_event_t hwc_e;\n" \

    #####
    ##### C wrapper
    #####
    olist.append("\n\nextern " + fdict[funct].protoline + "{" )
    olist.append(decl)
    if fdict[funct].wrapperPreList:
	olist.extend(fdict[funct].wrapperPreList)
    olist.append("setjmp (jbuf);\n")
    olist.append("\nrc = mpiPif_" + funct + "( &jbuf, " )
    for i in fdict[funct].paramConciseList:
	if (fdict[funct].paramDict[i].pointerLevel == 0) \
	   and (fdict[funct].paramDict[i].arrayLevel == 0) \
	   and (fdict[funct].paramDict[i].basetype != "void"):
	    olist.append(" & " + i)
	elif (fdict[funct].paramDict[i].pointerLevel > 0):
	    olist.append(i)
	else:
	    pass
	if fdict[funct].paramConciseList.index(i) < len(fdict[funct].paramConciseList) - 1:
	    olist.append(", ")

    olist.append(" );\n\n" + "return rc;\n" )
    olist.append("}" + " /* " + funct + " */\n")


    #####
    ##### Fortran wrapper
    #####

    ##### funct decl
    olist.append("\n\nextern void " + "F77_" + string.upper(funct) + "(" )
    
    ###  Type translation information
    xlateVarName = ""
    xlateVarNames = []
    xlateTypes = []
    xlateCount = 0
    #  Input types to translate
    xlateTargetTypes = [ "MPI_Comm", "MPI_Group", "MPI_Info", "MPI_Op", "MPI_Request" ]
        
    freelist = []
    
    for i in fdict[funct].paramConciseList:
        # In the case where MPI_Fint and MPI_Request are not the same size,
        #   we want to use MPI conversion functions.
        # The most obvious problem we have encountered is for MPI_Request objects,
        #   but Communicators, Group, Win, Info, Op are also possibily problems.
        # There are two cases:
        #   1) A single argument needs to be translated.
        #   2) An array of objects needs to be allocated and translated.
        #      This only appears necessary for Request and Datatype
        if ( doOpaqueXlate is True and fdict[funct].paramDict[i].basetype in xlateTargetTypes ) :
            currBasetype = "MPI_Fint"
            xlateTypes.append(fdict[funct].paramDict[i].basetype)
            xlateVarNames.append(i)
            #  Try to identify whether array or single value
            if ( xlateVarNames[xlateCount].count("array") > 0 ):
                decl += xlateTypes[xlateCount] + " *c_" + xlateVarNames[xlateCount] + ";\n";
            else:
                decl += xlateTypes[xlateCount] + " c_" + xlateVarNames[xlateCount] + ";\n";
            xlateCount += 1
        else:
    		currBasetype = fdict[funct].paramDict[i].basetype
    	if (fdict[funct].paramDict[i].pointerLevel == 0) \
    	   and (fdict[funct].paramDict[i].arrayLevel == 0) \
    	   and (fdict[funct].paramDict[i].basetype != "void"):
    	    olist.append(currBasetype + " * " + i)
    	elif (fdict[funct].paramDict[i].pointerLevel > 0):
    	    olist.append(currBasetype)
    	    for j in xrange(1,fdict[funct].paramDict[i].pointerLevel+1):
    		olist.append(" *")
    	    olist.append(i)
    	else:
    	    pass
    	if fdict[funct].paramConciseList.index(i) < len(fdict[funct].paramConciseList) - 1:
    	    olist.append(", ")
    olist.append(" , int *ierr)")
    olist.append("{")
    olist.append(decl)
    olist.append("\n")
    
    if fdict[funct].wrapperPreList:
	    olist.extend(fdict[funct].wrapperPreList)

    olist.append("setjmp (jbuf);\n\n")
    
    #  Generate pre-call translation code if necessary
    for i in range(len(xlateVarNames)) :
        xlateVarName = xlateVarNames[i]
        xlateType = xlateTypes[i]
        if ( xlateVarName.count("array") > 0 ):
            olist.append("c_" + xlateVarName + " = (" + xlateType + "*)malloc(sizeof(" + xlateType + ")*(*count));\n")
            olist.append("{\n  int i; \n")
            olist.append("  for (i = 0; i < *count; i++) { \n")
            olist.append("    c_" + xlateVarName + "[i] = " + xlateType + "_f2c(" + xlateVarName + "[i]);\n")
            olist.append("  }\n}\n")
            freelist.append("c_"+xlateVarName);
        else:
            olist.append("c_" + xlateVarName + " = " + xlateType + "_f2c(*" + xlateVarName + ");\n")
            
    olist.append("\nrc = mpiPif_" + funct + "( &jbuf, " )
    
    argname = ""
    for i in fdict[funct].paramConciseList:
        #  Replace argument with the translated variable
        if ( i in xlateVarNames ):
            xlateVarName = i
            if ( xlateVarName.count("array") > 0 ):
                argname = "c_" + xlateVarName;
            else:
                argname = "&c_" + xlateVarName;
        else:
            argname = i
        if (fdict[funct].paramDict[i].pointerLevel == 0) \
           and (fdict[funct].paramDict[i].arrayLevel == 0) \
           and (fdict[funct].paramDict[i].basetype != "void"):
            olist.append(argname)
        elif (fdict[funct].paramDict[i].pointerLevel > 0):
            olist.append(argname)
        else:
            pass
        if fdict[funct].paramConciseList.index(i) < len(fdict[funct].paramConciseList) - 1:
            olist.append(", ")

    olist.append(" );\n\n")
    olist.append("*ierr = rc;\n")

    #  Generate post-call translation code if necessary
    for i in range(len(xlateVarNames)) :
        xlateVarName = xlateVarNames[i]
        xlateType = xlateTypes[i]
        if ( xlateVarName.count("array") > 0 ):
            olist.append("{\n  int i; \n")
            olist.append("  for (i = 0; i < *count; i++) { \n")
            olist.append("    " + xlateVarName + "[i] = " + xlateType + "_c2f(c_" + xlateVarName + "[i]);\n")
            olist.append("  }\n}\n")
        else:
            olist.append("*" + xlateVarName + " = " + xlateType + "_c2f(c_" + xlateVarName + ");\n")
    for freeSym in freelist:
        olist.append("free("+freeSym+");\n")
                
#    #  Generate post-call translation code if necessary and free any allocated memory
#    if ( doOpaqueXlate is True ) :
#        if ( funct in xlateExitInfo ) :
#            olist.append("*" + xlateExitInfo[funct].varName + " = MPI_" + xlateExitInfo[funct].mpiType + "_c2f(c_" + xlateExitInfo[funct].varName + ");\n")
#        for freeSym in freelist:
#            olist.append("free("+freeSym+");\n")

    olist.append("return;\n" + "}" + " /* " + string.lower(funct) + " */\n")

    print "   Wrapped " + funct


def GenerateWrappers():
    global flist
    global fdict

    print "-----*----- Generating profiling wrappers"
    cwd = os.getcwd()
    os.chdir(cwd)
    sname = cwd + "/wrappers.c"
    g = open(sname, "w")
    olist = StandardFileHeader(sname)
    olist.append("#include \"mpiPi.h\"\n")
    olist.append("#include \"symbols.h\"\n")
    olist.append("#include \"mpiPi_def.h\"\n")
    olist.append("\n")
    for funct in flist:
	CreateWrapper(funct, olist)
    olist.append("\n")
    olist.append("\n")
    olist.append("/* eof */\n")
    g.writelines(olist)
    g.close()

def GenerateSymbolDefs():
    global flist
    global f77symbol

    cwd = os.getcwd()
    os.chdir(cwd)
    sname = cwd + "/symbols.h"

    symflist = copy.deepcopy(flist)
    symflist.append('mpipi_get_fortran_argc')
    symflist.append('mpipi_get_fortran_arg')

    g = open(sname, "w")
    for funct in symflist:
        if f77symbol == 'symbol':
            f77funct = string.lower(funct)
        elif f77symbol == 'symbol_':
            f77funct = string.lower(funct) + "_"
        elif f77symbol == 'symbol__':
            f77funct = string.lower(funct) + "__"
        elif f77symbol == 'SYMBOL':
            f77funct = string.upper(funct)
        elif f77symbol == 'SYMBOL_':
            f77funct = string.upper(funct) + "_"
        elif f77symbol == 'SYMBOL__':
            f77funct = string.upper(funct) + "__"
        else:
            f77funct = string.lower(funct)

        g.write("#define F77_" + string.upper(funct) + " " + f77funct + "\n")

    g.close()

def main():
    global fdict
    global flist
    global f77symbol
    global doOpaqueXlate

    opts, pargs = getopt.getopt(sys.argv[1:], '', ['f77symbol=', 'xlate'])

    print "MPI Wrapper Generator ($Revision$)"
    #print "opts=",opts
    #print "pargs=",pargs

    f77symbol = 'symbol'
    doOpaqueXlate = False
    
    for o, a in opts:
        if o == '--f77symbol':
            f77symbol = a
        if o == '--xlate':
            doOpaqueXlate = True
            

    ##### Load the input file
    if len(pargs) < 1:
	f = sys.__stdin__
    else:
	f = open(pargs[0])
    ReadInputFile(f)

    print "-----*----- Beginning parameter optimization"
    #ParameterOptimization()

    GenerateStructureFile()
    GenerateWrappers()
    GenerateSymbolDefs()
    GenerateLookup()
##### -jsv 8/14/01
#   GeneratePrintStmtsFile()

#####
##### Call main
#####
main()

# EOF
