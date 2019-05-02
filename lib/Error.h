#ifndef TPP_ERROR_H
#define TPP_ERROR_H

#include <cstdio>
#include <cstring>
extern "C" {
#include <sys/types.h>
#include <netinet/in.h>
}

/**
 * Different predefined log levels.
 */
enum LogLevel {
	logQUIET	= 0,
	logERROR	= 1,
	logWARNING	= 2,
	logNOTICE	= 3,
	logINFO		= 4,
	logTHROW	= 5,
	logDEBUG	= 6,
	logDEBUG1	= 7,
	logDEBUG2	= 8,
	logDEBUG3	= 9,
	logDEBUG4	= 10
	};

/**
 * Binary representation of log messages.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
struct LogInfo
{
	int32_t logLevel_;
	int32_t pid_;
	int32_t hostNameLen_;
	int32_t fileNameLen_;
	int32_t fileLineNo_;
	int32_t funcNameLen_;
	int32_t logMessageLen_;
	int64_t timeStamp_;		// time in seconds (see clock_gettime(2))
	int64_t timeStamp_nsec_;	// time in nanoseconds

	char * hostName_;
	char * fileName_;
	char * funcName_;
	char * logMessage_;

	LogInfo(
		int32_t logLevel,
		char const * fileName,
		int32_t fileLineNo,
		char const * funcName,
		char const * logMessage
		);
	
	LogInfo() : logLevel_(0), pid_(0), hostNameLen_(0), fileNameLen_(0),
		fileLineNo_(0), funcNameLen_(0), logMessageLen_(0), timeStamp_(0),
		timeStamp_nsec_(0), hostName_(0), fileName_(0), funcName_(0),
		logMessage_(0) { }

	~LogInfo();

	/**
	 * Serialize the LogInfo struct into a file descriptor.
	 *
	 * @param fd the file descriptor
	 * @return true on success, false otherwise
	 */
	bool write(int fd) const;

	/**
	 * Serialize the LogInfo struct into a stream.
	 *
	 * @param stream the stream
	 * @return true on success, false otherwise
	 */
	bool write(FILE * stream) const;

	/**
	 * Serialize the LogInfo struct into a buffer.
	 *
	 * @param buf target buffer
	 * @param len maximum number of bytes available in buf
	 * @param nbytes number of bytes written to buf
	 * @return true on success, false otherwise
	 */
	bool write(char * buf, size_t len, size_t & nbytes) const;

	/**
	 * Deserialize a buffer into the current LogInfo object.
	 *
	 * @param buf input stream buffer
	 * @param len number of bytes available in buf
	 * @param nbytes number of bytes read from buf
	 * @return true on success, false otherwise
	 */
	bool parse(const char * buf, size_t len, size_t & nbytes);

	/**
	 * Compute the size of the serialized LogInfo struct.
	 *
	 * @return size of the serialized LogInfo struct
	 */
	size_t size() const;

};

/**
 * Abstract base class for a log message publisher.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class LogMessagePublisher
{
protected:
	/** Maximum length of generated log message */
	size_t maxmsglen_;
	/** Flag: log message contains time stamp */
	bool usetimestamp_;
	/** Flag: log message ends with a newline character */
	bool usenewline_;
	/** Flag: log message uses ANSI colors */
	bool usecolor_;
	/** Flag: log message uses prefixes (I:, E:, W:, ...) */
	bool useprefix_;
	/** Flag: log message uses raw (machine parsable) JSON format; overrides the above */
	bool usejson_;

public:
	/**
	 * Constructor.
	 */
	inline LogMessagePublisher()
		: maxmsglen_(1024),
		  usetimestamp_(false),
		  usenewline_(false),
		  usecolor_(false),
		  useprefix_(true),
		  usejson_(false) { }

	/**
	 * Copy-Constructor.
	 *
	 * @param copy LogMessagePublisher object to copy
	 */
	inline LogMessagePublisher(LogMessagePublisher const & copy)
		: maxmsglen_(copy.maxmsglen_),
		  usetimestamp_(copy.usetimestamp_),
		  usenewline_(copy.usenewline_),
		  usecolor_(copy.usecolor_),
		  useprefix_(copy.useprefix_),
		  usejson_(copy.usejson_){ }

	/**
	 * Destructor.
	 */
	inline virtual ~LogMessagePublisher() { }

protected:
	/**
	 * Formats a log message.
	 *
	 * @param buf buffer with generated log message (out)
	 * @param level the log level
	 * @param file the source file generating the log message
	 * @param func the function generating the log message
	 * @param line the line number generating the log message
	 * @param msg the log message itself (format string)
	 * @param ap additional values for the log message
	 */
	virtual void formatMessage(
		char * buf,
		LogLevel level,
		char const * file,
		char const * func,
		size_t line,
		char const * msg,
		va_list ap
		);

