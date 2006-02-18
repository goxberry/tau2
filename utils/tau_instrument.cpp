/****************************************************************************
**			TAU Portable Profiling Package			   **
**			http://www.cs.uoregon.edu/research/paracomp/tau    **
*****************************************************************************
**    Copyright 2005  						   	   **
**    Department of Computer and Information Science, University of Oregon **
**    Advanced Computing Laboratory, Los Alamos National Laboratory        **
****************************************************************************/
/***************************************************************************
**	File 		: tau_instrument.cpp				  **
**	Description 	: Provides selective instrumentation support in   **
**                        TAU.                                            **
**	Author		: Sameer Shende					  **
**	Contact		: sameer@cs.uoregon.edu sameer@paratools.com      **
**	Documentation	:                                                 **
***************************************************************************/

/* Headers */
#include <string> 
#include <iostream>
using namespace std;
#include "tau_instrument.h"
#ifdef _OLD_HEADER_
# include <fstream.h>
# include <set.h>
# include <algo.h>
#else
# include <fstream>
  using std::ifstream;
    using std::ofstream;
# include <set>
      using std::set;
# include <algorithm>
        using std::sort;
	  using std::unique;
#endif
#include "pdbAll.h"

extern bool wildcardCompare(char *wild, char *string, char kleenestar);

/* Globals */
///////////////////////////////////////////////////////////////////////////
vector<tauInstrument *> instrumentList; 
///////////////////////////////////////////////////////////////////////////

/* Constructors */
///////////////////////////////////////////////////////////////////////////
// tauInstrument() ctor which sets all the items
///////////////////////////////////////////////////////////////////////////
tauInstrument::tauInstrument(string f, string r, int l, string c, 
	instrumentKind_t k): filename(f), fileSpecified(true), routineName(r),
	lineno(l), lineSpecified (true), code(c), codeSpecified(true), kind (k) 
{}


///////////////////////////////////////////////////////////////////////////
// tauInstrument() ctor
// file = "foo.cpp" line=245 code = "TAU_NODE(0);" 
///////////////////////////////////////////////////////////////////////////
tauInstrument::tauInstrument(string f, int l, string c, instrumentKind_t k) : 
      	filename(f), fileSpecified(true), lineno(l), lineSpecified(true), 
      	routineSpecified(false), code(c), codeSpecified(true), kind(k) 
{}

///////////////////////////////////////////////////////////////////////////
// tauInstrument() ctor
// entry routine="foo" code="print *, 'Hi'; " */
//    /* FOR THIS TYPE, you must specify the codeSpecified argument */
///////////////////////////////////////////////////////////////////////////
tauInstrument::tauInstrument(string r, string c, bool cs, instrumentKind_t k) :
      	routineName(r), routineSpecified(true), code(c), codeSpecified(cs), 
  	kind(k), fileSpecified(false), lineSpecified(false)
{}

///////////////////////////////////////////////////////////////////////////
// tauInstrument() ctor
//    /* entry/exit file = "foo.f90" routine = "foo" code = "printf" */
///////////////////////////////////////////////////////////////////////////
tauInstrument::tauInstrument(string f, string r, string c, instrumentKind_t k) 
	: filename(f), fileSpecified(true), routineName(r), 
	routineSpecified(true), code (c), codeSpecified(true), 
	lineSpecified(false), kind (k) 
{} 

///////////////////////////////////////////////////////////////////////////
// tauInstrument() ctor
// loops routine = "foo" 
///////////////////////////////////////////////////////////////////////////
tauInstrument::tauInstrument(string r, instrumentKind_t k ) : 
	routineName(r), routineSpecified (true), kind (k), 
	lineSpecified(false), fileSpecified(false), codeSpecified(false) 
{} 

///////////////////////////////////////////////////////////////////////////
// tauInstrument() ctor
// loops file = "f1.cpp" routine = "foo" 
///////////////////////////////////////////////////////////////////////////
tauInstrument::tauInstrument(string f, string r, instrumentKind_t k ) : 
	filename (f), fileSpecified(true), routineName(r), 
	routineSpecified (true), kind (k), lineSpecified(false), 
	codeSpecified(false) 
{} 

