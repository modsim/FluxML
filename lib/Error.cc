#include <cstdarg>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cctype>
#ifdef  __GNUG__
#include <cxxabi.h>
#endif

#include "fluxml_config.h"

extern "C" {
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <unistd.h>

#ifdef  __GNUG__
#include <execinfo.h>
#endif
}

#include "spawn_child.h"
#include "fRegEx.h"
#include "Error.h"

#define fSTRDUP(str)	strcpy(new char[strlen(str)+1],str)

static const char color_off[]     = "\033[0m";		// all attributes off
static const char color_ERROR[]   = "\033[31m";		// red
static const char color_WARNING[] = "\033[33m";		// yellow
static const char color_NOTICE[]  = "\033[32m";		// green
static const char color_INFO[]    = "\033[39m";		// (default)
static const char color_THROW[]   = "\033[35m";		// magenta
static const char color_DEBUG[]   = "\033[36m";		// cyan

LogInfo::LogInfo(
	int32_t logLevel,
	char const * fileName,
	int32_t fileLineNo,
	char const * funcName,
	char const * logMessage
	)
{
	pid_ = (int32_t)getpid();
	logLevel_ = logLevel;

	utsname U; uname(&U);
	hostName_ = fSTRDUP(U.nodename);
	hostNameLen_ = strlen(hostName_)+1;

	fileName_ = fSTRDUP(fileName);
	fileNameLen_ = strlen(fileName_)+1;
	fileLineNo_ = fileLineNo;

	funcName_ = fSTRDUP(funcName);
	funcNameLen_ = strlen(funcName)+1;

	logMessage_ = fSTRDUP(logMessage);
	logMessageLen_ = strlen(logMessage_)+1;

	timeStamp_ = time(0);
	timeStamp_nsec_ = 0;
}

LogInfo::~LogInfo()
{
	delete[] hostName_;
	delete[] fileName_;
	delete[] funcName_;
	delete[] logMessage_;
}
	
bool LogInfo::write(int fd) const
{
	bool success = true;

	success = success and (::write(fd,&logLevel_,4) == 4);
	success = success and (::write(fd,&pid_,4) == 4);
	success = success and (::write(fd,&hostNameLen_,4) == 4);
	success = success and (::write(fd,&fileNameLen_,4) == 4);
	success = success and (::write(fd,&fileLineNo_,4) == 4);
	success = success and (::write(fd,&funcNameLen_,4) == 4);
	success = success and (::write(fd,&logMessageLen_,4) == 4);
	success = success and (::write(fd,&timeStamp_,8) == 8);

	// Strings inkl. Terminator schreiben
	success = success and
		(::write(fd,hostName_,hostNameLen_) == hostNameLen_);
	success = success and
		(::write(fd,fileName_,fileNameLen_) == fileNameLen_);
	success = success and
		(::write(fd,funcName_,funcNameLen_) == funcNameLen_);
	success = success and
		(::write(fd,logMessage_,logMessageLen_) == logMessageLen_);
	return success;
}

bool LogInfo::write(FILE * stream) const
{
	return write(fileno(stream));
}


AssertionError::AssertionError(
	char const * expr,
	char const * file,
	char const * func,
	size_t line
	)
{
	size_t msg_len = ((64+strlen(expr))+strlen(func))+strlen(file);
	msg_ = new char[msg_len];
	snprintf(msg_,msg_len,"%s:%u (%s): assertion '%s' failed!",
		file, (unsigned)line, func, expr);
	file_ = fSTRDUP(file);
	func_ = fSTRDUP(func);
	expr_ = fSTRDUP(expr);
	line_ = line;
	fLOG(logERROR,file,func,line,"%s:%u: assertion '%s' failed!",
		file, (unsigned)line, expr);
#ifndef DEBUG
	backtrace(1);
#endif
}
	
AssertionError::AssertionError(AssertionError const & copy)
{
	file_ = fSTRDUP(copy.file_);
	func_ = fSTRDUP(copy.func_);
	expr_ = fSTRDUP(copy.expr_);
	line_ = copy.line_;
	msg_ = fSTRDUP(copy.msg_);
}
	
AssertionError::~AssertionError()
{
	delete[] msg_;
	delete[] file_;
	delete[] func_;
	delete[] expr_;
}