private:
	/**
	 * Escapes a string by adding backslashes.
	 */
	char * addSlashes(char const * cstr) const;

	/**
	 * Formats a log message to the raw JSON format (machine parsable).
	 *
	 * @see formatMessage
	 */
	void formatMessageJSON(
		char * buf,
		LogLevel level,
		char const * file,
		char const * func,
		size_t line,
		char const * msg,
		va_list ap
		);

public:
	/**
	 * Publishes a log message.
	 *
	 * @param level the log level
	 * @param file the source file generating the log message
	 * @param func the function generating the log message
	 * @param line the line number generating the log message
	 * @param msg the log message itself (format string)
	 * @param ap additional values for the log message
	 */
	virtual bool publish(
		LogLevel level,
		char const * file,
		char const * func,
		size_t line,
		char const * msg,
		va_list ap
		) = 0;

	/**
	 * Set the maximum length of a log message (including timestamps, etc).
	 *
	 * @param len desired maximum length of a log message
	 */
	inline virtual void limitMessageLength(size_t len) { maxmsglen_ = len; }

	/**
	 * Sets a flag: log messages contain a timestamp.
	 *
	 * @param t flag
	 */
	inline virtual void useTimestamp(bool t) { usetimestamp_ = t; }

	/**
	 * Sets a flag: log messages are terminated with a newline character.
	 *
	 * @param n flag
	 */
	inline virtual void useNewline(bool n) { usenewline_ = n; }

	/**
	 * Sets a flag: log messages use ANSI colors.
	 *
	 * @param c flag
	 */
	inline virtual void useColor(bool c) { usecolor_ = c; }
	
	/**
	 * Sets a flag: log messages use prefixes to indicate log levels.
	 *
	 * @param p flag
	 */
	inline virtual void usePrefix(bool p) { useprefix_ = p; }

	/**
	 * Sets a flag: log messages are in raw/JSON format
	 *
	 * @param p flag
	 */
	inline virtual void useJSON(bool p) { usejson_ = p; }

};

/**
 * Manager class for managing the output of log messages.
 * Example application:
 *
 * int main()
 * {
 *	PUBLISHLOG(stderr_log);
 *	SETLOGLEVEL(logDEBUG);
 *
 *	fDEBUG(0,"Hello World: %i",0xdeadbeef);
 *	fDEBUG(1,"Hello World: %i",0xdeadbeef);
 *	fDEBUG(2,"Hello World: %i",0xdeadbeef);
 *	fDEBUG(3,"Hello World: %i",0xdeadbeef);
 *	fINFO("Hello World: %i",0xdeadbeef);
 *	fWARNING("Hello World: %i",0xdeadbeef);
 *	fERROR("Hello World: %i",0xdeadbeef);
 *	return 0;
 * }
 *
 * nota bene:
 *  1. the defailt log level is logINFO. Only log messages <= logINFO are
 *     emitted
 *  2. without at least one PUBLISHLOG(...) no messages are emitted
 *
 * Another more sophisticated example: an alternative LogManager emits log
 * messages to a Unix socket, to another machine via UDP, and to stdout:
 *
 * LogManager LM(logINFO);
 * ...
 * int main()
 * {
 *      // send log messages to a unix filesystem socket in /tmp
 *      UNIXSocketPublisher p_unix("/tmp/my_socket_for_logs");
 *
 *      // send log messages to www.foo.com, UDP port 6000:
 *      UDPPublisher p_udp("www.foo.com", 6000);
 *
 *      // add both publishers and a stdout publisher:
 *      LM.addPublisher(p_unix);
 *      LM.addPublisher(p_udp);
 *      LM.addPublisher(stdout_log)
 *
 *      // disable color on stdout publisher
 *      stdout_log.useColor(false);
 *      // add a timestamp to the stdout publisher
 *      stdout_log.useTimestamp(true);
 *
 *      // emit log messages to LM:
 *      fINFOf(LM,"Hello World %x", 0xdeadbeef);
 *      ...
 * }
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class LogManager
{
protected:
	/** List of log-messages publishers */
	struct pub_list_t
	{
		LogMessagePublisher & publisher;
		pub_list_t * next;
		inline pub_list_t(LogMessagePublisher & p)
			: publisher(p), next(0) { }
		inline pub_list_t(LogMessagePublisher & p, pub_list_t * n)
			: publisher(p), next(n) { }
		inline ~pub_list_t() { if (next) delete next; }
	} * pub_list_, * managed_pub_list_;

	/** current log-level */
	LogLevel log_level_;

