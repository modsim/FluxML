#include <cstdio>
#include <cstddef>
#include <cerrno>
#include <cstring>
extern "C"
{
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
}

#include "readstream.h"

#if 0
// alte Implementierung
char * readfile(FILE * infp, size_t & size)
{
	struct stat sbuf;
	char * data;
	size_t bytes;
	int infd;

	// infp/infd ist kein file descriptor
	// fstat fehlgeschlagen
	// in beiden Fälle errno prüfen!
	if (infp == 0
		|| (infd = fileno(infp)) == -1
		|| fstat(infd,&sbuf) == -1
		|| sbuf.st_size == 0)
	{
		size = 0;
		return 0;
	}

	if (sbuf.st_size == 0)
	{
		size = 0;
		data = new char[1];
		data[0] = '\0';
		return data;
	}

	if (size == 0)
		// komplette Datei
		size = sbuf.st_size;
	else
		// min(dateigroesse,size)
		size = size < size_t(sbuf.st_size) ? size : sbuf.st_size;

	data = new char[size+1];
	
	clearerr(infp);
	bytes = fread(data,1,size,infp);
	if (ferror(infp) || bytes != size)
	{
		clearerr(infp);
		delete[] data;
		size = 0;
		return 0;
	}
	data[size] = '\0';
	return data;
}
#endif

char * readstream(FILE * stream, size_t & size)
{
#define BLEN	2048
	size_t bytes, offset, rlen;
	char * data;
	bool readall = (size==0);
	struct buf_list_t
	{
		size_t size_;
		char * data_;
		buf_list_t * next_;

		buf_list_t() : size_(0), data_(new char[BLEN]), next_(0) { }
		~buf_list_t() { delete[] data_; delete next_; }
	} head, * walk;

	if (ferror(stream))
	{
		size = 0;
		return 0;
	}

	bytes = 0;
	rlen = BLEN;
	walk = &head;
	clearerr(stream);
	while (not (ferror(stream) or feof(stream)))
	{
		if (not readall)
		{
			rlen = BLEN<size?BLEN:size;
			size -= rlen;
			if (rlen == 0) break;
		}

		if ((walk->size_ = fread(walk->data_,1,rlen,stream)) != 0)
		{
			bytes += walk->size_;
			walk->next_ = new buf_list_t;
			walk = walk->next_;
		}
	}

	if (ferror(stream))
	{
		size = 0;
		return 0;
	}
	
	data = new char[bytes+1];
	walk = &head;
	offset = 0;
	do
	{
		memcpy(data+offset, walk->data_, walk->size_);
		offset += walk->size_;
		walk = walk->next_;
	}
	while (walk);
	data[bytes] = '\0';
	size = bytes;
	return data;
}

int copyfile(char const * oldpath, char const * newpath)
{
	FILE * inF, * outF;
	char buf[2048];
	size_t bytes;

	if (oldpath == 0 || *oldpath == '\0'
		|| newpath == 0 || *newpath == '\0')
	{
		errno = ENOENT;
		return -1;
	}

	if ((inF = fopen(oldpath,"rb")) == 0)
	{
		errno = ENOENT;
		return -1;
	}

	if ((outF = fopen(newpath,"wb")) == 0)
	{
		fclose(inF);
		errno = EACCES;
		return -1;
	}

	do
	{
		if (!feof(inF) && (bytes = fread(buf,1,sizeof(buf),inF)) != 0)
			if (fwrite(buf,1,bytes,outF) != bytes)
				break;
	}
	while (!feof(inF));

	if (ferror(outF) || !feof(inF))
	{
		fclose(inF);
		fclose(outF);
		errno = EIO;
		return -1;
	}
	fclose(inF);
	fclose(outF);
	return 0;
}

int redirect_stdout(char const * dest)
{
	int old_stdout = dup(1);
	FILE * new_stdout = freopen(dest?dest:"/dev/fd/2", "w", stdout);
	if (new_stdout == 0)
		fprintf(stderr,"failed to turn off stdout\n");
	return old_stdout;
}

void restore_stdout(int old_stdout)
{
	char buf[16];
	sprintf(buf, "/dev/fd/%d", old_stdout);
	FILE * old_stdout_f = freopen(buf, "w", stdout);
	if (old_stdout_f == 0)
		fprintf(stderr,"failed to turn on stdout\n");
}