void AssertionError::attachDebugger()
{
	pid_t my_pid = getpid();
	pid_t ch_pid;
	bool cmd_file_ok = false;
	char tmpfn[255];
	FILE * tmpF;
	
	snprintf(tmpfn,254,"/tmp/assert.%010i", my_pid);
	if ((tmpF = fopen(tmpfn,"w")) != 0)
	{
		//fputs("up\nup\n",tmpF);
		fputs("up-silently 2\nframe\n",tmpF);
		fclose(tmpF);
		cmd_file_ok = true;
	}

	if ((ch_pid = fork()) == 0) // child
	{
		char buf0[2048], buf1[2048];
		int len;

		// under Linux procfs a symlink points to the executable:
		snprintf(buf1,2047,"/proc/%i/exe",my_pid);
		// follow the symlink:
		if ((len = readlink(buf1,buf0,2047)) <= 0)
		{
			fERROR("failed to locate the executable of the running process");
			_exit(EXIT_FAILURE);
		}
		buf0[len] = '\0'; // readlink() does not append the '\0'
		snprintf(buf1,2047,"%i",my_pid); // PID of the father
		// wait 1/4 sec to allow the father to leave the fork()
		usleep(250000);
		// execlp searches gdb in $PATH:
		execlp("gdb","gdb","-q","-x",tmpfn,buf0,buf1,(void*)0);
		/* DBX under AIX
		// the representative of the executable under AIX:
		snprintf(buf0,2047,"/proc/%i/object/a.out",my_pid);
		snprintf(buf1,2047,"%i",my_pid); // PID des Vaterprozesses
		// wait 1/4 sec to allow the father to leave the fork()
		usleep(250000);
		// execlp searches dbx in $PATH:
		execlp("dbx","dbx","-c",tmpfn,"-a",buf1,buf0);
		*/
		fERROR("child failed to invoke the debugger");
		_exit(EXIT_FAILURE);
	}
	// explicitly allow the debugger to attach to the running process
	prctl(PR_SET_PTRACER, ch_pid, 0, 0, 0);
	// go to sleep until a signal awakes us (e.g. by "kill -ALRM").
	// This prevents that the control leaves the faulty code.

#define USE_quit_TO_CONTINUE_EXECUTION waitpid(ch_pid, 0, 0);if(cmd_file_ok)unlink(tmpfn);
	USE_quit_TO_CONTINUE_EXECUTION
}

void AssertionError::backtrace(size_t offset, size_t nsteps)
{
#if __GNUG__
#define MAX_BT_DEPTH	64
	// (unit) (symbol) (offset) (address)
	flux::lib::RegEx rx(
		"([^(]+)\\(([^)^+]+)(\\+[^)]+)\\)\\s(\\[[^]]+\\])");
	void * buffer[MAX_BT_DEPTH];
	int nptrs = ::backtrace(buffer, MAX_BT_DEPTH);
	char ** symstrs = ::backtrace_symbols(buffer, nptrs);
	int j;
	size_t i;
	
	if (symstrs == 0)
	{
		fERROR("error calling backtrace_symbols()");
		return;
	}

	if (nptrs == MAX_BT_DEPTH)
		fWARNING("only innermost %i levels of backtrace:", MAX_BT_DEPTH-1);
	for (j=1+offset, i=0; j<nptrs; ++j, ++i)
	{
		if (nsteps > 0 and i>=nsteps)
			break;
		int status = 1;
		char const * sym = symstrs[j];
		char const ** L = rx.match(symstrs[j]);
		if (L)
		{
			sym = abi::__cxa_demangle(L[2],0,0,&status);
			if (status != 0)
				sym = L[2];
		}
		fWARNING("[backtrace] %s", sym);
		// stop backtrace at "main"
		if (strcmp(sym,"main")==0)
			j = nptrs;
		if (status == 0)
			free((char *)sym);
	}
	free(symstrs);
#endif
}

/*
 * -1: error
 *  1: local file name
 *  2: file descriptor
 *  3: unix socket filename
 *  4: udp host:port 
 *  5: tcp host:port
 */