public:
	/**
	 * Constructor.
	 *
	 * @param log_level initial log-level
	 */
	inline LogManager(LogLevel log_level)
		: pub_list_(0), managed_pub_list_(0), log_level_(log_level) { }

	/**
	 * Default-Constructor.
	 */
	inline LogManager()
		: pub_list_(0), managed_pub_list_(0), log_level_(logQUIET) { }

	/**
	 * Destructor.
	 */
	virtual ~LogManager();

public:
	/**
	 * Adds a LogMessagePublisher object to the publishing queue.
	 *
	 * @param publisher LogMessagePublisher object
	 */
	LogMessagePublisher & addPublisher(
		LogMessagePublisher & publisher
		);

	/**
	 * Generates and adds LogMessagePublishers based on a string
	 * specification. Supported types are:
	 * 
	 * - "fd:<integer>"      for a file descriptor publisher
	 * - "unix:<file name>"  for a UNIX domain socket publisher
	 * - "udp:<host>:<port>" for a UDP publisher
	 *
	 * any other string specification is interpreted as a local
	 * file name.
	 * 
	 * @param url string specification
	 */
	LogMessagePublisher & addPublisher(char const * url);

	/**
	 * Adds a series of publishers specified by strings
	 *
	 * @param urls an array of string specifications
	 */
	void addPublisher(char const ** urls);

	/**
	 * Creates a log message.
	 *
	 * @param level log-level of message
	 * @param file current file name (__file__)
	 * @param func current function name (__func__)
	 * @param line current line number (__line__)
	 * @param msg format string for log message
	 * @return true, if log messages was submitted successfully
	 */
	bool log(
		LogLevel level,
		char const * file,
		char const * func,
		size_t line,
		const char * msg,
		...
		)
#ifdef __GNUG__
		__attribute__((format(printf,6,7)))
#endif
	;

	/**
	 * Sets the log-level.
	 *
	 * @param log_level log-level
	 */
	inline void setLogLevel(LogLevel log_level) { log_level_ = log_level; }

	/**
	 * Returns the current log-level.
	 *
	 * @return current log-level
	 */
	inline LogLevel getLogLevel() const { return log_level_; }

};

/**
 * A non-publishing LogMessagePublisher
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class NULLPublisher : public LogMessagePublisher
{
public:
	/**
	 * Publishes a log message.
	 *
	 * @param level the log level
	 * @param file the source file generating the log message
	 * @param func the function generating the log message
	 * @param line the line number generating the log message
	 * @param msg the log message itself (format string)
	 * @param ap additional values for the log message
	 */
	bool publish(
		LogLevel level,
		char const * file,
		char const * func,
		size_t line,
		char const * msg,
		va_list ap
		) { return false; }

};

/**
 * A LogMessagePublisher for ordinary FILE * streams.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class FILEPublisher : public LogMessagePublisher
{
private:
	FILE * logf_;
	bool close_;

public:
	FILEPublisher(FILE * logf, bool close = false);

	FILEPublisher(FILEPublisher const & copy);

	virtual ~FILEPublisher();

public:
	bool publish(
		LogLevel level,
		char const * file,
		char const * func,
		size_t line,
		char const * msg,
		va_list ap
		);

	FILE * getFILE() { return logf_; }
};

/**
 * A LogMessagePublisher for file descriptors.
 * Base-class for UNIXSocketPublisher, UDPPublisher.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class FDPublisher : public LogMessagePublisher
{
protected:
	int logfd_;

public:
	/**
	 * Constructor.
	 *
	 * @param logfd file descriptor
	 */
	FDPublisher(int logfd);

	/**
	 * Copy-Constructor.
	 *
	 * @param copy FDPublisher object to copy
	 */
	FDPublisher(FDPublisher const & copy);

	inline FDPublisher()
		: logfd_(-1) { }

public:
	bool publish(
		LogLevel level,
		char const * file,
		char const * func,
		size_t line,
		char const * msg,
		va_list ap
		);

	int getFD() const { return logfd_; }
};

/**
 * A LogMessagePublisher for a UNIX socket in the filesystem.
 * The destructor removes the generated socket file.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class UNIXSocketPublisher : public FDPublisher
{
private:
	/** File name associated with the UNIX socket */
	char * sname_;

public:
	/**
	 * Constructor.
	 *
	 * @param sname file name associated with the UNIX socket
	 */
	UNIXSocketPublisher(char const * sname);

	/**
	 * Copy-Constructor.
	 *
	 * @param copy UNIXSocketPublisher object to copy
	 */
	UNIXSocketPublisher(UNIXSocketPublisher const & copy);

	/**
	 * Destructor.
	 */
	virtual ~UNIXSocketPublisher();

