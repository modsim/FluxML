#include <xercesc/util/TransService.hpp>
#include "Error.h"
#include "XMLException.h"
#include "UnicodeTools.h"

namespace flux {
namespace xml {

UTF2ASCII::UTF2ASCII(XMLCh const * unicode)
	: utf8_(0)
{
	XN XMLTransService::Codes result;
	XN XMLTranscoder * T = XN XMLPlatformUtils::fgTransService
			->makeNewTranscoderFor(
				"UTF-8", result, 16*1024
				);
	switch (result)
	{
	case XN XMLTransService::Ok: break;
	case XN XMLTransService::UnsupportedEncoding:
		fTHROW(XMLException,"failed to instantiate the transcoder: "
				"unsupported encoding");
	case XN XMLTransService::InternalFailure:
		fTHROW(XMLException,"failed to instantiate the transcoder: "
				"internal failure");
	case XN XMLTransService::SupportFilesNotFound:
		fTHROW(XMLException,"failed to instantiate the transcoder: "
				"support files not found");
	}


	XMLSize_t length = XN XMLString::stringLen(unicode);
	XMLSize_t charsEaten;
	utf8_ = new char[length+1];
	// Source string is in Unicode, want to transcode to UTF-8
	T->transcodeTo(
		unicode, length,
		(XMLByte *)utf8_, length,
		charsEaten,
		XN XMLTranscoder::UnRep_RepChar // XMLTranscoder::UnRep_Throw
		);
	utf8_[length] = '\0';
	delete T;
}

} // namespace flux::xml
} // namespace xml