///////////////////////////////////////////////////////////////////////////
// tauInstrument() dtor
///////////////////////////////////////////////////////////////////////////
tauInstrument::~tauInstrument() { }

///////////////////////////////////////////////////////////////////////////
// tauInstrument::print writes the contents of tauInstrument to ostr
///////////////////////////////////////////////////////////////////////////
ostream& tauInstrument::print(ostream& ostr) const 
{
       if (fileSpecified) ostr << "filename: "<<filename<< " ";
       if (routineSpecified) ostr <<"routine: "<<routineName<< " " ;
       if (lineSpecified) ostr<<"line no: "<<lineno<< "  " ;
       if (codeSpecified) ostr<<"code: "<<code<<" " ;
       switch (kind) 
       {
	 case TAU_LOOPS:
		 ostr<<"loops: ";
		 break;
	 case TAU_LINE:
		 ostr<<"line:";
		 break;
	 case TAU_ROUTINE_ENTRY:
		 ostr<<"entry: ";
		 break;
	 case TAU_ROUTINE_EXIT:
		 ostr<<"exit: ";
		 break;
	 case TAU_NOT_SPECIFIED:
		 ostr<<"ERROR: NOT SPECIFIED KIND";
		 break;
	 default:
		 ostr<<"default: ???";
		 break;
       }
       ostr<<endl;
       return ostr;
}
    
///////////////////////////////////////////////////////////////////////////
// tauInstrument::getFileSpecified() accesses private data member
///////////////////////////////////////////////////////////////////////////
bool 	tauInstrument::getFileSpecified(void) { return fileSpecified; }

///////////////////////////////////////////////////////////////////////////
// tauInstrument::getFileName() accesses private data member
///////////////////////////////////////////////////////////////////////////
string& tauInstrument::getFileName(void) { return filename; } 

///////////////////////////////////////////////////////////////////////////
// tauInstrument::getRoutineSpecified() accesses private data member
///////////////////////////////////////////////////////////////////////////
bool 	tauInstrument::getRoutineSpecified(void) { return routineSpecified; }

///////////////////////////////////////////////////////////////////////////
// tauInstrument::getRoutineName() accesses private data member
///////////////////////////////////////////////////////////////////////////
string& tauInstrument::getRoutineName(void) { return routineName; } 

///////////////////////////////////////////////////////////////////////////
// tauInstrument::getLineSpecified() accesses private data member
///////////////////////////////////////////////////////////////////////////
bool 	tauInstrument::getLineSpecified(void) { return lineSpecified; }

///////////////////////////////////////////////////////////////////////////
// tauInstrument::getLineNo() accesses private data member
///////////////////////////////////////////////////////////////////////////
int 	tauInstrument::getLineNo(void) { return lineno; }

///////////////////////////////////////////////////////////////////////////
// tauInstrument::getCodeSpecified() accesses private data member
///////////////////////////////////////////////////////////////////////////
bool 	tauInstrument::getCodeSpecified(void) { return codeSpecified; }

///////////////////////////////////////////////////////////////////////////
// tauInstrument::getCode() accesses private data member
///////////////////////////////////////////////////////////////////////////
string& tauInstrument::getCode(void) { return code; } 

///////////////////////////////////////////////////////////////////////////
// tauInstrument::getKind() accesses private data member
///////////////////////////////////////////////////////////////////////////
instrumentKind_t tauInstrument::getKind(void) { return kind; }


/* Instrumentation section */

///////////////////////////////////////////////////////////////////////////
// parseError
// input: line number and column 
///////////////////////////////////////////////////////////////////////////
void parseError(char *message, char *line, int lineno, int column)
{
  printf("ERROR: %s: parse error at selective instrumentation file line %d col %d\n",
	message, lineno, column);
  printf("line=%s\n", line);
  exit(0);
}