public:
	bool publish(
		LogLevel level,
		char const * file,
		char const * func,
		size_t line,
		char const * msg,
		va_list ap
		);

	/**
	 * Returns the file name associated with the UNIX socket.
	 *
	 * @return File name associated with the UNIX socket
	 */
	char const * getSocketName() const { return sname_; }

};

#define NETWORK_MAX_SENDBUF 8192

class NetworkPublisher
{
public:
	virtual ~NetworkPublisher() { }

public:
	/**
	 * Hostname lookup.
	 *
	 * @param host hostname (in)
	 * @param sa sockaddr struct (out)
	 * @return true on success
	 */
	static bool lookup(char const * host, struct sockaddr * sa = 0);
};

/**
 * A LogMessagePublisher for a TCP connection.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class TCPPublisher : public FDPublisher, protected NetworkPublisher
{
private:
	struct sockaddr_in serv_addr_;

public:
	/**
	 * Constructor.
	 *
	 * @param sname file name associated with the UNIX socket
	 */
	TCPPublisher(
		char const * host,
		unsigned short int port
		);

	/**
	 * Copy-Constructor.
	 *
	 * @param copy TCPPublisher object to copy
	 */
	inline TCPPublisher(TCPPublisher const & copy)
		: FDPublisher(copy) { }

	/**
	 * Destructor.
	 */
	virtual ~TCPPublisher();

public:
	bool publish(
		LogLevel level,
		char const * file,
		char const * func,
		size_t line,
		char const * msg,
		va_list ap
		);

};

#define UDP_SAFE_PKGSIZE	548
/**
 * A LogMessagePublisher sends log messages in form of UDP packets over the
 * network.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class UDPPublisher : public FDPublisher, protected NetworkPublisher
{
private:
	struct sockaddr_in daddr_;

public:
	/**
	 * Constructor.
	 *
	 * @param dhost Destination host
	 * @param dport Destination port (UDP)
	 */
	UDPPublisher(char const * dhost, unsigned int dport);

	UDPPublisher(UDPPublisher const & copy)
		: FDPublisher(copy), daddr_(copy.daddr_) { }

	/**
	 * Destructor.
	 */
	inline virtual ~UDPPublisher() { }

public:
	bool publish(
		LogLevel level,
		char const * file,
		char const * func,
		size_t line,
		char const * msg,
		va_list ap
		);

	inline virtual void limitMessageLength(size_t len)
	{
		if (len < UDP_SAFE_PKGSIZE)
			maxmsglen_ = len;
	}
};



/** The standard / global LogManager object */
extern LogManager global_logger_;
/** A publisher for stderr */
extern FILEPublisher stderr_log;
/** A publisher for stdout */
extern FILEPublisher stdout_log;

#define SETLOGLEVEL(log_level)		global_logger_.setLogLevel(log_level)
#define GETLOGLEVEL			global_logger_.getLogLevel
#define PUBLISHLOG(publisher)		global_logger_.addPublisher(publisher)

#if defined __STRICT_ANSI__ || not defined __GNUG__

# define fLOGf(logM,logL,...) \
	(logM).log((logL),__FILE__,__func__,__LINE__,__VA_ARGS__)
# ifdef DEBUG
#  define fASSERT(expr) if(!(expr)){\
	AssertionError::attachDebugger();\
	throw AssertionError(#expr,__FILE__,__func__,__LINE__);}
#  define fASSERT_NONREACHABLE(...) do {\
	AssertionError::attachDebugger();\
	throw AssertionError("reached non-reachable code",__FILE__,__func__,__LINE__);} while(0)
# else
#  define fASSERT(expr) if(!(expr)){\
	throw AssertionError(#expr,__FILE__,__func__,__LINE__);}
#  define fASSERT_NONREACHABLE(...) do {\
	throw AssertionError("reached non-reachable code",__FILE__,__func__,__LINE__);} while(0)
# endif

#else // defined __STRICT_ANSI__ || not defined __GNUG__

#define fLOGf(logM,logL,...) \
	(logM).log((logL),__FILE__,__PRETTY_FUNCTION__,__LINE__,__VA_ARGS__)
# ifdef DEBUG
#  define fASSERT(expr) if(!(expr)){\
	AssertionError::attachDebugger();\
	throw AssertionError(#expr,__FILE__,__PRETTY_FUNCTION__,__LINE__);}
# define fASSERT_NONREACHABLE(...) do {\
	AssertionError::attachDebugger();\
	throw AssertionError("reached non-reachable code",__FILE__,__PRETTY_FUNCTION__,__LINE__);} while(0)
