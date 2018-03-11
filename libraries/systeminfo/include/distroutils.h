#include "sys.h"
#include <QString>

namespace Sys {
struct LsbInfo
{
	QString distributor;
	QString version;
	QString description;
	QString codename;
};

bool main_lsb_info(LsbInfo & out);
bool fallback_lsb_info(Sys::LsbInfo & out);
void lsb_postprocess(Sys::LsbInfo & lsb, Sys::DistributionInfo & out);
Sys::DistributionInfo read_lsb_release();

QString _extract_distribution(const QString & x);
QString _extract_version(const QString & x);
Sys::DistributionInfo read_legacy_release();

Sys::DistributionInfo read_os_release();
}