#define WSPACE(line) while ( line[0] == ' ' || line[0] == '\t')  \
    { \
      if (line[0] == '\0') parseError("EOL found", line, lineno, line - original);  \
      line++;  \
    } 

#define TOKEN(k) if (line[0] != k || line[0] == '\0') parseError("token not found", line, lineno, (int ) (line - original)); \
		 else line++; 

#define RETRIEVESTRING(pname, line) i = 0; \
  while (line[0] != '"') { \
  if (line [0] == '\0') parseError("EOL", line, lineno, line - original); \
    pname[i++] = line[0]; line++; \
  } \
  pname[i] = '\0';  \
  line++; /* found closing " */

#define RETRIEVECODE(pname, line) i = 0; \
  while (line[0] != '"') { \
    if (line [0] == '\0') parseError("EOL", line, lineno, line - original); \
    if ((line[0] == '\\') && (line[1] == '"')) line++; \
    pname[i++] = line[0]; line++; \
  } \
  pname[i] = '\0';  \
  line++; /* found closing " */

#define RETRIEVENUMBER(pname, line) i = 0; \
  while (line[0] != ' ' && line[0] != '\t' ) { \
  if (line [0] == '\0') parseError("EOL", line, lineno, line - original); \
    pname[i++] = line[0]; line++; \
  } \
  pname[i] = '\0';  \
  line++; /* found closing " */