static int parseUrl(char const * url, char ** fnhn, int * fdpn)
{
	char const * p, * q;
	char * endptr;
	if (strncmp(url,"tcp:",4)==0)
	{
		p = &(url[4]);
		if ((q = strchr(p,':')) == 0)
			return -1;
		if (fdpn == 0)
			return -1;
		if (fnhn)
		{
			*fnhn = new char[int(q-p)+1];
			strncpy(*fnhn,p,q-p);
			(*fnhn)[q-p] = '\0';
		}
		else
			return -1;
		*fdpn = (int)strtol(++q,&endptr,10);
		if (q == endptr)
		{
			delete[] *fnhn;
			*fdpn = -1;
			return -1;
		}
		return 5;
	}
	else if (strncmp(url,"udp:",4)==0)
	{
		p = &(url[4]);
		if ((q = strchr(p,':')) == 0)
			return -1;
		if (fdpn == 0)
			return -1;
		if (fnhn)
		{
			*fnhn = new char[int(q-p)+1];
			strncpy(*fnhn,p,q-p);
			(*fnhn)[q-p] = '\0';
		}
		else
			return -1;
		*fdpn = (int)strtol(++q,&endptr,10);
		if (q == endptr)
		{
			delete[] *fnhn;
			*fdpn = -1;
			return -1;
		}
		return 4;
	}
	else if (strncmp(url,"unix:",5)==0)
	{
		p = &(url[5]);
		if (strlen(p) == 0)
			return -1;
		if (fnhn)
		{
			*fnhn = new char[strlen(p)+1];
			strcpy(*fnhn,p);
		}
		else
			return -1;
		if (fdpn != 0)
			*fdpn = -1;
		return 3;
	}
	else if (strncmp(url,"fd:",3) == 0)
	{
		if (fdpn == 0)
			return -1;
		p = &(url[3]);
		*fdpn = (int)strtol(p,&endptr,10);
		if (p == endptr)
		{
			*fdpn = -1;
			return -1;
		}
		if (fnhn)
			*fnhn = 0;
		return 2;
	}
	else
	{
		if (fnhn == 0)
			return -1;
		*fnhn = new char[strlen(url)+1];
		strcpy(*fnhn,url);
		if (fdpn != 0)
			*fdpn = -1;
		return 1;
	}
}

LogManager::~LogManager()
{
	if (pub_list_) delete pub_list_;
	if (managed_pub_list_)
	{
		pub_list_t * w = managed_pub_list_;
		while (w)
		{
			delete &(w->publisher);
			w = w->next;
		}
		delete managed_pub_list_;
	}
}

LogMessagePublisher & LogManager::addPublisher(
	LogMessagePublisher & publisher
	)
{
	if (pub_list_ == 0)
		pub_list_ = new pub_list_t(publisher);
	else
		pub_list_ = new pub_list_t(publisher,pub_list_);
	return publisher;
}