# else
#  define fASSERT(expr) if(!(expr)){\
	throw AssertionError(#expr,__FILE__,__PRETTY_FUNCTION__,__LINE__);}
# define fASSERT_NONREACHABLE(...) do {\
	throw AssertionError("reached non-reachable code",__FILE__,__PRETTY_FUNCTION__,__LINE__);} while(0)
# endif

#endif // defined __STRICT_ANSI__ || not defined __GNUG__

#define fERRORf(logM,...) \
	fLOGf((logM),logERROR,__VA_ARGS__)
#define fWARNINGf(logM,...) \
	fLOGf((logM),logWARNING,__VA_ARGS__)
#define fNOTICEf(logM,...) \
	fLOGf((logM),logNOTICE,__VA_ARGS__)
#define fINFOf(logM,...) \
	fLOGf((logM),logINFO,__VA_ARGS__)
#define fDEBUGf(logM,dbgL,...) \
	fLOGf((logM),(LogLevel)(logDEBUG+(dbgL)),__VA_ARGS__)

#define fLOG(logL,...) \
	fLOGf(global_logger_,(logL),__VA_ARGS__)

#define fERROR(...) \
	fLOG(logERROR,__VA_ARGS__)
#define fWARNING(...) \
	fLOG(logWARNING,__VA_ARGS__)
#define fNOTICE(...) \
	fLOG(logNOTICE,__VA_ARGS__)
#define fINFO(...) \
	fLOG(logINFO,__VA_ARGS__)
#define fDEBUG(dbgL,...) \
	fLOG((LogLevel)(logDEBUG+(dbgL)),__VA_ARGS__)

#define fTHROW(EX_CLASS, ...) do {\
	fLOG(logTHROW,"throwing %s",#EX_CLASS);\
	throw EX_CLASS(__VA_ARGS__);} while(0)
#define fRETHROW(EX_CLASS) do {\
	fLOG(logTHROW,"...");\
	throw EX_CLASS;} while(0)

/* Setting NDEBUG disables assertions and debugging completely */
#ifdef NDEBUG
# undef fASSERT
# undef fASSERT_NONREACHABLE
# define fASSERT(expr) ((void)0)
# define fASSERT_NONREACHABLE(...) ((void)0)
#endif

/**
 * Exception class AssertionError.
 * Models throw/catch-Assertions similar to the ones introduced in Java 1.4.x.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class AssertionError
{
private:
	/** File name */
	char * file_;
	/** Function name */
	char * func_;
	/** Failed expression */
	char * expr_;
	/** Line number */
	size_t line_;
	/** Error message */
	char * msg_;

public:
	/**
	 * Constructor.
	 * Not intended to be called directly.
	 *
	 * @param expr string representation of the failed expression
	 * @param func name of the calling function
	 * @param file name of the source file
	 * @param line line number offset of the source file
	 */
	AssertionError(
		char const * expr,
		char const * file,
		char const * func,
		size_t line
		);

	/**
	 * Copy-Constructor.
	 */
	AssertionError(AssertionError const & copy);

	/**
	 * Destructor.
	 */
	virtual ~AssertionError();

	/**
	 * A cast operator for casting the error message into a C style
	 * string.
	 *
	 * @return string with error message
	 */
	inline operator char const * () const { return msg_; }

	/**
	 * Return a string containing the error message.
	 *
	 * @return string with error message
	 */
	inline char const * toString() const { return msg_; }

	/**
	 * The name of the source file.
	 *
	 * @return name of the source file
	 */
	inline char const * getFileName() const { return file_; }

	/**
	 * The name of the function.
	 *
	 * @return name of the function
	 */
	inline char const * getFunctionName() const { return func_; }

	/**
	 * The expression.
	 *
	 * @return string representation of the expression
	 */
	inline char const * getExpression() const { return expr_; }

	/**
	 * The line number.
	 *
	 * @return line number in the source file
	 */
	size_t getLineNumber() const { return line_; }

	/**
	 * Under Linux and AIX a failing assertion results in the invocation
	 * of a debugger which attaches to the running process.
	 */
	static void attachDebugger();

	/**
	 * Prints a backtrace to stdout (only when using GCC/G++).
	 *
	 * @param offset offset of backtrace (stack frames to skip)
	 * @param nsteps Anzahl der Schritte (0=unbegrenzt)
	 */
	static void backtrace(size_t offset = 0, size_t nsteps = 0);

};

#endif