///////////////////////////////////////////////////////////////////////////
// parseInstrumentationCommand
// input: line -  character string containing a line of text from the selective 
// instrumentation file 
// input: lineno - integer line no. (for reporting parse errors if any)
//
///////////////////////////////////////////////////////////////////////////
void parseInstrumentationCommand(char *line, int lineno)
{
  char  *original;
  int i, ret, value; 
  bool filespecified = false; 
  char pname[INBUF_SIZE]; /* parsed name */
  char pfile[INBUF_SIZE]; /* parsed filename */
  char plineno[INBUF_SIZE]; /* parsed lineno */
  char pcode[INBUF_SIZE]; /* parsed code */

#ifdef DEBUG
  printf("Inside parseInstrumentationCommand: line %s lineno: %d\n",
		  line, lineno);
#endif /* DEBUG */

  original = line; 
  /* check the initial keyword */
  if (strncmp(line, "file", 4) == 0)
  { 
    /* parse: file = "foo.cc" line = 245 code = "TAU_NODE(0);" */
    line+=4; /* start checking from here */
    /* WHITE SPACES */
    WSPACE(line);
    TOKEN('=');
    WSPACE(line);
    TOKEN('"');
    RETRIEVESTRING(pfile, line);
    filespecified = true; 
#ifdef DEBUG
    printf("Got name = %s\n", pfile);
#endif 

    WSPACE(line); /* space  */
    if (strncmp(line, "line", 4) == 0)
    { /* got line token, get line no. */
      line += 4; 
      WSPACE(line);
      TOKEN('=');
      WSPACE(line);
      RETRIEVENUMBER(plineno, line);
      ret = sscanf(plineno, "%d", &value); 
#ifdef DEBUG
      printf("got line no = %d\n", value);
#endif /* DEBUG */
    }
    else parseError("<line> token not found", line, lineno, line - original);
    WSPACE(line); 
    /* go to code */
    if (strncmp(line, "code", 4) == 0)
    { 
      line+= 4; /* move 4 spaces */
      /* check for = <WSPACE> " */
      WSPACE(line);
      TOKEN('=');
      WSPACE(line);
      TOKEN('"');
      RETRIEVECODE(pcode, line);
#ifdef DEBUG
      printf("Got code = %s\n", pcode);
#endif /* DEBUG */
    }
    else parseError("<code> token not found", line, lineno, line - original); 
#ifdef DEBUG
    printf("file=%s, code = %s, line no = %d\n", pfile, pcode, value);
#endif /* DEBUG */
    instrumentList.push_back(new tauInstrument(string(pfile), value, string(pcode), TAU_LINE)); 
    
  }
  else
  { /* parse: entry routine="foo()", code = "TAU_SET_NODE(0)" */
    if (strncmp(line, "entry", 5) == 0)
    {
      line+=5; 
      WSPACE(line);
      if (strncmp(line, "file", 4) == 0)
      {
	line+= 4;
	WSPACE(line);
	TOKEN('=');
	WSPACE(line);
	TOKEN('"');
	RETRIEVESTRING(pfile, line);
	WSPACE(line);
	filespecified = true; 
#ifdef DEBUG
	printf("GOT FILE = %s\n", pfile);
#endif /* DEBUG */
      }
      if (strncmp(line, "routine", 7) == 0)
      {
	line+=7; 
	WSPACE(line);
	TOKEN('=');
	WSPACE(line);
	TOKEN('"');
	RETRIEVESTRING(pname, line);
	WSPACE(line);
        if (strncmp(line, "code", 4) == 0)
        { 
          line+= 4; /* move 4 spaces */
          /* check for = <WSPACE> " */
          WSPACE(line);
          TOKEN('=');
          WSPACE(line);
          TOKEN('"');
          RETRIEVECODE(pcode, line);
#ifdef DEBUG
          printf("Got code = %s\n", pcode);
#endif /* DEBUG */
	}
        else parseError("<code> token not found", line, lineno, line - original); 
#ifdef DEBUG 
	printf("Got entry routine = %s code =%s\n", pname, pcode);
#endif /* DEBUG */
        if (filespecified)
	{
          instrumentList.push_back(new tauInstrument(string(pfile), string(pname), string(pcode), TAU_ROUTINE_ENTRY)); 
	}
        else 
	{
	  bool codespecified = true; 
          instrumentList.push_back(new tauInstrument(string(pname), string(pcode), codespecified, TAU_ROUTINE_ENTRY)); 
	} /* file and routine are both specified for entry */
      }
      else parseError("<routine> token not found", line, lineno, line - original);
    } /* end of entry token */
    else 
    { /* parse exit, and loops */
      if (strncmp(line, "exit", 4) == 0)
      {
        line+=4; 
        WSPACE(line);
        if (strncmp(line, "file", 4) == 0)
        {
     	  line+= 4;
	  WSPACE(line);
	  TOKEN('=');
	  WSPACE(line);
	  TOKEN('"');
	  RETRIEVESTRING(pfile, line);
	  WSPACE(line);
	  filespecified = true; 
#ifdef DEBUG
	  printf("GOT FILE = %s\n", pfile);
#endif /* DEBUG */
	}
        if (strncmp(line, "routine", 7) == 0)
        {
          line+=7; 
	  WSPACE(line);
	  TOKEN('=');
	  WSPACE(line);
	  TOKEN('"');
	  RETRIEVESTRING(pname, line);
	  WSPACE(line);
          if (strncmp(line, "code", 4) == 0)
          { 
            line+= 4; /* move 4 spaces */
            /* check for = <WSPACE> " */
            WSPACE(line);
            TOKEN('=');
            WSPACE(line);
            TOKEN('"');
            RETRIEVECODE(pcode, line);
#ifdef DEBUG
            printf("Got code = %s\n", pcode);
#endif /* DEBUG */
            if (filespecified)
	    {
              instrumentList.push_back(new tauInstrument(string(pfile), string(pname), string(pcode), TAU_ROUTINE_EXIT)); 
	    }
            else 
	    {
	      bool codespecified = true; 
              instrumentList.push_back(new tauInstrument(string(pname), string(pcode), codespecified, TAU_ROUTINE_EXIT)); 
	    } /* file and routine are both specified for entry */
	  }
	  else parseError("<code> token not found", line, lineno, line - original);
	}
	else parseError("<routine> token not found", line, lineno, line - original);
#ifdef DEBUG
	printf("exit routine = %s code = %s\n", pname, pcode);
#endif /* DEBUG */
      } /* end of exit */
      else 
      { /* loops */
        if (strncmp(line, "loops", 5) == 0)
	{
	  line+= 5; /* move 5 spaces */
	  /* check for WSPACE */
	  WSPACE(line);
          if (strncmp(line, "file", 4) == 0)
          {
     	    line+= 4;
	    WSPACE(line);
	    TOKEN('=');
	    WSPACE(line);
	    TOKEN('"');
	    RETRIEVESTRING(pfile, line);
	    WSPACE(line);
	    filespecified = true; 
#ifdef DEBUG
	    printf("GOT FILE = %s\n", pfile);
#endif /* DEBUG */
	  }
	  if (strncmp(line, "routine", 7) == 0)
	  {
	    line+=7;
	    /* found routine */ 
	    WSPACE(line);
	    TOKEN('=');
	    WSPACE(line);
	    TOKEN('"');
	    RETRIEVESTRING(pname, line);
#ifdef DEBUG
	    printf("got loops routine = %s\n", pname);
#endif /* DEBUG */
	    if (filespecified)
	    {
	      instrumentList.push_back(new tauInstrument(string(pfile), string(pname), TAU_LOOPS));
	    }
	    else
	    {
	      instrumentList.push_back(new tauInstrument(string(pname), TAU_LOOPS));
	    }
	  }
	  else parseError("<routine> token not found", line, lineno, line - original);
	}
      } /* end of loops directive */
    }
  }


}