LogMessagePublisher & LogManager::addPublisher(char const * url)
{
	FILE * F;
	struct stat statbuf;
	char * host;
	int port;
	LogMessagePublisher * pub = 0;
	uid_t uid = geteuid();
	gid_t gid = getegid();

	if (strcmp(url,"@gui@")==0)
	{
		char buf[128];
		char const guicmd[] = "logpager -d";
		int status;
		FILE * lv_out;
		pid_t pid = spawn_child_stream(guicmd, 0, &lv_out, 0);
		if (pid <= 0)
		{
			fprintf(stderr,"invocation of logpager failed (perl-Tk not installed?)\n");
			pub = new NULLPublisher;
			// for cleanup:
			// add publisher also to the list of managed publishers
			if (managed_pub_list_ == 0)
				managed_pub_list_ = new pub_list_t(*pub);
			else
				managed_pub_list_ = new pub_list_t(*pub,managed_pub_list_);
			return *pub;
		}
		
		errno = 0;
		if (fgets(buf,sizeof(buf),lv_out) != 0)
		{
			fclose(lv_out);
			// Umbruch abschneiden
			buf[strlen(buf)-1] = '\0';
			return addPublisher(buf);
		}
		fclose(lv_out);
		fprintf(stderr,"Error: logpager did not provide a valid socket name for logging\n");
		
		do
		{
			if (waitpid(pid, &status, WUNTRACED | WCONTINUED) == -1)
			{
				fprintf(stderr,"waitpid() failed\n");
				break;
			}

			if (WIFEXITED(status))
				fprintf(stderr,"child exited with code %i\n", WEXITSTATUS(status));
			else if (WIFSIGNALED(status))
				fprintf(stderr,"child was killed by signal %i\n", WTERMSIG(status));
			else if (WIFSTOPPED(status))
				fprintf(stderr,"child was stopped by signal %i\n", WSTOPSIG(status));
			else if (WIFCONTINUED(status))
				fprintf(stderr,"child continued to run\n");
		}
		while (not WIFEXITED(status) and not WIFSIGNALED(status));
		fprintf(stderr,"Error: invocation of logpager failed (perl-Tk not installed?)\n");
		pub = new NULLPublisher;
		// for cleanup:
		// add publisher also to the list of managed publishers
		if (managed_pub_list_ == 0)
			managed_pub_list_ = new pub_list_t(*pub);
		else
			managed_pub_list_ = new pub_list_t(*pub,managed_pub_list_);
		return *pub;
	}

	switch (parseUrl(url,&host,&port))
	{
	case -1:
		fprintf(stderr,"Error: invalid logging URL\n");
		pub = new NULLPublisher;
		break;
	case 1:
		errno = 0;
		if ((F = fopen(host,"a")) != 0)
			pub = new FILEPublisher(F,true);
		else
		{
			perror("unable to publish log messages to file");
			pub = new NULLPublisher;
		}
		delete[] host;
		break;
	case 2:
		errno = 0;
		if (fstat(port, &statbuf) == 0
			and ((statbuf.st_uid == uid and ((statbuf.st_mode & S_IRWXU) & S_IWUSR))
			or (statbuf.st_gid == gid and ((statbuf.st_mode & S_IRWXG) & S_IWGRP))
			or ((statbuf.st_mode & S_IRWXO) & S_IWOTH))
			)
		{
			pub = new FDPublisher(port);
		}
		else
		{
			if (errno == 0)
				errno = EACCES;
			perror("unable to publish log messages via file descriptor");
			pub = new NULLPublisher;
		}
		break;
	case 3:
		errno = 0;
		if (stat(host, &statbuf) == 0 and (statbuf.st_mode & S_IFSOCK) != 0)
			pub = new UNIXSocketPublisher(host);
		else
		{
			if (errno == 0)
				errno = ENOENT;
			perror("unable to publish log messages via UNIX socket");
			pub = new NULLPublisher;
		}
		delete[] host;
		break;
	case 4:
		errno = 0;
		if (NetworkPublisher::lookup(host))
			pub = new UDPPublisher(host,port);
		else
		{
			if (errno == 0)
				errno = ENOENT;
			perror("unable to publish log messages via UDP");
			pub = new NULLPublisher;
		}
		delete[] host;
		break;
	case 5:
		errno = 0;
		if (NetworkPublisher::lookup(host))
			pub = new TCPPublisher(host,port);
		else
		{
			if (errno == 0)
				errno = ENOENT;
			perror("unable to publish log messages via TCP");
			pub = new NULLPublisher;
		}
		delete[] host;
		break;
	}

	// for cleanup:
	// add publisher also to the list of managed publishers
	if (managed_pub_list_ == 0)
		managed_pub_list_ = new pub_list_t(*pub);
	else
		managed_pub_list_ = new pub_list_t(*pub,managed_pub_list_);

	addPublisher(*pub);
	return *pub;
}

void LogManager::addPublisher(char const ** urls)
{
	char const ** url = urls;
	while (*url)
	{
		addPublisher(*url);
		url++;
	}
}

bool LogManager::log(
	LogLevel level,
	char const * file,
	char const * func,
	size_t line,
	const char * msg,
	...
	)
{
	// Log messages with level 'QUIET' don't get published at all
	if (level == logQUIET)
		return true;
	if (level > log_level_)
		return true;
	bool result = true, resultp;
	pub_list_t * item = pub_list_;
	while (item)
	{
		va_list ap;
		va_start(ap,msg);
		resultp = item->publisher.publish(
			level,file,func,line,msg,ap
			);
		va_end(ap);
		result = result and resultp;
		item = item->next;
	}
	return result;
}

