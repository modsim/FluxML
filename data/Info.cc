#include "Error.h"
#include "Combinations.h"
#include "Info.h"

namespace flux {
namespace data {

uint32_t Info::computeCheckSum(uint32_t crc, int crc_scope) const
{
	if ((crc_scope & CRC_ALL_ANNOTATIONS) == 0)
		return crc;

	if (strain_.size())
		crc = update_crc32(strain_.c_str(),strain_.size(),crc);
	if (version_.size())
		crc = update_crc32(version_.c_str(),version_.size(),crc);
	if (timestamp_ > 0)
	{
		int64_t ts = timestamp_;
		crc = update_crc32(&ts,8,crc);
	}
	if (comment_.size())
		crc = update_crc32(comment_.c_str(),comment_.size(),crc);
	return crc;
}

} // namespace flux::data
} // namespace flux