///////////////////////////////////////////////////////////////////////////
// isInstrumentListEmpty() returns true if there are no entries in 
// instrumentList
///////////////////////////////////////////////////////////////////////////

bool isInstrumentListEmpty(void)
{
  return instrumentList.empty();
}

///////////////////////////////////////////////////////////////////////////
// printInstrumentList() lists all entries in instrumentList
///////////////////////////////////////////////////////////////////////////


void printInstrumentList(void)
{
  char orig[INBUF_SIZE];
  vector<tauInstrument *>::iterator it;
#ifdef DEBUG
  if (!isInstrumentListEmpty())
  { /* the list is not empty! */ 
    for (it = instrumentList.begin(); it != instrumentList.end(); it++) {
      (*it)->print(cout);
      /*
      if ((*it)->getCodeSpecified()) {
	cout <<(*it)->getCode();
  	strcpy(orig, "ppp\nkkk");
  	string mystr(orig);
  	cout<<mystr <<endl;
	cout <<orig <<endl;
      }
      */
    }
  }
#endif /* DEBUG */

}
/* Add request for instrumentation for C/C++ loops */
void addRequestForLoopInstrumentation(const pdbRoutine *ro, const pdbLoc& start, const pdbLoc& stop, vector<itemRef *>& itemvec)
{
  const pdbFile *f = start.file();
  char lines[256];
  sprintf(lines, "line,col = <%d,%d> to <%d,%d>", 
	start.line(), start.col(), stop.line(), stop.col());
  string *timername = new string(string("Loop: " + ro->fullName() + "[ file = <"+f->name()+ "> "+ lines + " ]"));
#ifdef DEBUG
  printf("Adding instrumentation at %s\n", timername->c_str());
#endif /* DEBUG */
  string startsnippet(string("{ TAU_PROFILE_TIMER(lt, \"")+(*timername)+"\", \" \", TAU_USER); TAU_PROFILE_START(lt); ");
  string stopsnippet(string("TAU_PROFILE_STOP(lt); } "));
  itemvec.push_back( new itemRef((const pdbItem *)ro, INSTRUMENTATION_POINT, start.line(), start.col(), startsnippet, BEFORE));
  itemvec.push_back( new itemRef((const pdbItem *)ro, INSTRUMENTATION_POINT, stop.line(), stop.col()+1, stopsnippet, AFTER));
/*
  printf("Adding instrumentation at routine %s, file %s, start %d:%d, stop %d:%d\n",
    ro->fullName().c_str(), f->name(), start.line(), start.col(), stop.line(), stop.col());
*/
}