void LogMessagePublisher::formatMessage(
	char * buf,
	LogLevel level,
	char const * file,
	char const * func,
	size_t line,
	char const * msg,
	va_list ap
	)
{
	size_t len;
	int maxlen = maxmsglen_;
	buf[0] = '\0';

	if (usejson_)
	{
		formatMessageJSON(buf,level,file,func,line,msg,ap);
		return;
	}

	if (usetimestamp_)
	{
		time_t t = time(0);
		struct tm * lt;
		lt = localtime(&t);
		if (lt == 0)
		{
			perror(strerror(errno));
			return;
		}
		if ((len = strftime(buf,maxlen,"%b %d %H:%M:%S ",lt)) == 0)
		{
			perror("strftime");
			return;
		}
		buf = &(buf[len]);
		maxlen -= len;
	}
	if (maxlen <= 0)
		return;

	char * fmtbuf = new char[maxlen];
	vsnprintf(fmtbuf,maxlen,msg,ap);

	switch (level)
	{
	case logERROR:
		snprintf(buf,maxlen,"%s%s%s%s%s",
			usecolor_?color_ERROR:"",
			useprefix_?"E: ":"",
			fmtbuf,
			usecolor_?color_off:"",
			usenewline_?"\n":""
			);
		break;
	case logWARNING:
		snprintf(buf,maxlen,"%s%s%s%s%s",
			usecolor_?color_WARNING:"",
			useprefix_?"W: ":"",
			fmtbuf,
			usecolor_?color_off:"",
			usenewline_?"\n":""
			);
		break;
	case logNOTICE:
		snprintf(buf,maxlen,"%s%s%s%s%s",
			usecolor_?color_NOTICE:"",
			useprefix_?"N: ":"",
			fmtbuf,
			usecolor_?color_off:"",
			usenewline_?"\n":""
			);
		break; 
	case logINFO:
		snprintf(buf,maxlen,"%s%s%s%s%s",
			usecolor_?color_INFO:"",
			useprefix_?"I: ":"",
			fmtbuf,
			usecolor_?color_off:"",
			usenewline_?"\n":""
			);
		break;
	case logTHROW:
		snprintf(buf,maxlen,"%s%s%s (in %s, %s:%i)%s%s",
			usecolor_?color_THROW:"",
			useprefix_?"T: ":"",
			fmtbuf,func,file,int(line),
			usecolor_?color_off:"",
			usenewline_?"\n":""
			);
		break;
	case logDEBUG:
	case logDEBUG1:
	case logDEBUG2:
	case logDEBUG3:
	case logDEBUG4:
		// debugging messages always use the prefix ...
		snprintf(buf,maxlen,"%s%i: %s%s%s",
			usecolor_?color_DEBUG:"",
			level-logDEBUG,
			fmtbuf,
			usecolor_?color_off:"",
			usenewline_?"\n":""
			);
		break;
	default:
		break;
	}
	delete[] fmtbuf;
}

char * LogMessagePublisher::addSlashes(char const * cstr) const
{
	size_t k;
	char const * w = cstr;
	char * str;

	if (w == 0)
		return 0;

	for (k=0; *w; )
	{
		switch (*w) { case '\\': case '\'': ++k; }
		++w;
	}
	str = new char[size_t(w-cstr)+k+1];
	w = cstr;
	k = 0;
	while (*w)
	{
		switch (*w) { case '\\': case '\'': str[k++] = '\\'; break; }
		str[k++] = *w;
		++w;
	}
	str[k] = '\0';
	return str;
}

void LogMessagePublisher::formatMessageJSON(
	char * buf,
	LogLevel level,
	char const * file,
	char const * func,
	size_t line,
	char const * msg,
	va_list ap
	)
{
	time_t timestamp = time(0);
	char * fmtbuf = new char[maxmsglen_];
	char * escfmtbuf;
	int jsonlen, maxlen;
	char const level_str[][8] = {
		"QUIET","ERROR","WARNING","NOTICE","INFO","THROW",
		"DEBUG","DEBUG1","DEBUG2","DEBUG3","DEBUG4"
		};
	char const json_fmt[] =
		"{'level':'%s','file':'%s','func':'%s','line':%lu,'time':%lu,'msg':'%s'}";

	// determine length of JSON data without the message
	jsonlen = snprintf(0,0,json_fmt,level_str[level],file,func,line,timestamp,"");
	maxlen = maxmsglen_ - jsonlen - 1;

	// format the error message
	va_list ap0;
        va_copy(ap0,ap);
	vsnprintf(fmtbuf,maxmsglen_,msg,ap0);
        va_end(ap0);

	// determine the number of symbols that need to be escaped
	int k,e,len=strlen(fmtbuf);
	for (k=0,e=0; k<maxlen and k<len; ++k)
		if (fmtbuf[k]=='\\' or fmtbuf[k]=='\'') ++e;
	maxlen -= e;

	// format the error message
	va_list ap1;
	va_copy(ap1,ap);
	vsnprintf(fmtbuf,maxlen,msg,ap1);
	va_end(ap1);

	// escape special symbols
	escfmtbuf = addSlashes(fmtbuf);
	delete[] fmtbuf;
	snprintf(buf,maxmsglen_,json_fmt,level_str[level],file,func,line,timestamp,escfmtbuf);
	delete[] escfmtbuf;
}

FILEPublisher::FILEPublisher(FILE * logf, bool close)
	: logf_(logf), close_(close)
{
	usecolor_ = isatty(fileno(logf_));
	setvbuf(logf_,0,_IOLBF,0);
	usenewline_ = true;
	useprefix_ = not (logf_ == stdout or logf_ == stderr);
}

FILEPublisher::FILEPublisher(FILEPublisher const & copy)
	: LogMessagePublisher(copy)
{
	logf_ = fdopen(dup(fileno(copy.logf_)),"a");
	setvbuf(logf_,0,_IOLBF,0);
	close_ = true;
}

FILEPublisher::~FILEPublisher()
{
	if (close_) fclose(logf_);
}

bool FILEPublisher::publish(
	LogLevel level,
	char const * file,
	char const * func,
	size_t line,
	char const * msg,
	va_list ap
	)
{
	char * buf = new char[maxmsglen_];
	formatMessage(buf,level,file,func,line,msg,ap);
	bool result = (fputs(buf,logf_) != EOF);
	delete[] buf;
	return result;
}

FDPublisher::FDPublisher(int logfd)
	: logfd_(logfd)
{
	usecolor_ = isatty(logfd);
	usenewline_ = true;
	useprefix_ = not (logfd_ == 1 or logfd_ == 2);
}

FDPublisher::FDPublisher(FDPublisher const & copy)
	: LogMessagePublisher(copy), logfd_(dup(copy.logfd_)) { }

bool FDPublisher::publish(
	LogLevel level,
	char const * file,
	char const * func,
	size_t line,
	char const * msg,
	va_list ap
	)
{
	if (logfd_ == -1)
		return false;

	char * buf = new char[maxmsglen_];
	formatMessage(buf,level,file,func,line,msg,ap);
	bool result = (write(logfd_,buf,strlen(buf)) != -1);
	delete[] buf;
	return result;
}

UNIXSocketPublisher::UNIXSocketPublisher(char const * sname)
	: FDPublisher()
{
	sname_ = fSTRDUP(sname);
	usecolor_ = false;
	usenewline_ = true;
	//usejson_ = true;

	struct sockaddr_un name;
	size_t size;
	
	if ((logfd_ = socket(AF_UNIX,SOCK_DGRAM,0)) < 0)
	{
		logfd_ = -1;
		perror(strerror(errno));
		return;
	}

	fcntl(logfd_, F_SETFD, FD_CLOEXEC);

	name.sun_family = AF_LOCAL;//AF_FILE;
	strcpy(name.sun_path,sname_);

	// size of the address is the ofset of the start of the
	// filename, plus its length, plus one plus one for the
	// terminating null byte.
	size = offsetof(struct sockaddr_un, sun_path) + strlen(name.sun_path) + 1;
	if (connect(logfd_,(struct sockaddr *)&name,size) < 0)
	{
		perror(strerror(errno));
		close(logfd_);
		logfd_ = -1;
		return;
	}
}

UNIXSocketPublisher::UNIXSocketPublisher(UNIXSocketPublisher const & copy)
	: FDPublisher(copy)
{
	sname_ = strcpy(new char[strlen(copy.sname_)+1],copy.sname_);
}

#if defined(__INTEL_COMPILER) && not defined(va_copy)
#define va_copy(dest,src)	memmove(&dest,&src,sizeof(va_list))
#endif