/* Process Block to examine the routine */
int processBlock(const pdbStmt *s, pdbRoutine *ro, vector<itemRef *>& itemvec)
{
  pdbLoc start, stop; /* the location of start and stop timer statements */
  
  if (!s) return 1;
  if (!ro) return 1; /* if null, do not instrument */

  
  pdbStmt::stmt_t k = s->kind();

  if (k == pdbStmt::ST_BLOCK)
  {
#ifdef DEBUG
    printf("Going down the block, routine = %s\n", ro->fullName().c_str());
#endif /* DEBUG */
    processBlock(s->downStmt(), ro, itemvec);
  }
  else
  {
#ifdef DEBUG
    printf("Examining statement \n");
#endif /* DEBUG */
    switch(k) {
      case pdbStmt::ST_FOR:
      case pdbStmt::ST_WHILE:
      case pdbStmt::ST_DO:
#ifdef DEBUG
        printf("loop statement:\n");
#endif /* DEBUG */
        start = s->stmtBegin();
        stop = s->stmtEnd();
#ifdef DEBUG
        printf("start=<%d:%d> - end=<%d:%d>\n",
          start.line(), start.col(), stop.line(), stop.col());
#endif /* DEBUG */
	addRequestForLoopInstrumentation(ro, start, stop, itemvec); 
        break;
      case pdbStmt::ST_DECL:
#ifdef DEBUG
        printf("Decl statement:\n"); 
#endif /* DEBUG */
	break;
      case pdbStmt::ST_ASSIGN:
#ifdef DEBUG
        printf("Assign statement:\n"); 
#endif /* DEBUG */
	break;
      default:
#ifdef DEBUG
        printf("Other statement\n"); 
#endif /* DEBUG */
	break;
    }
  } /* and then process the next statement */
  return processBlock(s->nextStmt(), ro, itemvec);

}

/* Process list of C routines */
int processCRoutinesInstrumentation(PDB & p, vector<tauInstrument *>::iterator& it, vector<itemRef *>& itemvec) 
{
  /* compare the names of routines with our instrumentation request routine name */

  PDB::croutinevec croutines = p.getCRoutineVec();
  bool cmpResult1, cmpResult2; 
  pdbRoutine::locvec::iterator rlit;
  for(PDB::croutinevec::const_iterator rit = croutines.begin(); 
	rit != croutines.end(); ++rit)
  { /* iterate over all routines */
    /* the first argument contains wildcard, the second is the string */
    cmpResult1 = wildcardCompare((char *)((*it)->getRoutineName()).c_str(),
	(char *)(*rit)->name().c_str(), '#');
    /* the first argument contains wildcard, the second is the string */
    cmpResult2 = wildcardCompare((char *)((*it)->getRoutineName()).c_str(),
	(char *)(*rit)->fullName().c_str(), '#');
    if (cmpResult1 || cmpResult2)
    { /* there is a match */
#ifdef DEBUG
      cout <<"Examining Routine "<<(*rit)->fullName()<<" and "<<(*it)->getRoutineName()<<endl;
#endif /* DEBUG */
      /* examine the type of request - entry/exit */
      if ((*it)->getKind() == TAU_ROUTINE_ENTRY)
      {
#ifdef DEBUG
        cout <<"Instrumenting entry of routine "<<(*rit)->fullName()<<endl;
        /* get routine entry line no. */
        cout <<"at line: "<<(*rit)->bodyBegin().line()<<", col"<< (*rit)->bodyBegin().col()<<"code = "<<(*it)->getCode()<<endl;
#endif /* DEBUG */
	itemvec.push_back( new itemRef((pdbItem *)NULL, INSTRUMENTATION_POINT, (*rit)->bodyBegin().line(), (*rit)->bodyBegin().col()+1, (*it)->getCode(), BEFORE));
	/* should the column be 1 greater than body begin's col, so 
	   instrumentation is placed after the beginning of the routine? */

      }
      if ((*it)->getKind() == TAU_ROUTINE_EXIT)
      {
#ifdef DEBUG
        cout <<"Instrumenting exit of routine "<<(*rit)->fullName()<<endl;
#endif /* DEBUG */
        /* get routine entry line no. */
        pdbRoutine::locvec retlocations = (*rit)->returnLocations();
        for (rlit = retlocations.begin(); rlit != retlocations.end(); ++rlit)
        {
#ifdef DEBUG
          cout <<"at line: "<<(*rlit)->line()<<", col"<< (*rlit)->col()<<" code = "<<(*it)->getCode()<<endl;
#endif /* DEBUG */
	
	  itemvec.push_back( new itemRef((pdbItem *)NULL, INSTRUMENTATION_POINT, (*rlit)->line(), (*rlit)->col(), (*it)->getCode(), BEFORE));
        }
      } /* end of routine exit */
      if ((*it)->getKind() == TAU_LOOPS)
      { /* we need to instrument all outer loops in this routine */
	processBlock((*rit)->body(), (*rit), itemvec);
      }
    }
  }

  return 0;
}
/* Process list of F routines */
int processFRoutinesInstrumentation(PDB & p, vector<tauInstrument *>::iterator& it, vector<itemRef *>& itemvec) 
{
  PDB::froutinevec froutines = p.getFRoutineVec();
  bool cmpResult; 
  pdbRoutine::locvec::iterator rlit;
  for(PDB::froutinevec::const_iterator rit = froutines.begin(); 
	rit != froutines.end(); ++rit)
  { /* iterate over all routines */  
    /* the first argument contains the wildcard, the second is the string */
    cmpResult = wildcardCompare((char *)((*it)->getRoutineName()).c_str(), (char *)(*rit)->name().c_str(), '#');
    if (cmpResult)
    { /* there is a match */
#ifdef DEBUG
      cout <<"Examining Routine "<<(*rit)->fullName()<<" and "<<(*it)->getRoutineName()<<endl;
#endif /* DEBUG */
      /* examine the type of request - entry/exit */
      if ((*it)->getKind() == TAU_ROUTINE_ENTRY)
      {
#ifdef DEBUG
        cout <<"Instrumenting entry of routine "<<(*rit)->fullName()<<endl;
#endif /* DEBUG */
	  itemvec.push_back( new itemRef((pdbItem *)NULL, INSTRUMENTATION_POINT,
                (*rit)->firstExecStmtLocation().line(),
                1, (*it)->getCode(), BEFORE));
                /* start from 1st column instead of column: 
		(*rit)->firstExecStmtLocation().col() */
      }
      if ((*it)->getKind() == TAU_ROUTINE_EXIT)
      {
#ifdef DEBUG
        cout <<"Instrumenting exit of routine "<<(*rit)->fullName()<<endl;
#endif /* DEBUG */
        /* get routine entry line no. */
        pdbRoutine::locvec retlocations = (*rit)->returnLocations();
        pdbRoutine::locvec stoplocations = (*rit)->stopLocations();
	
	/* we first start with the return locations */
        for (rlit = retlocations.begin(); rlit != retlocations.end(); ++rlit)
        {
#ifdef DEBUG
          cout <<"at line: "<<(*rlit)->line()<<", col"<< (*rlit)->col()<<" code = "<<(*it)->getCode()<<endl;
#endif /* DEBUG */
	
	  itemvec.push_back( new itemRef((pdbItem *)NULL, INSTRUMENTATION_POINT, (*rlit)->line(), (*rlit)->col(), (*it)->getCode(), BEFORE));
        }
	/* and then examine the stop locations */
        for (rlit = stoplocations.begin(); rlit != stoplocations.end(); ++rlit)
        {
#ifdef DEBUG
          cout <<"at line: "<<(*rlit)->line()<<", col"<< (*rlit)->col()<<" code = "<<(*it)->getCode()<<endl;
#endif /* DEBUG */
	
	  itemvec.push_back( new itemRef((pdbItem *)NULL, INSTRUMENTATION_POINT, (*rlit)->line(), (*rlit)->col(), (*it)->getCode(), BEFORE));
        }
      } /* end of routine exit */
    } /* end of match */
  } /* iterate over all routines */

  return 0;
}
int addFileInstrumentationRequests(PDB& p, pdbFile *file, vector<itemRef *>& itemvec)
{
  /* Let us iterate over the list of instrumentation requests and see if 
   * any requests match this file */
  vector<tauInstrument *>::iterator it;
  bool cmpResult; 
  int column ;
  PDB::lang_t lang;
  PDB::croutinevec croutines;
  PDB::froutinevec froutines; 

  for (it = instrumentList.begin(); it != instrumentList.end(); it++) 
  {
#ifdef DEBUG
    cout <<"Checking "<<file->name().c_str()<<" and "<<(*it)->getFileName().c_str()<<endl; 
#endif /* DEBUG */
    /* the first argument contains the wildcard, the second is the string */
    cmpResult = wildcardCompare((char *)(*it)->getFileName().c_str(), (char *)file->name().c_str(), '*');
    if (cmpResult)
    { /* check if the current file is to be instrumented */
#ifdef DEBUG
      cout <<"Matched the file names!"<<endl;
#endif /* DEBUG */
      /* Now we must add the lines for instrumentation if a line is specified! */
      if ((*it)->getLineSpecified())
      { /* Yes, a line number was specified */
#ifdef DEBUG
	cout << "Need to add line no. " <<(*it)->getLineNo()<<"column 1 to the list!"<<endl;
#endif /* DEBUG */
	/* We need to create a pdbLoc, pdbItem and add the pdbItem to the 
	 * itemvec. While creating the pdbLoc, we use the line no. of the 
	 * associated instrumentation point. We use the column no. of 1
	 * and 0 as the id of the pdbItem since it is not known. */
	
	/*
	pdbSimpleItem *item = new pdbSimpleItem((*it)->getFileName(), 0);
	item->location((const pdbLoc&) (*it)); 
	*/
	/* itemRef::itemRef(const pdbItem *i, itemKind_t k, int l, int c, string code)
	 */
	itemvec.push_back( new itemRef((pdbItem *)NULL, INSTRUMENTATION_POINT, (*it)->getLineNo(), 1, (*it)->getCode(), BEFORE));
			
      }
    }
    /* What else is specified with the instrumentation request? Are routines
       specified? Match routines with the instrumentation requests */
    /* Create a list of routines */
    if ((*it)->getRoutineSpecified())
    {
#ifdef DEBUG
      cout <<"A routine is specified! "<<endl;
#endif /* DEBUG */
      lang = p.language();
      switch (lang) { /* check the language first */
        case PDB::LA_C :
        case PDB::LA_CXX:
        case PDB::LA_C_or_CXX:
#ifdef DEBUG
	  cout <<"C routine!"<<endl; 
#endif /* DEBUG */
          processCRoutinesInstrumentation(p, it, itemvec); 
          break; 
        case PDB::LA_FORTRAN:
#ifdef DEBUG
	  cout <<"F routine!"<<endl; 
#endif /* DEBUG */
          processFRoutinesInstrumentation(p, it, itemvec); 
          break;
        default:
          break;
       }
    }

  }
  return 1;
}

   

///////////////////////////////////////////////////////////////////////////
// Generate itemvec entries for instrumentation commands 
///////////////////////////////////////////////////////////////////////////


/* EOF */


/***************************************************************************
 * $RCSfile: tau_instrument.cpp,v $   $Author: sameer $
 * $Revision: 1.3 $   $Date: 2006/02/18 04:18:41 $
 * VERSION_ID: $Id: tau_instrument.cpp,v 1.3 2006/02/18 04:18:41 sameer Exp $
 ***************************************************************************/