bool UNIXSocketPublisher::publish(
	LogLevel level,
	char const * file,
	char const * func,
	size_t line,
	char const * msg,
	va_list ap
	)
{
	if (logfd_ == -1)
		return false;

	va_list ap0;
	va_copy(ap0,ap); // va_start(ap,msg);
	int len = vsnprintf(0,0,msg,ap0);
	va_end(ap0);

	char * fmtmsg = new char[len+1];

	va_list ap1;
	va_copy(ap1,ap); //va_start(ap,msg);
	vsnprintf(fmtmsg,len+1,msg,ap1);
	va_end(ap1);

	LogInfo LI(level,file,line,func,fmtmsg);
	delete[] fmtmsg;
	return LI.write(logfd_);
	/*
	char * buf = new char[maxmsglen_];
	formatMessage(buf,level,file,func,line,msg,ap);
	int buf_len = strlen(buf);
	bool result = (write(logfd_,buf,buf_len) == buf_len);
	delete[] buf;
	return result;
	*/
}

UNIXSocketPublisher::~UNIXSocketPublisher()
{
	if (logfd_ != -1)
		close(logfd_);
	delete[] sname_;
}

bool NetworkPublisher::lookup(char const * host, struct sockaddr * sa)
{
	struct addrinfo query;
	struct addrinfo * result, * rp;

	memset(&query,0,sizeof(struct addrinfo));
	query.ai_family = AF_UNSPEC;
	query.ai_socktype = SOCK_DGRAM;
	query.ai_flags = 0;
	query.ai_protocol = 0;

	int r;
	if ((r = getaddrinfo(host,0,&query,&result)) != 0)
	{
		perror(gai_strerror(r));
		return false;
	}

	if (sa)
	{
		for (rp = result; rp != 0; rp = rp->ai_next)
		{
			memcpy(sa,rp->ai_addr,sizeof(struct sockaddr));
			break;
		}
	}
	freeaddrinfo(result);
	return true;
}

TCPPublisher::TCPPublisher(
	char const * host,
	unsigned short int port
	) : FDPublisher(-1)
{
	struct sockaddr_in serv_addr;
	
	if (not lookup(host,(struct sockaddr*)&serv_addr))
		return;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	errno = 0;
	if ((logfd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{ 
		logfd_ = -1;
		perror("unable to publish log messages via TCP");
		return;
	}

	errno = 0;
	if (connect(logfd_, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("unable to publish log messages via TCP");
		close(logfd_);
		logfd_ = -1;
	}
}

bool TCPPublisher::publish(
	LogLevel level,
	char const * file,
	char const * func,
	size_t line,
	char const * msg,
	va_list ap
	)
{
	if (logfd_ == -1)
		return false;

	va_list ap0;
	va_copy(ap0,ap); // va_start(ap,msg);
	int len = vsnprintf(0,0,msg,ap0);
	va_end(ap0);

	char * fmtmsg = new char[len+1];

	va_list ap1;
	va_copy(ap1,ap); //va_start(ap,msg);
	vsnprintf(fmtmsg,len+1,msg,ap1);
	va_end(ap1);

	LogInfo LI(level,file,line,func,fmtmsg);
	delete[] fmtmsg;
	return LI.write(logfd_);
}

TCPPublisher::~TCPPublisher()
{
	if (logfd_ != -1)
		close(logfd_);
}

UDPPublisher::UDPPublisher(char const * dhost, unsigned int dport)
{
	maxmsglen_ = UDP_SAFE_PKGSIZE;
	usecolor_ = false;
	usenewline_ = false;

	if (not lookup(dhost,(struct sockaddr*)&daddr_))
		return;

	errno = 0;
	if ((logfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("unable to publish log messages via UDP");
		logfd_ = -1;
		return;
	}
	daddr_.sin_family = AF_INET;
	daddr_.sin_port = htons(dport);
}

bool UDPPublisher::publish(
	LogLevel level,
	char const * file,
	char const * func,
	size_t line,
	char const * msg,
	va_list ap
	)
{
	if (logfd_ == -1)
		return false;

	char * buf = new char[maxmsglen_];
	formatMessage(buf,level,file,func,line,msg,ap);
	bool result = (sendto(logfd_,buf,strlen(buf)+1,0,
			(struct sockaddr*)&daddr_,sizeof(daddr_)) != -1);
	delete[] buf;
	return result;
}


LogManager global_logger_(logINFO);
FILEPublisher stderr_log(stderr);
FILEPublisher stdout_log(stdout);

